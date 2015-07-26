/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(DEBUG_GLSTATE)

#define NO_GL_WRAP

#include <map>
#include <string>
#include <set>

#include "glStateDebug.h"

#include "System/Log/ILog.h"

#include "System/Platform/Threading.h"

#define VERIFYGL(func, type, pname, default, pstr) \
	{ \
		if (errorsSet.find(id + pstr) == errorsSet.end()) { \
			type _temp; \
			func(pname, &_temp); \
			if (_temp != default) \
			{ \
				LOG_L(L_ERROR, "%s was not set correctly when %s %s: %f", pstr, AREA,  id.c_str(), (float) _temp); \
				LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str()); \
				errorsSet.insert(id + pstr); \
			} \
		} \
	}
	
#define VERIFYGL2(func, type, pname, default0, default1, pstr) \
	{ \
		if (errorsSet.find(id + pstr) == errorsSet.end()) { \
			type _temp[2]; \
			func(pname, _temp); \
			if (_temp[0] != default0 || _temp[1] != default1) \
			{ \
				LOG_L(L_ERROR, "%s was not set correctly when %s %s: %f %f", pstr, AREA,  id.c_str(), (float) _temp[0], (float) _temp[1]); \
				LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str()); \
				errorsSet.insert(id + pstr); \
			} \
		} \
	}
	
#define VERIFYGL4(func, type, pname, default0, default1, default2, default3, pstr) \
	{ \
		if (errorsSet.find(id + pstr) == errorsSet.end()) { \
			type _temp[4]; \
			func(pname, _temp); \
			if (_temp[0] != default0 || _temp[1] != default1 || _temp[2] != default2 || _temp[3] != default3) \
			{ \
				LOG_L(L_ERROR, "%s was not set correctly when %s %s: %f %f %f %f", pstr, AREA,  id.c_str(), (float) _temp[0], (float) _temp[1], (float) _temp[2], (float) _temp[3]); \
				LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str()); \
				errorsSet.insert(id + pstr); \
			} \
		} \
	}
	

#define VERIFYGLBOOL(pname, default) VERIFYGL(glGetBooleanv, GLboolean, pname, default, #pname)
#define VERIFYGLBOOL4(pname, default0, default1, default2, default3) VERIFYGL4(glGetBooleanv, GLboolean, pname, default0, default1, default2, default3, #pname)

#define VERIFYGLFLOAT(pname, default) VERIFYGL(glGetFloatv, GLfloat, pname, default, #pname)
#define VERIFYGLFLOAT4(pname, default0, default1, default2, default3) VERIFYGL4(glGetFloatv, GLfloat, pname, default0, default1, default2, default3, #pname)

#define VERIFYGLINT(pname, default) VERIFYGL(glGetIntegerv, GLint, pname, default, #pname)
#define VERIFYGLINT2(pname, default0, default1) VERIFYGL2(glGetIntegerv, GLint, pname, default0, default1, #pname)

std::map<std::string, std::string> CGLStateChecker::lastSet;
std::set<std::string> CGLStateChecker::errorsSet;


void _wrap_glEnable(GLenum pname, std::string pstr, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet[pstr] = location;
	}
	glEnable(pname);
}

void _wrap_glDisable(GLenum pname, std::string pstr, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet[pstr] = location;
	}
	glDisable(pname);
}

void _wrap_glBlendFunc(GLenum sfactor, GLenum dfactor, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_BLEND_SRC_RGB"] = location;
		CGLStateChecker::lastSet["GL_BLEND_SRC_ALPHA"] = location;
		CGLStateChecker::lastSet["GL_BLEND_DST_RGB"] = location;
		CGLStateChecker::lastSet["GL_BLEND_DST_ALPHA"] = location;
	}
	glBlendFunc(sfactor, dfactor);
}

void _wrap_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_BLEND_SRC_RGB"] = location;
		CGLStateChecker::lastSet["GL_BLEND_SRC_ALPHA"] = location;
		CGLStateChecker::lastSet["GL_BLEND_DST_RGB"] = location;
		CGLStateChecker::lastSet["GL_BLEND_DST_ALPHA"] = location;
	}
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void _wrap_glColor3f(GLfloat red, GLfloat green, GLfloat blue, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_CURRENT_COLOR"] = location;
	}
	glColor3f(red, green, blue);
}

void _wrap_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_CURRENT_COLOR"] = location;
	}
	glColor4f(red, green, blue, alpha);
}

void _wrap_glColor4fv(const GLfloat *v, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_CURRENT_COLOR"] = location;
	}
	glColor4fv(v);
}

