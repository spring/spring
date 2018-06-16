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

static unsigned int glPushDepth = 0;
static spring::unordered_map<GLenum, std::vector<GLboolean>> flagStates;
static std::vector<GLenum4> blendFuncs;
static std::vector<GLenum> depthFuncs;
static std::vector<GLboolean4> colorMasks;

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

	GLboolean state;
	glGetBooleanv(pname, &state);
	LOG_L(L_DEBUG, "[%s]: %s (%u) changed from %u to %u", location.c_str(), pstr.c_str(), pname, state, true);

	if (Threading::IsMainThread()) {
		lastSet[pstr] = location;
		if (glPushDepth > 0) {
			while (flagStates[pname].size() < glPushDepth - 1)
				flagStates[pname].push_back(state);
			flagStates[pname].push_back(state);
		}
		else {
			std::vector<GLboolean> states;
			states.push_back(state);
			flagStates[pname] = states;
		}
	}

	glEnable(pname);
}

void _wrap_glDisable(GLenum pname, std::string pstr, std::string location)
{
	GLboolean state;
	glGetBooleanv(pname, &state);
	LOG_L(L_DEBUG, "[%s]: %s (%u) changed from %u to %u", location.c_str(), pstr.c_str(), pname, state, true);

	if (Threading::IsMainThread()) {
		lastSet[pstr] = location;
		if (glPushDepth > 0) {
			while (flagStates[pname].size() < glPushDepth - 1)
				flagStates[pname].push_back(state);
			flagStates[pname].push_back(state);
		}
	}

	glDisable(pname);
}

void _wrap_glBlendFunc(GLenum sfactor, GLenum dfactor, std::string location)
{
	GLint srgb, drgb, sa, da;
	glGetIntegerv(GL_BLEND_SRC_RGB, &srgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &sa);
	glGetIntegerv(GL_BLEND_DST_RGB, &drgb);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &da);
	LOG_L(L_DEBUG,
		  "[%s]: (GL_BLEND_SRC_RGB, GL_BLEND_SRC_ALPHA, GL_BLEND_DST_RGB, GL_BLEND_DST_ALPHA) changed from (0x%04x, 0x%04x, 0x%04x, 0x%04x) to (0x%04x, 0x%04x, 0x%04x, 0x%04x)",
		  location.c_str(), srgb, sa, drgb, da, sfactor, sfactor, dfactor, dfactor);

	if (Threading::IsMainThread()) {
		lastSet["GL_BLEND_SRC_RGB"] = location;
		lastSet["GL_BLEND_SRC_ALPHA"] = location;
		lastSet["GL_BLEND_DST_RGB"] = location;
		lastSet["GL_BLEND_DST_ALPHA"] = location;
		if (glPushDepth > 0) {
			GLenum4 funcs;
			funcs.x = srgb;
			funcs.y = drgb;
			funcs.z = sa;
			funcs.w = da;
			while (blendFuncs.size() <= glPushDepth - 1) {
				blendFuncs.push_back(funcs);
			}
			blendFuncs.push_back(funcs);
		}
	}

	glBlendFunc(sfactor, dfactor);
}

