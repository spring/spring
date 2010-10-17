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

#define GML_MUTEX_PROFILER 0 // enables profiler

#ifdef USE_GML
#define GML_MUTEX_PROFILE 0 // detailed profiling of specific mutex
extern const char *gmlProfMutex;

#include <set>
#include <map>

#include "gmlcls.h"

extern gmlQueue gmlQueues[GML_MAX_NUM_THREADS];

#include "gmlfun.h"

extern boost::thread *gmlThreads[GML_MAX_NUM_THREADS];

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
	GML_IF_SERVER_THREAD(threadnum) {\
		name(__VA_ARGS__);\
		return;\
	}\
	GML_ITEMSERVER_CHECK(threadnum);

#define GML_IF_NONCLIENT_THREAD_RET(ret,name,...)\
	int threadnum = gmlThreadNumber;\
	GML_IF_SERVER_THREAD(threadnum) {\
		return name(__VA_ARGS__);\
	}\
	GML_ITEMSERVER_CHECK_RET(threadnum,ret);

EXTERN inline GLhandleARB gmlCreateProgram() {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateProgram);
	return gmlProgramServer.GetItems();
}
EXTERN inline GLhandleARB gmlCreateProgramObjectARB() {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateProgramObjectARB);
	return gmlProgramObjectARBServer.GetItems();
}
EXTERN inline GLhandleARB gmlCreateShader(GLenum type) {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateShader,type);
	if(type==GL_VERTEX_SHADER)
		return gmlShaderServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER)
		return gmlShaderServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLhandleARB gmlCreateShaderObjectARB(GLenum type) {
	GML_IF_NONCLIENT_THREAD_RET(GLhandleARB,glCreateShaderObjectARB,type);
	if(type==GL_VERTEX_SHADER_ARB)
		return gmlShaderObjectARBServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER_ARB)
		return gmlShaderObjectARBServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderObjectARBServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLUquadric *gmluNewQuadric() {
	GML_IF_NONCLIENT_THREAD_RET(GLUquadric *,gluNewQuadric);
	return gmlQuadricServer.GetItems();
}

EXTERN inline void gmlGenTextures(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenTextures,n,items);
	gmlTextureServer.GetItems(n, items);
}
EXTERN inline void gmlGenBuffersARB(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenBuffersARB,n,items);
	gmlBufferARBServer.GetItems(n, items);
}
EXTERN inline void gmlGenFencesNV(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenFencesNV,n,items);
	gmlFencesNVServer.GetItems(n, items);
}
EXTERN inline void gmlGenProgramsARB(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenProgramsARB,n,items);
	gmlProgramsARBServer.GetItems(n, items);
}
EXTERN inline void gmlGenRenderbuffersEXT(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenRenderbuffersEXT,n,items);
	gmlRenderbuffersEXTServer.GetItems(n, items);
}
EXTERN inline void gmlGenFramebuffersEXT(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenFramebuffersEXT,n,items);
	gmlFramebuffersEXTServer.GetItems(n, items);
}
EXTERN inline void gmlGenQueries(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenQueries,n,items);
	gmlQueryServer.GetItems(n, items);
}
EXTERN inline void gmlGenBuffers(GLsizei n, GLuint *items) {
	GML_IF_NONCLIENT_THREAD(glGenBuffers,n,items);
	gmlBufferServer.GetItems(n, items);
}

EXTERN inline GLuint gmlGenLists(GLsizei items) {
	GML_IF_NONCLIENT_THREAD_RET(GLuint,glGenLists,items);
	return gmlListServer.GetItems(items);
}

#include "gmlimp.h"
#include "gmldef.h"

#define GML_VECTOR gmlVector
#define GML_CLASSVECTOR gmlClassVector

#if GML_ENABLE_SIM
#include <boost/thread/mutex.hpp>
extern boost::mutex caimutex;
extern boost::mutex decalmutex;
extern boost::mutex treemutex;
extern boost::mutex modelmutex;
extern boost::mutex mapmutex;
extern boost::mutex inmapmutex;
extern boost::mutex tempmutex;
extern boost::mutex posmutex;
extern boost::mutex runitmutex;
extern boost::mutex netmutex;
extern boost::mutex histmutex;
extern boost::mutex logmutex;
extern boost::mutex timemutex;
extern boost::mutex watermutex;
extern boost::mutex dquemutex;
extern boost::mutex scarmutex;
extern boost::mutex trackmutex;
extern boost::mutex projmutex;
extern boost::mutex rprojmutex;
extern boost::mutex rflashmutex;
extern boost::mutex rpiecemutex;
extern boost::mutex rfeatmutex;
extern boost::mutex drawmutex;

#include <boost/thread/recursive_mutex.hpp>
extern boost::recursive_mutex unitmutex;
extern boost::recursive_mutex quadmutex;
extern boost::recursive_mutex selmutex;
extern boost::recursive_mutex &luamutex;
extern boost::recursive_mutex featmutex;
extern boost::recursive_mutex grassmutex;
extern boost::recursive_mutex &guimutex;
extern boost::recursive_mutex filemutex;
extern boost::recursive_mutex &qnummutex;
extern boost::recursive_mutex &groupmutex;
extern boost::recursive_mutex &grpselmutex;
extern boost::recursive_mutex laycmdmutex;

extern gmlMutex simmutex;

