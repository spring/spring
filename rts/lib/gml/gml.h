// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used freely for any purpose, as
// long as this notice remains unchanged

#ifndef GML_H
#define GML_H

#ifdef USE_GML

#include <set>
#include <map>

#include "gmlcls.h"

extern gmlQueue gmlQueues[GML_MAX_NUM_THREADS];

#include "gmlfun.h"

extern boost::thread *gmlThreads[GML_MAX_NUM_THREADS];

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3) && (__GNUC_PATCHLEVEL__ == 0)
// gcc has issues with attributes in function pointers it seems

extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlProgramServer;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlProgramObjectARBServer;

extern gmlSingleItemServer<GLUquadric *, GLUquadric *(*)(void)> gmlQuadricServer;

extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei,GLuint *)> gmlTextureServer;

extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlBufferARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlFencesNVServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlProgramsARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlRenderbuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlFramebuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlQueryServer;
extern gmlMultiItemServer<GLuint, GLsizei, void (*)(GLsizei, GLuint*)> gmlBufferServer;

extern gmlItemSequenceServer<GLuint, GLsizei,GLuint (*)(GLsizei)> gmlListServer;

#else

extern gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMPROC *> gmlProgramServer;
extern gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMOBJECTARBPROC *> gmlProgramObjectARBServer;

extern gmlSingleItemServer<GLUquadric *, GLUquadric *(GML_GLAPIENTRY *)(void)> gmlQuadricServer;

extern gmlMultiItemServer<GLuint, GLsizei, void (GML_GLAPIENTRY *)(GLsizei,GLuint *)> gmlTextureServer;

extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSARBPROC *> gmlBufferARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFENCESNVPROC *> gmlFencesNVServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENPROGRAMSARBPROC *> gmlProgramsARBServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENRENDERBUFFERSEXTPROC *> gmlRenderbuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFRAMEBUFFERSEXTPROC *> gmlFramebuffersEXTServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENQUERIESPROC *> gmlQueryServer;
extern gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSPROC *> gmlBufferServer;

extern gmlItemSequenceServer<GLuint, GLsizei,GLuint (GML_GLAPIENTRY *)(GLsizei)> gmlListServer;

#endif

extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_VERTEX;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_FRAGMENT;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_GEOMETRY_EXT;

extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_VERTEX;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_FRAGMENT;
extern gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_GEOMETRY_EXT;

extern void gmlInit();

EXTERN inline GLhandleARB gmlCreateProgram() {
	GML_ITEMSERVER_CHECK();
	return gmlProgramServer.GetItems();
}
EXTERN inline GLhandleARB gmlCreateProgramObjectARB() {
	GML_ITEMSERVER_CHECK();
	return gmlProgramObjectARBServer.GetItems();
}
EXTERN inline GLhandleARB gmlCreateShader(GLenum type) {
	GML_ITEMSERVER_CHECK();
	if(type==GL_VERTEX_SHADER)
		return gmlShaderServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER)
		return gmlShaderServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLhandleARB gmlCreateShaderObjectARB(GLenum type) {
	GML_ITEMSERVER_CHECK();
	if(type==GL_VERTEX_SHADER_ARB)
		return gmlShaderObjectARBServer_VERTEX.GetItems();
	if(type==GL_FRAGMENT_SHADER_ARB)
		return gmlShaderObjectARBServer_FRAGMENT.GetItems();
	if(type==GL_GEOMETRY_SHADER_EXT)
		return gmlShaderObjectARBServer_GEOMETRY_EXT.GetItems();
	return 0;
}
EXTERN inline GLUquadric *gmluNewQuadric() {
	GML_ITEMSERVER_CHECK();
	return gmlQuadricServer.GetItems();
}


EXTERN inline void gmlGenTextures(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlTextureServer.GetItems(n, items);
}
EXTERN inline void gmlGenBuffersARB(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlBufferARBServer.GetItems(n, items);
}
EXTERN inline void gmlGenFencesNV(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlFencesNVServer.GetItems(n, items);
}
EXTERN inline void gmlGenProgramsARB(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlProgramsARBServer.GetItems(n, items);
}
EXTERN inline void gmlGenRenderbuffersEXT(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlRenderbuffersEXTServer.GetItems(n, items);
}
EXTERN inline void gmlGenFramebuffersEXT(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlFramebuffersEXTServer.GetItems(n, items);
}
EXTERN inline void gmlGenQueries(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlQueryServer.GetItems(n, items);
}
EXTERN inline void gmlGenBuffers(GLsizei n, GLuint *items) {
	GML_ITEMSERVER_CHECK();
	gmlBufferServer.GetItems(n, items);
}

EXTERN inline GLuint gmlGenLists(GLsizei items) {
	GML_ITEMSERVER_CHECK();
	return gmlListServer.GetItems(items);
}

#include "gmlimp.h"
#include "gmldef.h"

#define GML_VECTOR gmlVector
#define GML_CLASSVECTOR gmlClassVector

#if GML_ENABLE_SIMDRAW
#include <boost/thread/mutex.hpp>
extern boost::mutex caimutex;
extern boost::mutex decalmutex;
extern boost::mutex treemutex;
extern boost::mutex modelmutex;
extern boost::mutex texmutex;
extern boost::mutex mapmutex;
extern boost::mutex groupmutex;
extern boost::mutex inmapmutex;
extern boost::mutex tempmutex;
extern boost::mutex posmutex;
extern boost::mutex rendermutex;
extern boost::mutex simmutex;
extern boost::mutex netmutex;
extern boost::mutex histmutex;
extern boost::mutex logmutex;

#include <boost/thread/recursive_mutex.hpp>
extern boost::recursive_mutex unitmutex;
extern boost::recursive_mutex quadmutex;
extern boost::recursive_mutex selmutex;
extern boost::recursive_mutex &luamutex;
extern boost::recursive_mutex featmutex;
extern boost::recursive_mutex projmutex;
extern boost::recursive_mutex grassmutex;
extern boost::recursive_mutex guimutex;
extern boost::recursive_mutex filemutex;
extern boost::recursive_mutex &qnummutex;

#define GML_STDMUTEX_LOCK(name) boost::mutex::scoped_lock name##lock(name##mutex)
#define GML_RECMUTEX_LOCK(name) boost::recursive_mutex::scoped_lock name##lock(name##mutex)

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

#define GML_GET_TICKS(var) var=gmlGetTicks()
#define GML_UPDATE_TICKS() gmlUpdateTicks()

#define GML_PARG_H , boost::recursive_mutex::scoped_lock *projlock = &boost::recursive_mutex::scoped_lock(projmutex)
#define GML_PARG_C , boost::recursive_mutex::scoped_lock *projlock
#define GML_PARG_P , projlock

#else

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)

#define GML_GET_TICKS(var)
#define GML_UPDATE_TICKS()

#define GML_PARG_H
#define GML_PARG_C
#define GML_PARG_P

#endif

#else

#define GML_VECTOR std::vector
#define GML_CLASSVECTOR std::vector

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)

#define GML_GET_TICKS(var)
#define GML_UPDATE_TICKS()

#define GML_PARG_H
#define GML_PARG_C
#define GML_PARG_P

#endif // USE_GML

#endif
