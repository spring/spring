/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "LuaFBOs.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "LuaOpenGL.h"
#include "LuaRBOs.h"
#include "LuaTextures.h"

#include "System/Log/ILog.h"

#include <map>


/******************************************************************************/
/******************************************************************************/

LuaFBOs::LuaFBOs()
{
}


LuaFBOs::~LuaFBOs()
{
	for (FBO* fbo: fbos) {
		glDeleteFramebuffersEXT(1, &fbo->id);
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaFBOs::PushEntries(lua_State* L)
{
	CreateMetatable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(CreateFBO);
	REGISTER_LUA_CFUNC(DeleteFBO);
	REGISTER_LUA_CFUNC(IsValidFBO);
	REGISTER_LUA_CFUNC(ActiveFBO);
	REGISTER_LUA_CFUNC(UnsafeSetFBO);
	if (GLEW_EXT_framebuffer_blit) {
		REGISTER_LUA_CFUNC(BlitFBO);
	}

	return true;
}


bool LuaFBOs::CreateMetatable(lua_State* L)
{
	luaL_newmetatable(L, "FBO");
	HSTR_PUSH_CFUNC(L, "__gc",        meta_gc);
	HSTR_PUSH_CFUNC(L, "__index",     meta_index);
	HSTR_PUSH_CFUNC(L, "__newindex",  meta_newindex);
	lua_pop(L, 1);
	return true;
}


/******************************************************************************/
/******************************************************************************/

inline void CheckDrawingEnabled(lua_State* L, const char* caller)
{
	if (!LuaOpenGL::IsDrawingEnabled(L)) {
		luaL_error(L, "%s(): OpenGL calls can only be used in Draw() "
		              "call-ins, or while creating display lists", caller);
	}
}


/******************************************************************************/
/******************************************************************************/

const LuaFBOs::FBO* LuaFBOs::GetLuaFBO(lua_State* L, int index)
{
	return static_cast<FBO*>(LuaUtils::GetUserData(L, index, "FBO"));
}


/******************************************************************************/
/******************************************************************************/

void LuaFBOs::FBO::Init(lua_State* L)
{
	id     = 0;
	target = GL_FRAMEBUFFER_EXT;
	luaRef = LUA_NOREF;
	xsize = 0;
	ysize = 0;
}


void LuaFBOs::FBO::Free(lua_State* L)
{
	if (luaRef == LUA_NOREF) {
		return;
	}
	luaL_unref(L, LUA_REGISTRYINDEX, luaRef);
	luaRef = LUA_NOREF;

	glDeleteFramebuffersEXT(1, &id);
	id = 0;

	CLuaHandle::GetActiveFBOs(L).fbos.erase(this);
}


/******************************************************************************/
/******************************************************************************/

int LuaFBOs::meta_gc(lua_State* L)
{
	FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	fbo->Free(L);
	return 0;
}


int LuaFBOs::meta_index(lua_State* L)
{
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	if (fbo->luaRef == LUA_NOREF) {
		return 0;
	}

	// read the value from the ref table
	lua_rawgeti(L, LUA_REGISTRYINDEX, fbo->luaRef);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);

	return 1;
}


int LuaFBOs::meta_newindex(lua_State* L)
{
	FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	if (fbo->luaRef == LUA_NOREF) {
		return 0;
	}

	if (lua_israwstring(L, 2)) {
		const std::string key = lua_tostring(L, 2);
		const GLenum type = ParseAttachment(key);
		if (type != 0) {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);
			ApplyAttachment(L, 3, fbo, type);
			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "drawbuffers") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);
			ApplyDrawBuffers(L, 3);
			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "readbuffer") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
			glBindFramebufferEXT(fbo->target, fbo->id);
			if (lua_isnumber(L, 3)) {
				const GLenum buffer = (GLenum)lua_toint(L, 3);
				glReadBuffer(buffer);
			}
			glBindFramebufferEXT(fbo->target, currentFBO);
		}
		else if (key == "target") {
			return 0;// fbo->target = (GLenum)luaL_checkint(L, 3);
		}
	}

	// set the key/value in the ref table
	lua_rawgeti(L, LUA_REGISTRYINDEX, fbo->luaRef);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_rawset(L, -3);

	return 0;
}


/******************************************************************************/
/******************************************************************************/

