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


/******************************************************************************/
/******************************************************************************/

LuaFBOs::~LuaFBOs()
{
	for (const FBO* fbo: fbos) {
		glDeleteFramebuffers(1, &fbo->id);
	}
}


/******************************************************************************/
/******************************************************************************/

bool LuaFBOs::PushEntries(lua_State* L)
{
	CreateMetatable(L);

	REGISTER_LUA_CFUNC(CreateFBO);
	REGISTER_LUA_CFUNC(DeleteFBO);
	REGISTER_LUA_CFUNC(IsValidFBO);
	REGISTER_LUA_CFUNC(ActiveFBO);
	REGISTER_LUA_CFUNC(RawBindFBO);
	REGISTER_LUA_CFUNC(BlitFBO);

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

static GLenum GetBindingEnum(GLenum target)
{
	switch (target) {
		case GL_FRAMEBUFFER     : { return GL_FRAMEBUFFER_BINDING     ; }
		case GL_DRAW_FRAMEBUFFER: { return GL_DRAW_FRAMEBUFFER_BINDING; }
		case GL_READ_FRAMEBUFFER: { return GL_READ_FRAMEBUFFER_BINDING; }
		default: {}
	}

	return 0;
}

static GLenum ParseAttachment(const std::string& name)
{
	switch (hashString(name.c_str())) {
		case hashString(  "depth"): { return GL_DEPTH_ATTACHMENT  ; } break;
		case hashString("stencil"): { return GL_STENCIL_ATTACHMENT; } break;
		case hashString("color0" ): { return GL_COLOR_ATTACHMENT0 ; } break;
		case hashString("color1" ): { return GL_COLOR_ATTACHMENT1 ; } break;
		case hashString("color2" ): { return GL_COLOR_ATTACHMENT2 ; } break;
		case hashString("color3" ): { return GL_COLOR_ATTACHMENT3 ; } break;
		case hashString("color4" ): { return GL_COLOR_ATTACHMENT4 ; } break;
		case hashString("color5" ): { return GL_COLOR_ATTACHMENT5 ; } break;
		case hashString("color6" ): { return GL_COLOR_ATTACHMENT6 ; } break;
		case hashString("color7" ): { return GL_COLOR_ATTACHMENT7 ; } break;
		case hashString("color8" ): { return GL_COLOR_ATTACHMENT8 ; } break;
		case hashString("color9" ): { return GL_COLOR_ATTACHMENT9 ; } break;
		case hashString("color10"): { return GL_COLOR_ATTACHMENT10; } break;
		case hashString("color11"): { return GL_COLOR_ATTACHMENT11; } break;
		case hashString("color12"): { return GL_COLOR_ATTACHMENT12; } break;
		case hashString("color13"): { return GL_COLOR_ATTACHMENT13; } break;
		case hashString("color14"): { return GL_COLOR_ATTACHMENT14; } break;
		case hashString("color15"): { return GL_COLOR_ATTACHMENT15; } break;
		default                   : {                               } break;
	}

	return 0;
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
	index  = -1u;
	id     = 0;
	target = GL_FRAMEBUFFER;
	luaRef = LUA_NOREF;
	xsize = 0;
	ysize = 0;
}


void LuaFBOs::FBO::Free(lua_State* L)
{
	if (luaRef == LUA_NOREF)
		return;

	luaL_unref(L, LUA_REGISTRYINDEX, luaRef);
	luaRef = LUA_NOREF;

	glDeleteFramebuffers(1, &id);
	id = 0;

	{
		// get rid of the userdatum
		LuaFBOs& activeFBOs = CLuaHandle::GetActiveFBOs(L);
		auto& fbos = activeFBOs.fbos;

		assert(index < fbos.size());
		assert(fbos[index] == this);

		fbos[index] = fbos.back();
		fbos[index]->index = index;
		fbos.pop_back();
	}
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

	if (fbo->luaRef == LUA_NOREF)
		return 0;

	// read the value from the ref table
	lua_rawgeti(L, LUA_REGISTRYINDEX, fbo->luaRef);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	return 1;
}


int LuaFBOs::meta_newindex(lua_State* L)
{
	FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->luaRef == LUA_NOREF)
		return 0;

	if (lua_israwstring(L, 2)) {
		const std::string& key = lua_tostring(L, 2);
		const GLenum type = ParseAttachment(key);

		if (type != 0) {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
			glBindFramebuffer(fbo->target, fbo->id);
			ApplyAttachment(L, 3, fbo, type);
			glBindFramebuffer(fbo->target, currentFBO);
		}
		else if (key == "drawbuffers") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
			glBindFramebuffer(fbo->target, fbo->id);
			ApplyDrawBuffers(L, 3);
			glBindFramebuffer(fbo->target, currentFBO);
		}
		else if (key == "readbuffer") {
			GLint currentFBO;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
			glBindFramebuffer(fbo->target, fbo->id);

			if (lua_isnumber(L, 3))
				glReadBuffer((GLenum)lua_toint(L, 3));

			glBindFramebuffer(fbo->target, currentFBO);
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

bool LuaFBOs::AttachObject(
	lua_State* L,
	int index,
	FBO* fbo,
	GLenum attachID,
	GLenum attachTarget,
	GLenum attachLevel
) {
	if (lua_isnil(L, index)) {
		// nil object
		glFramebufferTexture2D(fbo->target, attachID, GL_TEXTURE_2D, 0, 0);
		glFramebufferRenderbuffer(fbo->target, attachID, GL_RENDERBUFFER, 0);
		return true;
	}
	if (lua_israwstring(L, index)) {
		// custom texture
		const LuaTextures& textures = CLuaHandle::GetActiveTextures(L);
		const LuaTextures::Texture* tex = textures.GetInfo(lua_tostring(L, index));

		if (tex == nullptr)
			return false;

		if (attachTarget == 0)
			attachTarget = tex->target;

		glFramebufferTexture2D(fbo->target, attachID, attachTarget, tex->id, attachLevel);
		fbo->xsize = tex->xsize;
		fbo->ysize = tex->ysize;
		return true;
	}

	// render buffer object
	const LuaRBOs::RBO* rbo = static_cast<LuaRBOs::RBO*>(LuaUtils::GetUserData(L, index, "RBO"));

	if (rbo == nullptr)
		return false;

	if (attachTarget == 0)
		attachTarget = rbo->target;

	glFramebufferRenderbuffer(fbo->target, attachID, attachTarget, rbo->id);

	fbo->xsize = rbo->xsize;
	fbo->ysize = rbo->ysize;
	return true;
}


bool LuaFBOs::ApplyAttachment(
	lua_State* L,
	int index,
	FBO* fbo,
	const GLenum attachID
) {
	if (attachID == 0)
		return false;

	if (!lua_istable(L, index))
		return AttachObject(L, index, fbo, attachID);

	const int table = (index < 0) ? index : (lua_gettop(L) + index + 1);

	GLenum target = 0;
	GLint  level  = 0;

	lua_rawgeti(L, table, 2);
	if (lua_isnumber(L, -1))
		target = (GLenum)lua_toint(L, -1);
	lua_pop(L, 1);

	lua_rawgeti(L, table, 3);
	if (lua_isnumber(L, -1))
		level = (GLint)lua_toint(L, -1);
	lua_pop(L, 1);

	lua_rawgeti(L, table, 1);
	const bool success = AttachObject(L, -1, fbo, attachID, target, level);
	lua_pop(L, 1);

	return success;
}


bool LuaFBOs::ApplyDrawBuffers(lua_State* L, int index)
{
	if (lua_isnumber(L, index)) {
		glDrawBuffer(static_cast<GLenum>(lua_toint(L, index)));
		return true;
	}
	if (lua_istable(L, index)) {
		static_assert(sizeof(GLenum) == sizeof(int), "");

		int buffers[32] = {GL_NONE};
		const int count = LuaUtils::ParseIntArray(L, index, buffers, sizeof(buffers) / sizeof(buffers[0]));

		glDrawBuffers(count, reinterpret_cast<const GLenum*>(&buffers[0]));
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
	if (bindTarget == 0)
		return 0;

	// maintain a lua table to hold RBO references
 	lua_newtable(L);
	fbo.luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
	if (fbo.luaRef == LUA_NOREF)
		return 0;

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);

	glGenFramebuffers(1, &fbo.id);
	glBindFramebuffer(fbo.target, fbo.id);


	FBO* fboPtr = static_cast<FBO*>(lua_newuserdata(L, sizeof(FBO)));
	*fboPtr = fbo;

	luaL_getmetatable(L, "FBO");
	lua_setmetatable(L, -2);

	// parse the initialization table
	if (lua_istable(L, table)) {
		for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
			if (!lua_israwstring(L, -2))
				continue;

			const std::string& key = lua_tostring(L, -2);
			const GLenum type = ParseAttachment(key);
			if (type != 0) {
				ApplyAttachment(L, -1, fboPtr, type);
			}
			else if (key == "drawbuffers") {
				ApplyDrawBuffers(L, -1);
			}
		}
	}

	// revert to the old fbo
	glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

	if (fboPtr->luaRef != LUA_NOREF) {
		LuaFBOs& activeFBOs = CLuaHandle::GetActiveFBOs(L);
		auto& fbos = activeFBOs.fbos;

		fbos.push_back(fboPtr);
		fboPtr->index = fbos.size() - 1;
	}

	return 1;
}


int LuaFBOs::DeleteFBO(lua_State* L)
{
	if (lua_isnil(L, 1))
		return 0;

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

	const GLenum  fboTarget = (GLenum)luaL_optint(L, 2, fbo->target);
	const GLenum bindTarget = GetBindingEnum(fboTarget);

	if (bindTarget == 0) {
		lua_pushboolean(L, false);
		return 1;
	}

	GLint currentFBO;
	glGetIntegerv(bindTarget, &currentFBO);

	glBindFramebuffer(fboTarget, fbo->id);
	const GLenum status = glCheckFramebufferStatus(fboTarget);
	glBindFramebuffer(fboTarget, currentFBO);

	lua_pushboolean(L, (status == GL_FRAMEBUFFER_COMPLETE));
	lua_pushnumber(L, status);
	return 2;
}


int LuaFBOs::ActiveFBO(lua_State* L)
{
	CheckDrawingEnabled(L, __func__);
	
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->id == 0)
		return 0;

	int funcIndex = 2;

	// custom target option
	const GLenum  fboTarget = (lua_israwnumber(L, funcIndex))? (GLenum)lua_toint(L, funcIndex++): fbo->target;
	const GLenum bindTarget = GetBindingEnum(fboTarget);

	// silently skip deprecated 'identities' argument
	funcIndex += (lua_isboolean(L, funcIndex));

	if (!lua_isfunction(L, funcIndex))
		luaL_error(L, "[LuaFBOs::%s] argument %d not a function", __func__, funcIndex);

	if (bindTarget == 0)
		return 0;

	glAttribStatePtr->PushViewPortBit();
	glAttribStatePtr->ViewPort(0, 0, fbo->xsize, fbo->ysize);

	GLint currentFBO = 0;
	glGetIntegerv(bindTarget, &currentFBO);
	glBindFramebuffer(fboTarget, fbo->id);

	// draw
	const int error = lua_pcall(L, (lua_gettop(L) - funcIndex), 0, 0);

	glBindFramebuffer(fboTarget, currentFBO);
	glAttribStatePtr->PopBits();

	if (error != 0) {
		LOG_L(L_ERROR, "gl.ActiveFBO: error(%i) = %s", error, lua_tostring(L, -1));
		lua_error(L);
	}

	return 0;
}