#if GML_MUTEX_PROFILER
#	include "System/TimeProfiler.h"
#	if GML_MUTEX_PROFILE
#		ifdef _DEBUG
#			define GML_MTXCMP(a,b) !strcmp(a,b) // comparison of static strings using addresses may not work in debug mode
#		else
#			define GML_MTXCMP(a,b) (a==b)
#		endif
#		define GML_LINEMUTEX_LOCK(name, type, line)\
			char st##name[sizeof(ScopedTimer)];\
			int stc##name=GML_MTXCMP(GML_QUOTE(name),gmlProfMutex);\
			if(stc##name)\
				new (st##name) ScopedTimer(GML_QUOTE(name ## line ## Mutex));\
			boost::type::scoped_lock name##lock(name##mutex);\
			if(stc##name)\
				((ScopedTimer *)st##name)->~ScopedTimer()
#		define GML_PROFMUTEX_LOCK(name, type, line) GML_LINEMUTEX_LOCK(name, type, line)
#		define GML_STDMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, mutex, __LINE__)
#		define GML_RECMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, recursive_mutex, __LINE__)
#	else
#		define GML_PROFMUTEX_LOCK(name, type)\
			char st##name[sizeof(ScopedTimer)];\
			new (st##name) ScopedTimer(GML_QUOTE(name##Mutex));\
			boost::type::scoped_lock name##lock(name##mutex);\
			((ScopedTimer *)st##name)->~ScopedTimer()
#		define GML_STDMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, mutex)
#		define GML_RECMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, recursive_mutex)
#	endif
#else
#	define GML_STDMUTEX_LOCK(name) boost::mutex::scoped_lock name##lock(name##mutex)
#	define GML_RECMUTEX_LOCK(name) boost::recursive_mutex::scoped_lock name##lock(name##mutex)
#endif
class gmlMutexLock {
	gmlMutex &mtx;
public:
	gmlMutexLock(gmlMutex &m) : mtx(m) { mtx.Lock(); }
	virtual ~gmlMutexLock() { mtx.Unlock(); }
};
#define GML_MSTMUTEX_LOCK(name) gmlMutexLock name##mutexlock(name##mutex)
#define GML_MSTMUTEX_DOLOCK(name) name##mutex.Lock()
#define GML_MSTMUTEX_DOUNLOCK(name) name##mutex.Unlock()
#define GML_STDMUTEX_LOCK_NOPROF(name) boost::mutex::scoped_lock name##lock(name##mutex)
extern int gmlNextTickUpdate;
extern unsigned gmlCurrentTicks;

#include <SDL_timer.h>

inline unsigned gmlUpdateTicks() {
	gmlNextTickUpdate = 100;
	return gmlCurrentTicks=SDL_GetTicks();
}

inline unsigned gmlGetTicks() {
	if(--gmlNextTickUpdate > 0)
		return gmlCurrentTicks;
	return gmlUpdateTicks();
}

#if GML_CALL_DEBUG
#define GML_EXPGEN_CHECK() \
	extern volatile int gmlMultiThreadSim, gmlStartSim;\
	if(gmlThreadNumber != GML_SIM_THREAD_NUM && gmlMultiThreadSim && gmlStartSim) {\
		logOutput.Print("GML error: Draw thread created ExpGenSpawnable (%s)", GML_CURRENT_LUA());\
		if(gmlCurrentLuaState) luaL_error(gmlCurrentLuaState,"Invalid call");\
	}
#define GML_CALL_DEBUGGER() gmlCallDebugger gmlCDBG(L);
#define GML_DRAW_CALLIN_TIME() (gmlCallDebugger::getDrawCallInTime())
#else
#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_DRAW_CALLIN_TIME() 0
#endif

#define GML_GET_TICKS(var) var=gmlGetTicks()
#define GML_UPDATE_TICKS() gmlUpdateTicks()

#else

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)
#define GML_STDMUTEX_LOCK_NOPROF(name)
#define GML_MSTMUTEX_LOCK(name)
#define GML_MSTMUTEX_DOLOCK(name)
#define GML_MSTMUTEX_DOUNLOCK(name)

#define GML_GET_TICKS(var)
#define GML_UPDATE_TICKS()

#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_DRAW_CALLIN_TIME() 0

#endif

#else

#define GML_VECTOR std::vector
#define GML_CLASSVECTOR std::vector

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)
#define GML_STDMUTEX_LOCK_NOPROF(name)
#define GML_MSTMUTEX_LOCK(name)
#define GML_MSTMUTEX_DOLOCK(name)
#define GML_MSTMUTEX_DOUNLOCK(name)

#define GML_GET_TICKS(var)
#define GML_UPDATE_TICKS()

#define GML_EXPGEN_CHECK()
#define GML_CALL_DEBUGGER()
#define GML_DRAW_CALLIN_TIME() 0

#endif // USE_GML

#ifdef    HEADLESS
#define glGenerateMipmapEXT_NONGML NULL
#define glUseProgram_NONGML NULL
#define glProgramParameteriEXT_NONGML NULL
#define glBlendEquation_NONGML NULL
#else  // HEADLESS
#define glGenerateMipmapEXT_NONGML GLEW_GET_FUN(__glewGenerateMipmapEXT)
#define glUseProgram_NONGML GLEW_GET_FUN(__glewUseProgram)
#define glProgramParameteriEXT_NONGML GLEW_GET_FUN(__glewProgramParameteriEXT)
#define glBlendEquation_NONGML GLEW_GET_FUN(__glewBlendEquation)
#endif // HEADLESS

#endif