static GLenum GetBindingEnum(GLenum target)
{
	switch (target) {
		case GL_FRAMEBUFFER_EXT:      { return GL_FRAMEBUFFER_BINDING_EXT;      }
		case GL_DRAW_FRAMEBUFFER_EXT: { return GL_DRAW_FRAMEBUFFER_BINDING_EXT; }
		case GL_READ_FRAMEBUFFER_EXT: { return GL_READ_FRAMEBUFFER_BINDING_EXT; }
		default: {
			return 0;
		}
	}
}


/******************************************************************************/
/******************************************************************************/

GLenum LuaFBOs::ParseAttachment(const std::string& name)
{
	static std::map<std::string, GLenum> attachMap;
	if (attachMap.empty()) {
		attachMap["depth"]   = GL_DEPTH_ATTACHMENT_EXT; 
		attachMap["stencil"] = GL_STENCIL_ATTACHMENT_EXT;
		attachMap["color0"]  = GL_COLOR_ATTACHMENT0_EXT;
		attachMap["color1"]  = GL_COLOR_ATTACHMENT1_EXT;
		attachMap["color2"]  = GL_COLOR_ATTACHMENT2_EXT;
		attachMap["color3"]  = GL_COLOR_ATTACHMENT3_EXT;
		attachMap["color4"]  = GL_COLOR_ATTACHMENT4_EXT;
		attachMap["color5"]  = GL_COLOR_ATTACHMENT5_EXT;
		attachMap["color6"]  = GL_COLOR_ATTACHMENT6_EXT;
		attachMap["color7"]  = GL_COLOR_ATTACHMENT7_EXT;
		attachMap["color8"]  = GL_COLOR_ATTACHMENT8_EXT;
		attachMap["color9"]  = GL_COLOR_ATTACHMENT9_EXT;
		attachMap["color10"] = GL_COLOR_ATTACHMENT10_EXT;
		attachMap["color11"] = GL_COLOR_ATTACHMENT11_EXT;
		attachMap["color12"] = GL_COLOR_ATTACHMENT12_EXT;
		attachMap["color13"] = GL_COLOR_ATTACHMENT13_EXT;
		attachMap["color14"] = GL_COLOR_ATTACHMENT14_EXT;
		attachMap["color15"] = GL_COLOR_ATTACHMENT15_EXT;
	}
	std::map<string, GLenum>::const_iterator it = attachMap.find(name);
	if (it != attachMap.end()) {
		return it->second;
	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

bool LuaFBOs::AttachObject(lua_State* L, int index,
		                       FBO* fbo, GLenum attachID,
		                       GLenum attachTarget, GLenum attachLevel)
{
	if (lua_isnil(L, index)) {
		// nil object
		glFramebufferTexture2DEXT(fbo->target, attachID,
		                          GL_TEXTURE_2D, 0, 0);
		glFramebufferRenderbufferEXT(fbo->target, attachID,
		                             GL_RENDERBUFFER_EXT, 0);
		return true;
	}
	else if (lua_israwstring(L, index)) {
		// custom texture
		const string texName = lua_tostring(L, index);
		LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* tex = textures.GetInfo(texName);
		if (tex == NULL) {
			return false;
		}
		if (attachTarget == 0) {
			attachTarget = tex->target;
		} 
		glFramebufferTexture2DEXT(fbo->target,
		                          attachID, attachTarget, tex->id, attachLevel);
		fbo->xsize = tex->xsize;
		fbo->ysize = tex->ysize;
		return true;
	}
	else {
		// render buffer object
		const LuaRBOs::RBO* rbo =
			(LuaRBOs::RBO*)LuaUtils::GetUserData(L, index, "RBO");
		if (rbo == NULL) {
			return false;
		}
		if (attachTarget == 0) {
			attachTarget = rbo->target;
		} 
		glFramebufferRenderbufferEXT(fbo->target,
		                             attachID, attachTarget, rbo->id);
		fbo->xsize = rbo->xsize;
		fbo->ysize = rbo->ysize;
		return true;
	}

	return false;
}


bool LuaFBOs::ApplyAttachment(lua_State* L, int index,
                              FBO* fbo, const GLenum attachID)
{
	if (attachID == 0) {
		return false;
	}

	if (!lua_istable(L, index)) {
		return AttachObject(L, index, fbo, attachID);
	}

	const int table = (index < 0) ? index : (lua_gettop(L) + index + 1);

	GLenum target = 0;
	GLint  level  = 0;

	lua_rawgeti(L, table, 2);
	if (lua_isnumber(L, -1)) { target = (GLenum)lua_toint(L, -1); }
	lua_pop(L, 1);

	lua_rawgeti(L, table, 3);
	if (lua_isnumber(L, -1)) { level = (GLint)lua_toint(L, -1); }
	lua_pop(L, 1);

	lua_rawgeti(L, table, 1);
	const bool success = AttachObject(L, -1, fbo, attachID, target, level);
	lua_pop(L, 1);

	return success;
}


bool LuaFBOs::ApplyDrawBuffers(lua_State* L, int index)
{
	if (lua_isnumber(L, index)) {
		const GLenum buffer = (GLenum)lua_toint(L, index);
		glDrawBuffer(buffer);
		return true;
	}
	else if (lua_istable(L, index) && GLEW_ARB_draw_buffers) {
		const int table = (index > 0) ? index : (lua_gettop(L) + index + 1);

		vector<GLenum> buffers;
		for (int i = 1; lua_checkgeti(L, table, i) != 0; lua_pop(L, 1), i++) {
			const GLenum buffer = (GLenum)luaL_checkint(L, -1);
			buffers.push_back(buffer);
		}

		glDrawBuffersARB(buffers.size(), &buffers.front());

		return true;
	}
	return false;
}


/******************************************************************************/
/******************************************************************************/

int LuaFBOs::CreateFBO(lua_State* L)
{
	FBO fbo;
	fbo.Init(L);

	const int table = 1;
/*
	if (lua_istable(L, table)) {
		lua_getfield(L, table, "target");
		if (lua_isnumber(L, -1)) {
			fbo.target = (GLenum)lua_toint(L, -1);
		} else {
			lua_pop(L, 1);
		}
	}
*/
	const GLenum bindTarget = GetBindingEnum(fbo.target);
	if (bindTarget == 0) {
		return 0;
	}

	// maintain a lua table to hold RBO references
 	lua_newtable(L);
	fbo.luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	if (fbo.luaRef == LUA_NOREF) {
		return 0;
	}

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);

	glGenFramebuffersEXT(1, &fbo.id);
	glBindFramebufferEXT(fbo.target, fbo.id);


	FBO* fboPtr = static_cast<FBO*>(lua_newuserdata(L, sizeof(FBO)));
	*fboPtr = fbo;

	luaL_getmetatable(L, "FBO");
	lua_setmetatable(L, -2);

	// parse the initialization table
	if (lua_istable(L, table)) {
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (lua_israwstring(L, -2)) {
				const std::string key = lua_tostring(L, -2);
				const GLenum type = ParseAttachment(key);
				if (type != 0) {
					ApplyAttachment(L, -1, fboPtr, type);
				}
				else if (key == "drawbuffers") {
					ApplyDrawBuffers(L, -1);
				}
			}
		}
	}

	// revert to the old fbo
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);

	if (fboPtr->luaRef != LUA_NOREF) {
		CLuaHandle::GetActiveFBOs(L).fbos.insert(fboPtr);
	}

	return 1;
}