void _wrap_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, std::string location)
{
	GLint srgb, drgb, sa, da;
	glGetIntegerv(GL_BLEND_SRC_RGB, &srgb);
	glGetIntegerv(GL_BLEND_SRC_ALPHA, &sa);
	glGetIntegerv(GL_BLEND_DST_RGB, &drgb);
	glGetIntegerv(GL_BLEND_DST_ALPHA, &da);
	LOG_L(L_DEBUG,
		  "[%s]: (GL_BLEND_SRC_RGB, GL_BLEND_SRC_ALPHA, GL_BLEND_DST_RGB, GL_BLEND_DST_ALPHA) changed from (0x%04x, 0x%04x, 0x%04x, 0x%04x) to (0x%04x, 0x%04x, 0x%04x, 0x%04x)",
		  location.c_str(), srgb, sa, drgb, da, srcRGB, srcAlpha, dstRGB, dstAlpha);

	if (Threading::IsMainThread()) {
		lastSet["GL_BLEND_SRC_RGB"] = location;
		lastSet["GL_BLEND_SRC_ALPHA"] = location;
		lastSet["GL_BLEND_DST_RGB"] = location;
		lastSet["GL_BLEND_DST_ALPHA"] = location;
		if (glPushDepth > 0) {
			GLenum4 funcs;
			funcs.x = srgb;
			funcs.y = drgb;
			funcs.z = sa;
			funcs.w = da;
			while (blendFuncs.size() <= glPushDepth - 1) {
				blendFuncs.push_back(funcs);
			}
			blendFuncs.push_back(funcs);
		}
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
	GLboolean state;
	glGetBooleanv(GL_DEPTH_WRITEMASK, &state);
	LOG_L(L_DEBUG, "[%s]: GL_DEPTH_WRITEMASK changed from %u to %u", location.c_str(), state, flag);

	if (Threading::IsMainThread()) {
		lastSet["GL_DEPTH_WRITEMASK"] = location;
		if (glPushDepth > 0) {
			while (flagStates[GL_DEPTH_WRITEMASK].size() < glPushDepth - 1)
				flagStates[GL_DEPTH_WRITEMASK].push_back(state);
			flagStates[GL_DEPTH_WRITEMASK].push_back(state);
		}
		else {
			std::vector<GLboolean> states;
			states.push_back(state);
			flagStates[GL_DEPTH_WRITEMASK] = states;
		}
	}

	glDepthMask(flag);
}


void _wrap_glDepthFunc(GLenum func, std::string location)
{
	GLint depthFunc;
	glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
	LOG_L(L_DEBUG, "[%s]: GL_DEPTH_FUNC changed from 0x%04x to 0x%04x", location.c_str(), depthFunc, func);

	if (Threading::IsMainThread()) {
		lastSet["GL_DEPTH_FUNC"] = location;
		if (glPushDepth > 0) {
			while (depthFuncs.size() <= glPushDepth - 1) {
				depthFuncs.push_back((GLenum)(depthFunc));
			}
			depthFuncs.push_back((GLenum)(depthFunc));
		}
	}

	glDepthFunc(func);
}

void _wrap_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, std::string location)
{
	GLboolean cmask[4];
	glGetBooleanv(GL_COLOR_WRITEMASK, cmask);
	LOG_L(L_DEBUG,
		  "[%s]: GL_COLOR_WRITEMASK changed from (%u, %u, %u, %u) to (%u, %u, %u, %u)",
		  location.c_str(), cmask[0], cmask[1], cmask[2], cmask[3], red, green, blue, alpha);

	if (Threading::IsMainThread()) {
		lastSet["GL_COLOR_WRITEMASK"] = location;
		if (glPushDepth > 0) {
			GLboolean4 mask;
			mask.x = cmask[0];
			mask.y = cmask[1];
			mask.z = cmask[2];
			mask.w = cmask[3];
			while (colorMasks.size() <= glPushDepth - 1) {
				colorMasks.push_back(mask);
			}
			colorMasks.push_back(mask);
		}
	}

	glColorMask(red, green, blue, alpha);
}

void _wrap_glPushAttrib(GLenum cap, std::string location)
{
	LOG_L(L_ERROR, "[%s]: glPushAttrib is a deprecated function!", location.c_str());
	if (Threading::IsMainThread()) {
		// Extend all the already pushed variables
		if(glPushDepth > 0) {
			for ( auto it = flagStates.begin(); it != flagStates.end(); ++it )
				it->second.push_back(it->second.back());
			if (blendFuncs.size() > 0)
				blendFuncs.push_back(blendFuncs.back());
			if (depthFuncs.size() > 0)
				depthFuncs.push_back(depthFuncs.back());
			if (colorMasks.size() > 0)
				colorMasks.push_back(colorMasks.back());
		}
		glPushDepth += 1;
		LOG_L(L_DEBUG, "[%s]: glPushAttrib depth = %u", location.c_str(), glPushDepth);
	}
	else {
		LOG_L(L_ERROR, "[%s]: Non-thread-safe glPushAttrib call!", location.c_str());
	}

	// glPushAttrib(cap);
}

void _wrap_glPopAttrib(std::string location)
{
	LOG_L(L_ERROR, "[%s]: glPopAttrib is a deprecated function!", location.c_str());
	if (Threading::IsMainThread()) {
		if (glPushDepth == 0) {
			LOG_L(L_ERROR, "[%s]: glPushAttrib depth = %u while calling glPopAttrib!", location.c_str(), glPushDepth);
		}
		else {
			// Restore the pushed values
			for ( auto it = flagStates.begin(); it != flagStates.end(); ++it ) {
				GLboolean state = it->second.back();
				it->second.pop_back();
				if (it->first == GL_DEPTH_WRITEMASK) {
					glDepthMask(state);
				}
				else {
					if (state) {
						glEnable(it->first);
					}
					else {
						glDisable(it->first);
					}
				}
			}
			if (blendFuncs.size() > 0) {
				GLenum4 funcs = blendFuncs.back();
				blendFuncs.pop_back();
				glBlendFuncSeparate(funcs.x, funcs.y, funcs.z, funcs.w);
			}
			if (depthFuncs.size() > 0) {
				GLenum func = depthFuncs.back();
				depthFuncs.pop_back();
				glDepthFunc(func);
			}
			if (colorMasks.size() > 0) {
				GLboolean4 mask = colorMasks.back();
				colorMasks.pop_back();
				glColorMask(mask.x, mask.y, mask.z, mask.w);
			}

			glPushDepth -= 1;
			LOG_L(L_DEBUG, "[%s]: glPushAttrib depth = %u", location.c_str(), glPushDepth);

			if (glPushDepth == 0) {
				spring::clear_unordered_map(flagStates);
				blendFuncs.clear();
				depthFuncs.clear();
				colorMasks.clear();
			}
		}
	}
	else {
		LOG_L(L_ERROR, "[%s]: Non-thread-safe glPopAttrib call!", location.c_str());
	}

	// glPopAttrib();
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
