/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(DEBUG_GLSTATE)

#define NO_GL_WRAP

#include <string>

#include "glStateDebug.h"

#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

static spring::unordered_map<std::string, std::string> lastSet;
static spring::unordered_set<std::string> errorsSet;

template<typename T, typename F>
static void VERIFYGL(F func, GLenum pname, T defaultValue, std::string pstr, std::string area) {
	if (errorsSet.find(area + pstr) == errorsSet.end()) {
		T _temp;
		func(pname, &_temp);
		if (_temp != defaultValue) {
			LOG_L(L_ERROR, "%s was not set correctly when %s: %f", pstr.c_str(), area.c_str(), (float) _temp);
			LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str());
			errorsSet.insert(area + pstr);
		}
	}
}

template<typename T, typename F>
static void VERIFYGL2(F func, GLenum pname, T default0, T default1, std::string pstr, std::string area) {
	if (errorsSet.find(area + pstr) == errorsSet.end()) {
		T _temp[4];
		func(pname, _temp);
		if (_temp[0] != default0 || _temp[1] != default1) {
			LOG_L(L_ERROR, "%s was not set correctly when %s: %f %f", pstr.c_str(), area.c_str(), (float) _temp[0], (float) _temp[1]);
			LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str());
			errorsSet.insert(area + pstr);
		}
	}
}

template<typename T, typename F>
static void VERIFYGL4(F func, GLenum pname, T default0, T default1, T default2, T default3, std::string pstr, std::string area) {
	if (errorsSet.find(area + pstr) == errorsSet.end()) {
		T _temp[4];
		func(pname, _temp);
		if (_temp[0] != default0 || _temp[1] != default1 || _temp[2] != default2 || _temp[3] != default3) {
			LOG_L(L_ERROR, "%s was not set correctly when %s: %f %f %f %f", pstr.c_str(), area.c_str(), (float) _temp[0], (float) _temp[1], (float) _temp[2], (float) _temp[3]);
			LOG_L(L_ERROR, "last time set here: %s", lastSet[pstr].c_str());
			errorsSet.insert(area + pstr);
		}
	}
}
	
#define VERIFYGLBOOL(pname, defaultValue, area) VERIFYGL(glGetBooleanv, pname, (GLboolean) defaultValue, #pname, area)
#define VERIFYGLBOOL4(pname, default0, default1, default2, default3, area) VERIFYGL4(glGetBooleanv, pname, (GLboolean) default0, (GLboolean) default1, (GLboolean) default2, (GLboolean) default3, #pname, area)

#define VERIFYGLFLOAT(pname, defaultValue, area) VERIFYGL(glGetFloatv, pname, (GLfloat) defaultValue, #pname, area)
#define VERIFYGLFLOAT4(pname, default0, default1, default2, default3, area) VERIFYGL4(glGetFloatv, pname, (GLfloat) default0, (GLfloat) default1, (GLfloat) default2, (GLfloat) default3, #pname, area)

#define VERIFYGLINT(pname, defaultValue, area) VERIFYGL(glGetIntegerv, pname, (GLint) defaultValue, #pname, area)
#define VERIFYGLINT2(pname, default0, default1, area) VERIFYGL2(glGetIntegerv, pname, (GLint) default0, (GLint) default1, #pname, area)

void _wrap_glEnable(GLenum pname, std::string pstr, std::string location)
{
	if (Threading::IsMainThread())
		lastSet[pstr] = location;

	glAttribStatePtr->Enable(pname);
}

void _wrap_glDisable(GLenum pname, std::string pstr, std::string location)
{
	if (Threading::IsMainThread())
		lastSet[pstr] = location;

	glAttribStatePtr->Disable(pname);
}

void _wrap_glBlendFunc(GLenum sfactor, GLenum dfactor, std::string location)
{
	if (Threading::IsMainThread()) {
		lastSet["GL_BLEND_SRC_RGB"] = location;
		lastSet["GL_BLEND_SRC_ALPHA"] = location;
		lastSet["GL_BLEND_DST_RGB"] = location;
		lastSet["GL_BLEND_DST_ALPHA"] = location;
	}
	glAttribStatePtr->BlendFunc(sfactor, dfactor);
}

