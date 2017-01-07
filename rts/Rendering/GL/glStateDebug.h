/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GL_STATE_CHECKER_H
#define _GL_STATE_CHECKER_H

#ifndef DEBUG_GLSTATE
#define GL_STATE_CHECKER(name)
#else

#include <string>
	
#include "Rendering/GL/myGL.h"
// GL_STATE_CHECKER(name) verifies the GL state has default values when entering/exiting its scope.
// If something isn't set correctly, it logs an error with a mention of when the value was last set (TODO: deal with lists)
#define GL_STATE_CHECKER(name) CGLStateChecker __gl_state_checker(name)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FILEPOS __FILE__ ":" TOSTRING(__LINE__)


#ifndef NO_GL_WRAP

	#define glEnable(i) _wrap_glEnable(i, #i, FILEPOS)
	#define glDisable(i) _wrap_glDisable(i, #i, FILEPOS)

	#ifdef glBlendFunc
		#undef glBlendFunc
	#endif

	#ifdef glBlendFuncSeparate
		#undef glBlendFuncSeparate
	#endif
	
	#ifdef glColor4f
		#undef glColor4f
	#endif
	
	#ifdef glColor3f
		#undef glColor3f
	#endif
	
	#ifdef glColor4fv
		#undef glColor4fv
	#endif
	
	#ifdef glDepthMask
		#undef glDepthMask
	#endif

	#define glBlendFunc(sfactor, dfactor) _wrap_glBlendFunc(sfactor, dfactor, FILEPOS)
	#define glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha) _wrap_glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha, FILEPOS)
	#define glColor3f(r, g, b) _wrap_glColor3f(r, g, b, FILEPOS)
	#define glColor4f(r, g, b, a) _wrap_glColor4f(r, g, b, a, FILEPOS)
	#define glColor4fv(v) _wrap_glColor4fv(v, FILEPOS)
	#define glDepthMask(flag) _wrap_glDepthMask(flag, FILEPOS)
	#define glDepthFunc(func) _wrap_glDepthFunc(func, FILEPOS)
	#define glColorMask(r, g, b, a) _wrap_glColorMask(r, g, b, a, FILEPOS)
#endif


extern void _wrap_glEnable(GLenum pname, std::string pstr, std::string location);

extern void _wrap_glDisable(GLenum pname, std::string pstr, std::string location);

extern void _wrap_glBlendFunc(GLenum sfactor, GLenum dfactor, std::string location);

extern void _wrap_glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha, std::string location);

extern void _wrap_glColor3f(GLfloat red, GLfloat green, GLfloat blue, std::string location);

extern void _wrap_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha, std::string location);

extern void _wrap_glColor4fv(const GLfloat *v, std::string location);

extern void _wrap_glDepthMask(GLboolean flag, std::string location);

extern void _wrap_glDepthFunc(GLenum func, std::string location);

extern void _wrap_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha, std::string location);

class CGLStateChecker
{
public:
	CGLStateChecker(std::string id);
	~CGLStateChecker();
	const std::string id;

private:
	void VerifyState(std::string area);
};

#endif // DEBUG_GLSTATE

#endif // _GL_STATE_CHECKER_H