void _wrap_glDepthMask(GLboolean flag, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_DEPTH_WRITEMASK"] = location;
	}
	glDepthMask(flag);
}


void _wrap_glDepthFunc(GLenum func, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_DEPTH_FUNC"] = location;
	}
	glDepthFunc(func);
}

void _wrap_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, std::string location)
{
	if(Threading::IsMainThread())
	{
		CGLStateChecker::lastSet["GL_COLOR_WRITEMASK"] = location;
	}
	glColorMask(red, green, blue, alpha);
}

CGLStateChecker::CGLStateChecker(std::string id) : id(id)
{
#define AREA "entering"
	if (Threading::IsMainThread())
	{
		//VERIFYGLBOOL(GL_CULL_FACE, GL_FALSE)
		VERIFYGLBOOL(GL_ALPHA_TEST, GL_FALSE)
		VERIFYGLBOOL(GL_LIGHTING, GL_FALSE)
		VERIFYGLBOOL(GL_SCISSOR_TEST, GL_FALSE)
		VERIFYGLBOOL(GL_STENCIL_TEST, GL_FALSE)
		// VERIFYGLBOOL(GL_TEXTURE_2D, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_S, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_T, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_R, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_Q, GL_FALSE);
		VERIFYGLINT(GL_STENCIL_WRITEMASK, ~0)
		VERIFYGLINT2(GL_POLYGON_MODE, GL_FILL, GL_FILL)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_FILL, GL_FALSE)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_LINE, GL_FALSE)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_POINT, GL_FALSE)
		VERIFYGLBOOL(GL_LINE_STIPPLE, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE0, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE1, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE2, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE3, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE4, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE5, GL_FALSE)
		VERIFYGLFLOAT(GL_LINE_WIDTH, 1.0f)
		VERIFYGLFLOAT(GL_POINT_SIZE, 1.0f)
		VERIFYGLFLOAT4(GL_CURRENT_COLOR, 1.0f, 1.0f, 1.0f, 1.0f)
		VERIFYGLBOOL4(GL_COLOR_WRITEMASK, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)
		VERIFYGLBOOL(GL_DEPTH_WRITEMASK, GL_TRUE)
		VERIFYGLINT(GL_DEPTH_FUNC, GL_LEQUAL)
		VERIFYGLINT(GL_BLEND_SRC_RGB, GL_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_SRC_ALPHA, GL_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_DST_RGB, GL_ONE_MINUS_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
	}
#undef AREA
}

CGLStateChecker::~CGLStateChecker()
{
#define AREA "exiting"
	if (Threading::IsMainThread())
	{
		//VERIFYGLBOOL(GL_CULL_FACE, GL_FALSE)
		VERIFYGLBOOL(GL_ALPHA_TEST, GL_FALSE)
		VERIFYGLBOOL(GL_LIGHTING, GL_FALSE)
		VERIFYGLBOOL(GL_SCISSOR_TEST, GL_FALSE)
		VERIFYGLBOOL(GL_STENCIL_TEST, GL_FALSE)
		// VERIFYGLBOOL(GL_TEXTURE_2D, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_S, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_T, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_R, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_Q, GL_FALSE);
		VERIFYGLINT(GL_STENCIL_WRITEMASK, ~0)
		VERIFYGLINT2(GL_POLYGON_MODE, GL_FILL, GL_FILL)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_FILL, GL_FALSE)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_LINE, GL_FALSE)
		VERIFYGLBOOL(GL_POLYGON_OFFSET_POINT, GL_FALSE)
		VERIFYGLBOOL(GL_LINE_STIPPLE, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE0, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE1, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE2, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE3, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE4, GL_FALSE)
		VERIFYGLBOOL(GL_CLIP_PLANE5, GL_FALSE)
		VERIFYGLFLOAT(GL_LINE_WIDTH, 1.0f)
		VERIFYGLFLOAT(GL_POINT_SIZE, 1.0f)
		VERIFYGLFLOAT4(GL_CURRENT_COLOR, 1.0f, 1.0f, 1.0f, 1.0f)
		VERIFYGLBOOL4(GL_COLOR_WRITEMASK, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE)
		VERIFYGLBOOL(GL_DEPTH_WRITEMASK, GL_TRUE)
		VERIFYGLINT(GL_DEPTH_FUNC, GL_LEQUAL)
		VERIFYGLINT(GL_BLEND_SRC_RGB, GL_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_SRC_ALPHA, GL_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_DST_RGB, GL_ONE_MINUS_SRC_ALPHA)
		VERIFYGLINT(GL_BLEND_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
	}
#undef AREA
}

#endif //DEBUG_GLSTATE