int LuaFBOs::DeleteFBO(lua_State* L)
{
	if (lua_isnil(L, 1)) {
		return 0;
	}
	FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	fbo->Free(L);
	return 0;
}


int LuaFBOs::IsValidFBO(lua_State* L)
{
	if (lua_isnil(L, 1) || !lua_isuserdata(L, 1)) {
		lua_pushboolean(L, false);
		return 1;
	}
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	if ((fbo->id == 0) || (fbo->luaRef == LUA_NOREF)) {
		lua_pushboolean(L, false);
		return 1;
	}

	const GLenum target = (GLenum)luaL_optint(L, 2, fbo->target);
	const GLenum bindTarget = GetBindingEnum(target);
	if (bindTarget == 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);
	glBindFramebufferEXT(target, fbo->id);
	const GLenum status = glCheckFramebufferStatusEXT(target);
	glBindFramebufferEXT(target, currentFBO);

	const bool valid = (status == GL_FRAMEBUFFER_COMPLETE_EXT);
	lua_pushboolean(L, valid);
	lua_pushnumber(L, status);
	return 2;
}


int LuaFBOs::ActiveFBO(lua_State* L)
{
	CheckDrawingEnabled(L, __FUNCTION__);
	
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	if (fbo->id == 0) {
		return 0;
	}

	int funcIndex = 2;

	// target and matrix manipulation options
	GLenum target = fbo->target;
	if (lua_israwnumber(L, funcIndex)) {
		target = (GLenum)lua_toint(L, funcIndex);
		funcIndex++;
	}
	bool identities = false;
	if (lua_isboolean(L, funcIndex)) {
		identities = lua_toboolean(L, funcIndex);
		funcIndex++;
	}
		
	if (!lua_isfunction(L, funcIndex)) {
		luaL_error(L, "Incorrect arguments to gl.ActiveFBO()");
	}
	const int args = lua_gettop(L);

	const GLenum bindTarget = GetBindingEnum(target);
	if (bindTarget == 0) {
		return 0;
	}

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, fbo->xsize, fbo->ysize);
	if (identities) {
		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
	}
	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);
	glBindFramebufferEXT(target, fbo->id);

	const int error = lua_pcall(L, (args - funcIndex), 0, 0);

	glBindFramebufferEXT(target, currentFBO);
	if (identities) {
		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW);  glPopMatrix();
	}
	glPopAttrib();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.ActiveFBO: error(%i) = %s",
				error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaFBOs::UnsafeSetFBO(lua_State* L)
{
	//CheckDrawingEnabled(L, __FUNCTION__);

	if (lua_isnil(L, 1)) {
		const GLenum target = (GLenum)luaL_optnumber(L, 2, GL_FRAMEBUFFER_EXT);
		glBindFramebufferEXT(target, 0);
		return 0;
	}
		
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	if (fbo->id == 0) {
		return 0;
	}
	const GLenum target = (GLenum)luaL_optnumber(L, 2, fbo->target);
	glBindFramebufferEXT(target, fbo->id);
	return 0;
}


