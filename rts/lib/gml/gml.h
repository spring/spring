// GML - OpenGL Multithreading Library
// for Spring http://springrts.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

#ifndef GML_H
#define GML_H

#if defined USE_GML_SIM && !defined USE_GML
#error USE_GML_SIM requires USE_GML
#endif

#include "gmlcnf.h"

#define GML_MUTEX_PROFILER 0 // enables mutex profiler

#ifdef USE_GML

#define GML_MUTEX_PROFILE 0 // detailed profiling of specific mutex
extern const char *gmlProfMutex;
#define GML_PROC_PROFILER 0 // enables gmlprocessor profiler

#include <set>
#include <map>

#include <GL/glew.h>
#include "gmlcls.h"
#include "gmlque.h"

extern gmlQueue gmlQueues[GML_MAX_NUM_THREADS];

#include "gmlfun.h"

#if GML_PROC_PROFILER
	extern int gmlProcNumLoop;
	extern int gmlProcInterval;
	#define GML_PROFILER(name) \
	GML::Enabled() && name && (globalRendering->drawFrame & gmlProcInterval);\
	SCOPED_TIMER(!name ? "NoProc" : ((name && (globalRendering->drawFrame & gmlProcInterval)) ? " " GML_QUOTE(name) "MTProc" : " " GML_QUOTE(name) "Proc"));\
	for(int i = 0; i < (name ? gmlProcNumLoop : 1); ++i)
#else
	#define GML_PROFILER(name) (GML::Enabled() && name);
#endif

extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_VERTEX;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_FRAGMENT;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_GEOMETRY_EXT;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_VERTEX;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_FRAGMENT;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_GEOMETRY_EXT;
extern gmlSingleItemServer<GLUquadric *, GLUquadric *(GML_GLAPIENTRY *)(void)> gmlQuadricServer;

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ == 0)
// gcc has issues with attributes in function pointers it seems
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (**)(void)> gmlProgramServer;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (**)(void)> gmlProgramObjectARBServer;

extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlBufferARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlFencesNVServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlProgramsARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlRenderbuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlFramebuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlQueryServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (**)(GLsizei, GLuint *)> gmlBufferServer;
#else
extern gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMPROC *> gmlProgramServer;
extern gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMOBJECTARBPROC *> gmlProgramObjectARBServer;

extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSARBPROC *> gmlBufferARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFENCESNVPROC *> gmlFencesNVServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENPROGRAMSARBPROC *> gmlProgramsARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENRENDERBUFFERSEXTPROC *> gmlRenderbuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFRAMEBUFFERSEXTPROC *> gmlFramebuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENQUERIESPROC *> gmlQueryServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSPROC *> gmlBufferServer;
#endif

extern gmlMultiItemServer<GLuint, GLsizei, void (GML_GLAPIENTRY *)(GLsizei, GLuint *)> gmlTextureServer;

extern gmlItemSequenceServer<GLuint, GLsizei,GLuint (GML_GLAPIENTRY *)(GLsizei)> gmlListServer;

extern void gmlInit();

#define GML_IF_NONCLIENT_THREAD(name,...)\
	int threadnum = gmlThreadNumber;\
	GML_IF_SHARE_THREAD(threadnum) {\
		name(__VA_ARGS__);\
		return;\
	}\
	GML_ITEMSERVER_CHECK(threadnum);

#define GML_IF_NONCLIENT_THREAD_RET(ret,name,...)\
	int threadnum = gmlThreadNumber;\
	GML_IF_SHARE_THREAD(threadnum) {\
		return name(__VA_ARGS__);\
	}\
	GML_ITEMSERVER_CHECK_RET(threadnum,ret);

EXTERN inline GLhandleARB GML_GLAPIENTRY gmlCreateProgram() {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateProgram);
	return gmlProgramServer.GetItems();
}
EXTERN inline GLhandleARB GML_GLAPIENTRY gmlCreateProgramObjectARB() {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateProgramObjectARB);
	return gmlProgramObjectARBServer.GetItems();
}
EXTERN inline GLhandleARB GML_GLAPIENTRY gmlCreateShader(GLenum type) {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateShader,type);
	if(type==GL_VERTEX_SHADER)
		return gmlShaderServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER)
		return gmlShaderServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLhandleARB GML_GLAPIENTRY gmlCreateShaderObjectARB(GLenum type) {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateShaderObjectARB,type);
	if(type==GL_VERTEX_SHADER_ARB)
		return gmlShaderObjectARBServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER_ARB)
		return gmlShaderObjectARBServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderObjectARBServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLUquadric *GML_GLAPIENTRY gmluNewQuadric() {
	GML_IF_NONCLIENT_THREAD_RET(GLUquadric *,gluNewQuadric);
	return gmlQuadricServer.GetItems();
}

EXTERN inline void GML_GLAPIENTRY gmlGenTextures(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenTextures,n,items);
	gmlTextureServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenBuffersARB(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenBuffersARB,n,items);
	gmlBufferARBServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenFencesNV(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenFencesNV,n,items);
	gmlFencesNVServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenProgramsARB(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenProgramsARB,n,items);
	gmlProgramsARBServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenRenderbuffersEXT(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenRenderbuffersEXT,n,items);
	gmlRenderbuffersEXTServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenFramebuffersEXT(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenFramebuffersEXT,n,items);
	gmlFramebuffersEXTServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenQueries(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenQueries,n,items);
	gmlQueryServer.GetItems(n, items);
}
EXTERN inline void GML_GLAPIENTRY gmlGenBuffers(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenBuffers,n,items);
	gmlBufferServer.GetItems(n, items);
}

EXTERN inline GLuint GML_GLAPIENTRY gmlGenLists(GLsizei items) {
	GML_IF_NONCLIENT_THREAD_RET(GLuint,glGenLists,items);
	return gmlListServer.GetItems(items);
}

#include "gmlimp.h"
#include "gmldef.h"

#define GML_VECTOR gmlVector
#define GML_CLASSVECTOR gmlClassVector

#include "gmlmut.h"

#if GML_ENABLE_SIM

#if GML_CALL_DEBUG
#define GML_EXPGEN_CHECK() \
	if(gmlThreadNumber != GML_SIM_THREAD_NUM && gmlMultiThreadSim && gmlStartSim) {\
		lua_State *currentLuaState = gmlCurrentLuaStates[gmlThreadNumber];\
		LOG_SL("Threading", L_ERROR, "Draw thread created ExpGenSpawnable (%s)", GML_CURRENT_LUA(currentLuaState));\
		if(currentLuaState) luaL_error(currentLuaState,"Invalid call");\
	}
#define GML_CALL_DEBUGGER() gmlCallDebugger gmlCDBG(L);
#define GML_LOCK_TIME() (gmlCallDebugger::getLockTime())
#else
#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_LOCK_TIME() 0
#endif

#else

#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_LOCK_TIME() 0

#endif

#else

#define GML_VECTOR std::vector
#define GML_CLASSVECTOR std::vector

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)
#define GML_THRMUTEX_LOCK(name,thr)
#define GML_OBJMUTEX_LOCK(name,thr,...)
#define GML_STDMUTEX_LOCK_NOPROF(name)
#define GML_MSTMUTEX_LOCK(name,...)
#define GML_MSTMUTEX_DOLOCK(name)
#define GML_MSTMUTEX_DOUNLOCK(name)

#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_LOCK_TIME() 0

#endif // USE_GML

#include "gml_base.h"

#endif