void _wrap_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, std::string location)
{
	if (Threading::IsMainThread()) {
		lastSet["GL_BLEND_SRC_RGB"] = location;
		lastSet["GL_BLEND_SRC_ALPHA"] = location;
		lastSet["GL_BLEND_DST_RGB"] = location;
		lastSet["GL_BLEND_DST_ALPHA"] = location;
	}
	glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void _wrap_glColor3f(GLfloat red, GLfloat green, GLfloat blue, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_CURRENT_COLOR"] = location;

	glColor3f(red, green, blue);
}

void _wrap_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_CURRENT_COLOR"] = location;

	glColor4f(red, green, blue, alpha);
}

void _wrap_glColor4fv(const GLfloat *v, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_CURRENT_COLOR"] = location;

	glColor4fv(v);
}

void _wrap_glDepthMask(GLboolean flag, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_DEPTH_WRITEMASK"] = location;

	glAttribStatePtr->DepthMask(flag);
}


void _wrap_glDepthFunc(GLenum func, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_DEPTH_FUNC"] = location;

	glAttribStatePtr->DepthFunc(func);
}

void _wrap_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, std::string location)
{
	if (Threading::IsMainThread())
		lastSet["GL_COLOR_WRITEMASK"] = location;

	glAttribStatePtr->ColorMask(red, green, blue, alpha);
}

void CGLStateChecker::VerifyState(std::string area) {
	if (Threading::IsMainThread()) {
		std::string _area = area + " " + id;
		//VERIFYGLBOOL(GL_CULL_FACE, GL_FALSE)
		VERIFYGLBOOL(GL_ALPHA_TEST, GL_FALSE, _area);
		VERIFYGLBOOL(GL_SCISSOR_TEST, GL_FALSE, _area);
		VERIFYGLBOOL(GL_STENCIL_TEST, GL_FALSE, _area);
		// VERIFYGLBOOL(GL_TEXTURE_2D, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_S, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_T, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_R, GL_FALSE);
		// VERIFYGLBOOL(GL_TEXTURE_GEN_Q, GL_FALSE);
		VERIFYGLINT(GL_STENCIL_WRITEMASK, ~0, _area);
		VERIFYGLINT2(GL_POLYGON_MODE, GL_FILL, GL_FILL, _area);
		VERIFYGLBOOL(GL_POLYGON_OFFSET_FILL, GL_FALSE, _area);
		VERIFYGLBOOL(GL_POLYGON_OFFSET_LINE, GL_FALSE, _area);
		VERIFYGLBOOL(GL_POLYGON_OFFSET_POINT, GL_FALSE, _area);
		VERIFYGLBOOL(GL_LINE_STIPPLE, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE0, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE1, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE2, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE3, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE4, GL_FALSE, _area);
		VERIFYGLBOOL(GL_CLIP_PLANE5, GL_FALSE, _area);
		VERIFYGLFLOAT(GL_LINE_WIDTH, 1.0f, _area);
		VERIFYGLFLOAT(GL_POINT_SIZE, 1.0f, _area);
		VERIFYGLFLOAT4(GL_CURRENT_COLOR, 1.0f, 1.0f, 1.0f, 1.0f, _area);
		VERIFYGLBOOL4(GL_COLOR_WRITEMASK, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, _area);
		VERIFYGLBOOL(GL_DEPTH_WRITEMASK, GL_TRUE, _area);
		VERIFYGLINT(GL_DEPTH_FUNC, GL_LEQUAL, _area);
		VERIFYGLINT(GL_BLEND_SRC_RGB, GL_SRC_ALPHA, _area);
		VERIFYGLINT(GL_BLEND_SRC_ALPHA, GL_SRC_ALPHA, _area);
		VERIFYGLINT(GL_BLEND_DST_RGB, GL_ONE_MINUS_SRC_ALPHA, _area);
		VERIFYGLINT(GL_BLEND_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, _area);
	}
}

CGLStateChecker::CGLStateChecker(std::string id) : id(id)
{
	VerifyState("entering");
}

CGLStateChecker::~CGLStateChecker()
{
	VerifyState("exiting");
}

#endif //DEBUG_GLSTATE