/******************************************************************************/

int LuaFBOs::BlitFBO(lua_State* L)
{
	if (lua_israwnumber(L, 1)) {
		const GLint x0Src = (GLint)luaL_checknumber(L, 1);
		const GLint y0Src = (GLint)luaL_checknumber(L, 2);
		const GLint x1Src = (GLint)luaL_checknumber(L, 3);
		const GLint y1Src = (GLint)luaL_checknumber(L, 4);

		const GLint x0Dst = (GLint)luaL_checknumber(L, 5);
		const GLint y0Dst = (GLint)luaL_checknumber(L, 6);
		const GLint x1Dst = (GLint)luaL_checknumber(L, 7);
		const GLint y1Dst = (GLint)luaL_checknumber(L, 8);

		const GLbitfield mask = (GLbitfield)luaL_optint(L, 9, GL_COLOR_BUFFER_BIT);
		const GLenum filter = (GLenum)luaL_optint(L, 10, GL_NEAREST);

		glBlitFramebufferEXT(x0Src, y0Src, x1Src, y1Src,
												 x0Dst, y0Dst, x1Dst, y1Dst,
												 mask, filter);
	}
	else {
		const FBO* fboSrc = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
		if (fboSrc->id == 0) { return 0; }
		const GLint x0Src = (GLint)luaL_checknumber(L, 2);
		const GLint y0Src = (GLint)luaL_checknumber(L, 3);
		const GLint x1Src = (GLint)luaL_checknumber(L, 4);
		const GLint y1Src = (GLint)luaL_checknumber(L, 5);

		const FBO* fboDst = static_cast<FBO*>(luaL_checkudata(L, 6, "FBO"));
		if (fboDst->id == 0) { return 0; }
		const GLint x0Dst = (GLint)luaL_checknumber(L, 7);
		const GLint y0Dst = (GLint)luaL_checknumber(L, 8);
		const GLint x1Dst = (GLint)luaL_checknumber(L, 9);
		const GLint y1Dst = (GLint)luaL_checknumber(L, 10);

		const GLbitfield mask = (GLbitfield)luaL_optint(L, 11, GL_COLOR_BUFFER_BIT);
		const GLenum filter = (GLenum)luaL_optint(L, 12, GL_NEAREST);

		GLint currentFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);
		
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fboSrc->id);
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fboDst->id);

		glBlitFramebufferEXT(x0Src, y0Src, x1Src, y1Src,
												 x0Dst, y0Dst, x1Dst, y1Dst,
												 mask, filter);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
	}

	return 0;
}


/******************************************************************************/
/******************************************************************************/