int LuaFBOs::RawBindFBO(lua_State* L)
{
	//CheckDrawingEnabled(L, __func__);

	if (lua_isnil(L, 1)) {
		// revert to default or specified FB
		glBindFramebuffer((GLenum) luaL_optinteger(L, 2, GL_FRAMEBUFFER), luaL_optinteger(L, 3, 0));
		return 0;
	}
		
	const FBO* fbo = static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));

	if (fbo->id == 0)
		return 0;

	GLint currentFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
	glBindFramebuffer((GLenum) luaL_optinteger(L, 2, fbo->target), fbo->id);

	lua_pushnumber(L, currentFBO);
	return 1;
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

		glBlitFramebuffer(x0Src, y0Src, x1Src, y1Src,  x0Dst, y0Dst, x1Dst, y1Dst,  mask, filter);
		return 0;
	}

	const FBO* fboSrc = (lua_isnil(L, 1))? nullptr: static_cast<FBO*>(luaL_checkudata(L, 1, "FBO"));
	const FBO* fboDst = (lua_isnil(L, 6))? nullptr: static_cast<FBO*>(luaL_checkudata(L, 6, "FBO"));

	// if passed a non-nil arg, userdatum buffer must always be valid
	// otherwise the default framebuffer is substituted as its target
	if (fboSrc != nullptr && fboSrc->id == 0)
		return 0;
	if (fboDst != nullptr && fboDst->id == 0)
		return 0;

	const GLint x0Src = (GLint)luaL_checknumber(L, 2);
	const GLint y0Src = (GLint)luaL_checknumber(L, 3);
	const GLint x1Src = (GLint)luaL_checknumber(L, 4);
	const GLint y1Src = (GLint)luaL_checknumber(L, 5);

	const GLint x0Dst = (GLint)luaL_checknumber(L, 7);
	const GLint y0Dst = (GLint)luaL_checknumber(L, 8);
	const GLint x1Dst = (GLint)luaL_checknumber(L, 9);
	const GLint y1Dst = (GLint)luaL_checknumber(L, 10);

	const GLbitfield mask = (GLbitfield)luaL_optint(L, 11, GL_COLOR_BUFFER_BIT);
	const GLenum filter = (GLenum)luaL_optint(L, 12, GL_NEAREST);

	GLint currentFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, (fboSrc == nullptr)? 0: fboSrc->id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (fboDst == nullptr)? 0: fboDst->id);

	glBlitFramebuffer(x0Src, y0Src, x1Src, y1Src,  x0Dst, y0Dst, x1Dst, y1Dst,  mask, filter);

	glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
	return 0;
}


/******************************************************************************/
/******************************************************************************/
