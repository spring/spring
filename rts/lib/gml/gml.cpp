// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used freely for any purpose, as
// long as this notice remains unchanged

// GML works by "patching" all OpenGL calls. It is injected via a #include "gml.h" statement located in myGL.h.
// All files that need GL should therefore include myGL.h. INCLUDING gl.h, glu.h, glext.h ... IS FORBIDDEN.
// When a client thread (gmlThreadNumber > 0) executes a GL call, it is redirected into a queue.
// The server thread (gmlThreadNumber = 0) will then consume GL calls from the queues of each thread.
// When the server thread makes a GL call, it calls directly into OpenGL of course.

// Since a single server thread makes all GL calls, there is no point in multithreading code that contains
// lots of GL calls but almost no CPU intensive calculations. Also, there is no point in multithreading
// functions that take very short execution time to complete. The overhead of starting and managing the threads
// will defeat any possible performance benefit.

// Certain calls need synchronization. For example, all glGet*** functions return a value that must
// be available to the thread immediately. The client thread works around this by requesting the
// server to run in synced mode. This means the client will halt and ask the server to begin consuming
// GL calls from its queue until it reaches the sync point containing the GL call that returns the value.
// Running synced is expensive performance wise, so glGet*** etc should be avoided at all cost in threaded code.

// Instructions for adding new GL functions to GML:
// 1. add the   GML_MAKEFUN***(Function, ...)   statement to the long list of declarations at the end of gmlfun.h
// 2. add the corresponding   GML_MAKEHANDLER***(Function)   statment to the QueueHandler function below
// 3. add   #undef glFunction   to the list in the upper half of gmldef.h
// 4. add   #define glFunction gmlFunction   to the list in the lower half of gmldef.h
// Please note: Some functions may require more advanced coding to implement
// If a function is not yet supported by GML, a compile error pointing to 'GML_FUNCTION_NOT_IMPLEMENTED' will occur

#include "gmlcls.h"

#define EXEC_RUN (BYTE *)NULL
#define EXEC_SYNC (BYTE *)-1
#define EXEC_RES (BYTE *)-2

// TLS (thread local storage) thread identifier
#ifdef _MSC_VER
__declspec(thread) int gmlThreadNumber=0;
#else
__thread int gmlThreadNumber=0;
#endif
int gmlThreadCount=GML_CPU_COUNT; // number of threads to use
int gmlThreadCountOverride=0; // number of threads to use (can be manually overridden here)
int gmlItemsConsumed=0;

// gmlCPUCount returns the number of CPU cores
// it was taken from the latest version of boost
// boost::thread::hardware_concurrency()
#ifdef WIN32
#include <windows.h>
#else
#ifdef __linux__
#include <sys/sysinfo.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(__sun)
#include <unistd.h>
#endif
#endif
unsigned gmlCPUCount() {
#ifdef WIN32
	SYSTEM_INFO info={0};
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#else
#if defined(PTW32_VERSION) || defined(__hpux)
	return pthread_num_processors_np();
#elif defined(__linux__)
	return get_nprocs();
#elif defined(__APPLE__) || defined(__FreeBSD__)
	int count;
	size_t size=sizeof(count);
	return sysctlbyname("hw.ncpu",&count,&size,NULL,0)?0:count;
#elif defined(__sun)
	int const count=sysconf(_SC_NPROCESSORS_ONLN);
	return (count>0)?count:0;
#else
	return 0;
#endif
#endif
}

// cache maps for gmlInit
std::map<GLenum,GLint> gmlGetIntegervCache;
std::map<GLenum,GLfloat> gmlGetFloatvCache;
std::map<GLenum,std::string> gmlGetStringCache;

// params to be cached by gmlInit
GLenum gmlIntParams[]={GL_MAX_TEXTURE_SIZE,GL_MAX_TEXTURE_UNITS,GL_MAX_TEXTURE_IMAGE_UNITS_ARB,GL_MAX_TEXTURE_COORDS_ARB,GL_MAX_TEXTURE_UNITS_ARB,GL_UNPACK_ALIGNMENT};
GLenum gmlFloatParams[]={GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT};
GLenum gmlStringParams[]={GL_VERSION,GL_VENDOR,GL_RENDERER,GL_EXTENSIONS};

// gmlInit caches certain glGet return values to
// reduce the need for synced queue execution
BOOL_ gmlInited=FALSE;
void gmlInit() {
	if(gmlInited)
		return;
	for(int i=0; i<sizeof(gmlIntParams)/sizeof(GLenum); ++i) {
   	GLint gi;
 	  glGetIntegerv(gmlIntParams[i],&gi);
		gmlGetIntegervCache[gmlIntParams[i]]=gi;
	}
	for(int i=0; i<sizeof(gmlFloatParams)/sizeof(GLenum); ++i) {
   	GLfloat fi;
 	  glGetFloatv(gmlFloatParams[i],&fi);
		gmlGetFloatvCache[gmlFloatParams[i]]=fi;
	}
	for(int i=0; i<sizeof(gmlStringParams)/sizeof(GLenum); ++i) {
		std::string si=(char *)glGetString(gmlStringParams[i]);
		gmlGetStringCache[gmlStringParams[i]]=si;
	}
	gmlInited=TRUE;
}

EXTERN inline GLhandleARB glCreateShader_VERTEX() {
	return glCreateShader(GL_VERTEX_SHADER);
}
EXTERN inline GLhandleARB glCreateShader_FRAGMENT() {
	return glCreateShader(GL_FRAGMENT_SHADER);
}
EXTERN inline GLhandleARB glCreateShaderObjectARB_VERTEX() {
	return glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
}
EXTERN inline GLhandleARB glCreateShaderObjectARB_FRAGMENT() {
	return glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
}
gmlQueue gmlQueues[GML_MAX_NUM_THREADS];

boost::thread *gmlThreads[GML_MAX_NUM_THREADS];

// Item server instances
gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMPROC *> gmlProgramServer(&glCreateProgram, 2, 0);
gmlSingleItemServer<GLhandleARB, PFNGLCREATEPROGRAMOBJECTARBPROC *> gmlProgramObjectARBServer(&glCreateProgramObjectARB, 2, 0);

gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_VERTEX(&glCreateShader_VERTEX, 2, 0);
gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderServer_FRAGMENT(&glCreateShader_FRAGMENT, 2, 0);

gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_VERTEX(&glCreateShaderObjectARB_VERTEX, 2, 0);
gmlSingleItemServer<GLhandleARB, GLhandleARB (*)(void)> gmlShaderObjectARBServer_FRAGMENT(&glCreateShaderObjectARB_FRAGMENT, 2, 0);
gmlSingleItemServer<GLUquadric *, GLUquadric *(GML_GLAPIENTRY *)(void)> gmlQuadricServer(&gluNewQuadric, 100, 25);

gmlMultiItemServer<GLuint, GLsizei, void (GML_GLAPIENTRY *)(GLsizei,GLuint *)> gmlTextureServer(&glGenTextures, 100, 25);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSARBPROC *> gmlBufferARBServer(&glGenBuffersARB, 2, 0);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFENCESNVPROC *> gmlFencesNVServer(&glGenFencesNV, 2, 0);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENPROGRAMSARBPROC *> gmlProgramsARBServer(&glGenProgramsARB, 2, 0);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENRENDERBUFFERSEXTPROC *> gmlRenderbuffersEXTServer(&glGenRenderbuffersEXT, 2, 0);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENFRAMEBUFFERSEXTPROC *> gmlFramebuffersEXTServer(&glGenFramebuffersEXT, 2, 0);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENQUERIESPROC *> gmlQueryServer(&glGenQueries, 20, 5);
gmlMultiItemServer<GLuint, GLsizei, PFNGLGENBUFFERSPROC *> gmlBufferServer(&glGenBuffers, 2, 0);


// GMLqueue implementation
gmlQueue::gmlQueue():
ReadPos(0),WritePos(0),WriteSize(0),Read(0),Write(0),Locked1(FALSE),Locked2(FALSE),Reloc(FALSE),Sync(EXEC_RUN),WasSynced(FALSE),
ClientState(0),
CPsize(0), CPtype(0), CPstride(0), CPpointer(NULL), 
EFPstride(0), EFPpointer(NULL), 
IPtype(0), IPstride(0), IPpointer(NULL), 
NPtype(0), NPstride(0), NPpointer(NULL), 
TCPsize(0), TCPtype(0), TCPstride(0), TCPpointer(NULL),
ArrayBuffer(0), ElementArrayBuffer(0), PixelPackBuffer(0),PixelUnpackBuffer(0)
{
	Queue1=(BYTE *)malloc(GML_INIT_QUEUE_SIZE*sizeof(BYTE));
	Queue2=(BYTE *)malloc(GML_INIT_QUEUE_SIZE*sizeof(BYTE));
	Pos1=Queue1;
	Pos2=Queue2;
	Size1=Queue1+GML_INIT_QUEUE_SIZE;
	Size2=Queue2+GML_INIT_QUEUE_SIZE;
}

BYTE *gmlQueue::Realloc(BYTE **e) {
	int oldsize=WriteSize-Write;
	int newsize=oldsize*2;
	int oldpos=WritePos-Write;
	int olde=0;
	if(e)
		olde=*e-Write;
	if(Write==Queue1) {
		*(BYTE * volatile *)&Write=Queue1=(BYTE *)realloc(Queue1,newsize);
		Size1=Queue1+newsize;
	}
	else {
		*(BYTE * volatile *)&Write=Queue2=(BYTE *)realloc(Queue2,newsize);
		Size2=Queue2+newsize;
	}  
	*(BYTE * volatile *)&WritePos=Write+oldpos;
	*(BYTE * volatile *)&WriteSize=Write+newsize;

	Reloc=FALSE;
	if(e)
		*e=Write+olde;
	return WritePos;
}

BYTE *gmlQueue::WaitRealloc(BYTE **e) {
	int olde=0;
	if(e)
		olde=*e-Write;

	Reloc=TRUE;
	while(Reloc)
		boost::thread::yield();

	if(e)
		*e=(BYTE *)*(BYTE * volatile *)&Write+olde;
	return (BYTE *)*(BYTE * volatile *)&WritePos;
}

void gmlQueue::ReleaseWrite(BOOL_ final) {
	if(Write==NULL)
		return;
#if GML_ALTERNATE_SYNCMODE
	if(WritePos==Write) {
		*(int *)WritePos=0;
		WritePos+=sizeof(int);
	}

	if(Write==Queue1) {
		if(final) {
			while(*(BYTE * volatile *)&Pos2!=Queue2)
				boost::thread::yield();

			if(WasSynced) {
				Sync=WritePos;
				while(Sync==WritePos)
					boost::thread::yield();
			}
		}

		Pos1=WritePos;
		Locks1.Unlock();
		Locked1=FALSE;
	}
	else {
		if(final) {
			while(*(BYTE * volatile *)&Pos1!=Queue1)
				boost::thread::yield();

			if(WasSynced) {
				Sync=WritePos;
				while(Sync==WritePos)
					boost::thread::yield();
			}
		}

		Pos2=WritePos;
		Locks2.Unlock();
		Locked2=FALSE;
	}

	if(final && WasSynced) {
		while(Sync!=EXEC_RUN)
			boost::thread::yield();
		WasSynced=FALSE;
	}

#else
	if(Write==Queue1) {
		if(final) {
			while(*(BYTE * volatile *)&Pos2!=Queue2)
				boost::thread::yield();
		}
		if(WasSynced) {
			Sync=WritePos;
			while(Sync==WritePos)
				boost::thread::yield();
			WasSynced=FALSE;
		}
		Pos1=WritePos;
		Locks1.Unlock();
		Locked1=FALSE;
	}
	else {
		if(final) {
			while(*(BYTE * volatile *)&Pos1!=Queue1)
				boost::thread::yield();
		}
		if(WasSynced) {
			Sync=WritePos;
			while(Sync==WritePos)
				boost::thread::yield();
			WasSynced=FALSE;
		}
		Pos2=WritePos;
		Locks2.Unlock();
		Locked2=FALSE;
	}
#endif
	Write=NULL;
	WritePos=NULL;
	WriteSize=NULL;
}

BOOL_ gmlQueue::GetWrite(BOOL_ critical) {
	while(1) {
		if(!Locked1 && Pos1==Queue1) {
			if(Locks1.Lock()) {
				Locked1=TRUE;
				ReleaseWrite(critical==2);
				WritePos=Write=Queue1;
				WriteSize=Size1;
				return TRUE;
			}
		}
		if(!Locked2 && Pos2==Queue2) {
			if(Locks2.Lock()) {
				Locked2=TRUE;
				ReleaseWrite(critical==2);
				WritePos=Write=Queue2;
				WriteSize=Size2;
				return TRUE;
			}
		}
		if(!critical)
			return FALSE;
		boost::thread::yield();
	} 
}

void gmlQueue::ReleaseRead() {
	if(Read==NULL)
		return;
	if(Read==Queue1) {
		Pos1=Queue1;
		Locks1.Unlock();
		Locked1=FALSE;
	}
	else {
		Pos2=Queue2;
		Locks2.Unlock();
		Locked2=FALSE;
	}
	Read=NULL;
	ReadPos=NULL;
}

BOOL_ gmlQueue::GetRead(BOOL_ critical) {
	while(1) {
		if(!Locked1 && Pos1!=Queue1) {
			if(Locks1.Lock()) {
				Locked1=TRUE;
				Read=Queue1;
				ReadPos=Pos1;
				return TRUE;
			}
		}
		if(!Locked2 && Pos2!=Queue2) {
			if(Locks2.Lock()) {
				Locked2=TRUE;
				Read=Queue2;
				ReadPos=Pos2;
				return TRUE;
			}
		}
		if(!critical)
			return FALSE;
		boost::thread::yield();
	}
}


void gmlQueue::SyncRequest() {
#if GML_ALTERNATE_SYNCMODE
  // make sure server is finished with other queue
	if(Write==Queue1) {
		while(*(BYTE * volatile *)&Pos2!=Queue2)
			boost::thread::yield();
	}
	else {
		while(*(BYTE * volatile *)&Pos1!=Queue1)
			boost::thread::yield();
	}

	WasSynced=TRUE;
	Sync=EXEC_SYNC;
	while(Sync==EXEC_SYNC) // wait for syncmode confirmation before release
		boost::thread::yield();

	GetWrite(TRUE); // get new queue so server can get the old one
	while(Sync!=EXEC_RES) // waiting for result
		boost::thread::yield();
	Sync=EXEC_RUN; // server may proceed (avoid entering sync again)
#else
	if(Write==Queue1) {
		while(*(BYTE * volatile *)&Pos2!=Queue2)
			boost::thread::yield();
	}
	if(Write==Queue2) {
		while(*(BYTE * volatile *)&Pos1!=Queue1)
			boost::thread::yield();
	}
	BYTE *wp=WritePos;
	*(BYTE * volatile *)&WritePos=wp;
	WasSynced=TRUE;
	Sync=EXEC_SYNC;
	while(Sync==EXEC_SYNC)
		boost::thread::yield();
#endif
}


// Handler definition macros
// These handlers execute GL commands from the queues
#define GML_MAKEHANDLER0(name) case gml##name##Enum:\
	gl##name();\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER0R(name) case gml##name##Enum:\
	((gml##name##Data *)p)->ret=gl##name();\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER1(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER1R(name) case gml##name##Enum:\
	((gml##name##Data *)p)->ret=gl##name(((gml##name##Data *)p)->A);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER2(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER2R(name) case gml##name##Enum:\
	((gml##name##Data *)p)->ret=gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER3(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER4(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER5(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER6(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER7(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER8(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER9(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->I);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER9R(name) case gml##name##Enum:\
	((gml##name##Data *)p)->ret=gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->I);\
        p+=sizeof(gml##name##Data);\
        break;

#define GML_MAKEHANDLER10(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->I,((gml##name##Data *)p)->J);\
        p+=sizeof(gml##name##Data);\
        break;
//glTexImage1D
#define GML_MAKEHANDLER8S(name) case gml##name##Enum:\
  gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H?((BYTE *)(((gml##name##Data *)p)->H))-1:(BYTE *)(((gml##name##Data *)p)+1));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glTexImage2D
#define GML_MAKEHANDLER9S(name) case gml##name##Enum:\
  gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->I?((BYTE *)(((gml##name##Data *)p)->I))-1:(BYTE *)(((gml##name##Data *)p)+1));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glTexImage3D
#define GML_MAKEHANDLER10S(name) case gml##name##Enum:\
  gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->I,((gml##name##Data *)p)->J?((BYTE *)(((gml##name##Data *)p)->J))-1:(BYTE *)(((gml##name##Data *)p)+1));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glColor4fv
#define GML_MAKEHANDLER1V(name) case gml##name##Enum:\
	gl##name((&((gml##name##Data *)p)->A));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glFogfv
#define GML_MAKEHANDLER2V(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,(&((gml##name##Data *)p)->B));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glLight
#define GML_MAKEHANDLER3V(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,(&((gml##name##Data *)p)->C));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glUniformMatrix4fv
#define GML_MAKEHANDLER4V(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,(&((gml##name##Data *)p)->D));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glBufferDataARB
#define GML_MAKEHANDLER4VS(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,(&((gml##name##Data *)p)->C),((gml##name##Data *)p)->D);\
        p+=((gml##name##Data *)p)->size;\
        break;
//glShaderSource
#define GML_MAKEHANDLER4VSS(name,type) case gml##name##Enum:\
	ptr=(BYTE *)((gml##name##Data *)p)+((gml##name##Data *)p)->lensize;\
	for(int i=0; i<((gml##name##Data *)p)->B; ++i) {\
		GLint j=((intptr_t *)&((gml##name##Data *)p)->C)[i];\
		(&((gml##name##Data *)p)->C)[i]=(type *)ptr;\
		ptr+=j;\
	}\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,(&((gml##name##Data *)p)->C),NULL);\
        p+=((gml##name##Data *)p)->size;\
        break;
//glCompressedTexImage1DARB
#define GML_MAKEHANDLER7VP(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->GP?((gml##name##Data *)p)->GP-1:(&((gml##name##Data *)p)->G));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glCompressedTexImage2DARB
#define GML_MAKEHANDLER8VP(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->HP?((gml##name##Data *)p)->HP-1:(&((gml##name##Data *)p)->H));\
        p+=((gml##name##Data *)p)->size;\
        break;
//glCompressedTexImage3DARB
#define GML_MAKEHANDLER9VP(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)->G,((gml##name##Data *)p)->H,((gml##name##Data *)p)->IP?((gml##name##Data *)p)->IP-1:(&((gml##name##Data *)p)->I));\
        p+=((gml##name##Data *)p)->size;\
        break;
//gluBuild2DMipmaps
#define GML_MAKEHANDLER7S(name) case gml##name##Enum:\
	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,((gml##name##Data *)p)->D,((gml##name##Data *)p)->E,((gml##name##Data *)p)->F,((gml##name##Data *)p)+1);\
        p+=((gml##name##Data *)p)->size;\
        break;
//glLight
#define GMLMAKESUBHANDLER2(flag,fun,arg,name)\
	if(((gml##name##Data *)p)->ClientState & (1<<(flag-GL_VERTEX_ARRAY))) {\
 	  fun(0,(((gml##name##Data *)p)->ClientState & GML_##arg##_ARRAY_BUFFER)?((gml##name##Data *)p)->arg##pointer:ptr);\
	  ptr+=((gml##name##Data *)p)->arg##totalsize;\
	}
#define GMLMAKESUBHANDLER3(flag,fun,arg,name)\
	if(((gml##name##Data *)p)->ClientState & (1<<(flag-GL_VERTEX_ARRAY))) {\
 	  fun(((gml##name##Data *)p)->arg##type,0,(((gml##name##Data *)p)->ClientState & GML_##arg##_ARRAY_BUFFER)?((gml##name##Data *)p)->arg##pointer:ptr);\
	  ptr+=((gml##name##Data *)p)->arg##totalsize;\
	}
#define GMLMAKESUBHANDLER4(flag,fun,arg,name)\
	if(((gml##name##Data *)p)->ClientState & (1<<(flag-GL_VERTEX_ARRAY))) {\
 	  fun(((gml##name##Data *)p)->arg##size,((gml##name##Data *)p)->arg##type,0,(((gml##name##Data *)p)->ClientState & GML_##arg##_ARRAY_BUFFER)?((gml##name##Data *)p)->arg##pointer:ptr);\
	  ptr+=((gml##name##Data *)p)->arg##totalsize;\
	}
#define GMLMAKESUBHANDLERVA(name)\
  for(int i=0; i<((gml##name##Data *)p)->VAcount; ++i) {\
    VAstruct *va=(VAstruct *)ptr;\
 	  glVertexAttribPointer(va->target,va->size,va->type,va->normalized,0,va->buffer?va->pointer:(ptr+sizeof(VAstruct)));\
    ptr+=va->totalsize;\
  }


#define GML_MAKEHANDLER3VDA(name) case gml##name##Enum:\
	ptr=(BYTE *)(((gml##name##Data *)p)+1);\
	GMLMAKESUBHANDLER4(GL_VERTEX_ARRAY,glVertexPointer,VP,name)\
	GMLMAKESUBHANDLER4(GL_COLOR_ARRAY,glColorPointer,CP,name)\
	GMLMAKESUBHANDLER4(GL_TEXTURE_COORD_ARRAY,glTexCoordPointer,TCP,name)\
	GMLMAKESUBHANDLER3(GL_INDEX_ARRAY,glIndexPointer,IP,name)\
	GMLMAKESUBHANDLER3(GL_NORMAL_ARRAY,glNormalPointer,NP,name)\
	GMLMAKESUBHANDLER2(GL_EDGE_FLAG_ARRAY,glEdgeFlagPointer,EFP,name)\
	GMLMAKESUBHANDLERVA(name)\
	gl##name(((gml##name##Data *)p)->A,0,((gml##name##Data *)p)->C);\
  p+=((gml##name##Data *)p)->size;\
        break;

#define GML_MAKEHANDLER4VDE(name) case gml##name##Enum:\
	ptr=(BYTE *)(((gml##name##Data *)p)+1);\
	GMLMAKESUBHANDLER4(GL_VERTEX_ARRAY,glVertexPointer,VP,name)\
	GMLMAKESUBHANDLER4(GL_COLOR_ARRAY,glColorPointer,CP,name)\
	GMLMAKESUBHANDLER4(GL_TEXTURE_COORD_ARRAY,glTexCoordPointer,TCP,name)\
	GMLMAKESUBHANDLER3(GL_INDEX_ARRAY,glIndexPointer,IP,name)\
	GMLMAKESUBHANDLER3(GL_NORMAL_ARRAY,glNormalPointer,NP,name)\
	GMLMAKESUBHANDLER2(GL_EDGE_FLAG_ARRAY,glEdgeFlagPointer,EFP,name)\
	GMLMAKESUBHANDLERVA(name)\
  if(((gml##name##Data *)p)->ClientState & GML_ELEMENT_ARRAY_BUFFER)\
  	gl##name(((gml##name##Data *)p)->A,((gml##name##Data *)p)->B,((gml##name##Data *)p)->C,\
	           ((gml##name##Data *)p)->D);\
  else\
    glDrawArrays(((gml##name##Data *)p)->A,0,((gml##name##Data *)p)->B);\
  p+=((gml##name##Data *)p)->size;\
        break;


#include "gmlfun.h"
// this item server instance needs gmlDeleteLists from gmlfun.h, that is why it is declared down here
gmlItemSequenceServer<GLuint, GLsizei,GLuint (GML_GLAPIENTRY *)(GLsizei)> gmlListServer(&glGenLists, &gmlDeleteLists, 100, 25, 20, 5);

// queue handler - exequtes one GL command from queue (pointed to by p)
// ptr is a temporary variable used inside the handlers
inline void QueueHandler(BYTE *&p, BYTE *&ptr) {
		switch(*(int *)p) {
#if GML_ALTERNATE_SYNCMODE
			  case 0: p+=sizeof(int); break;
#endif
  			GML_MAKEHANDLER1(Disable)
				GML_MAKEHANDLER1(Enable)
				GML_MAKEHANDLER2(BindTexture)
				GML_MAKEHANDLER3(TexParameteri)
				GML_MAKEHANDLER1(ActiveTextureARB)
				GML_MAKEHANDLER4(Color4f)
				GML_MAKEHANDLER3(Vertex3f)
				GML_MAKEHANDLER3(TexEnvi)
				GML_MAKEHANDLER2(TexCoord2f)
				GML_MAKEHANDLER6(ProgramEnvParameter4fARB)
				GML_MAKEHANDLER0(End)
				GML_MAKEHANDLER1(Begin)
				GML_MAKEHANDLER1(MatrixMode)
				GML_MAKEHANDLER2(Vertex2f)
				GML_MAKEHANDLER0(PopMatrix)
				GML_MAKEHANDLER0(PushMatrix)
				GML_MAKEHANDLER0(LoadIdentity)
				GML_MAKEHANDLER3(Translatef)
				GML_MAKEHANDLER2(BlendFunc)
				GML_MAKEHANDLER1(CallList)
				GML_MAKEHANDLER3(Color3f)
				GML_MAKEHANDLER9S(TexImage2D)
				GML_MAKEHANDLER1V(Color4fv)
				GML_MAKEHANDLER2(BindProgramARB)
				GML_MAKEHANDLER3(Scalef)
				GML_MAKEHANDLER4(Viewport)
				GML_MAKEHANDLER2V(DeleteTextures)
				GML_MAKEHANDLER3(MultiTexCoord2fARB)
				GML_MAKEHANDLER2(AlphaFunc)
				GML_MAKEHANDLER1(DepthMask)
				GML_MAKEHANDLER1(LineWidth)
				GML_MAKEHANDLER2(BindFramebufferEXT)
				GML_MAKEHANDLER4(Rotatef)
				GML_MAKEHANDLER2(DeleteLists)
				GML_MAKEHANDLER1(DisableClientState)
				GML_MAKEHANDLER1(EnableClientState)
				GML_MAKEHANDLER4(Rectf)
				GML_MAKEHANDLER3V(Lightfv)
				GML_MAKEHANDLER7S(uBuild2DMipmaps)
				GML_MAKEHANDLER1(Clear)
				GML_MAKEHANDLER0(EndList)
				GML_MAKEHANDLER2(NewList)
				GML_MAKEHANDLER4(ClearColor)
				GML_MAKEHANDLER2(PolygonMode)
				GML_MAKEHANDLER1(ActiveTexture)
				GML_MAKEHANDLER2(Fogf)
				GML_MAKEHANDLER1V(MultMatrixf)
				GML_MAKEHANDLER6(Ortho)
				GML_MAKEHANDLER0(PopAttrib)
				GML_MAKEHANDLER3V(Materialfv)
				GML_MAKEHANDLER2(PolygonOffset)
				GML_MAKEHANDLER1(PushAttrib)
				GML_MAKEHANDLER1(CullFace)
				GML_MAKEHANDLER4(ColorMask)
				GML_MAKEHANDLER1V(Vertex3fv)
				GML_MAKEHANDLER3V(TexGenfv)
				GML_MAKEHANDLER2(Vertex2d)
				GML_MAKEHANDLER4(VertexPointer)
				GML_MAKEHANDLER3VDA(DrawArrays)
				GML_MAKEHANDLER2V(Fogfv)
				GML_MAKEHANDLER5(FramebufferTexture2DEXT)
				GML_MAKEHANDLER4(TexCoordPointer)
				GML_MAKEHANDLER9S(TexSubImage2D)
				GML_MAKEHANDLER2V(ClipPlane)
				GML_MAKEHANDLER4(Color4d)
				GML_MAKEHANDLER2(LightModeli)
				GML_MAKEHANDLER3(TexGeni)
				GML_MAKEHANDLER3(TexParameterf)
				GML_MAKEHANDLER8(CopyTexSubImage2D)
				GML_MAKEHANDLER2V(DeleteFramebuffersEXT)
				GML_MAKEHANDLER1V(LoadMatrixf)
				GML_MAKEHANDLER1(ShadeModel)
				GML_MAKEHANDLER1(UseProgram)
				GML_MAKEHANDLER1(ClientActiveTextureARB)
				GML_MAKEHANDLER2V(DeleteRenderbuffersEXT)
				GML_MAKEHANDLER0(Flush)
				GML_MAKEHANDLER3(Normal3f)
				GML_MAKEHANDLER1(UseProgramObjectARB)
				GML_MAKEHANDLER8VP(CompressedTexImage2DARB)
				GML_MAKEHANDLER1(DeleteObjectARB)
				GML_MAKEHANDLER2(Fogi)
				GML_MAKEHANDLER1V(MultMatrixd)
				GML_MAKEHANDLER2(PixelStorei)
				GML_MAKEHANDLER2(PointParameterf)
				GML_MAKEHANDLER3(TexCoord3f)
				GML_MAKEHANDLER2(Uniform1i)
				GML_MAKEHANDLER2(BindRenderbufferEXT)
				GML_MAKEHANDLER1V(Color3fv)
				GML_MAKEHANDLER1(DepthFunc)
				GML_MAKEHANDLER2(Hint)
				GML_MAKEHANDLER1(LogicOp)
				GML_MAKEHANDLER3(StencilOp)
				GML_MAKEHANDLER3V(TexEnvfv)
				GML_MAKEHANDLER4V(UniformMatrix4fv)
				GML_MAKEHANDLER4(uOrtho2D)
				GML_MAKEHANDLER2(AttachObjectARB)
				GML_MAKEHANDLER2(BindBufferARB)
				GML_MAKEHANDLER1V(Color3ubv)
				GML_MAKEHANDLER2(DetachObjectARB)
				GML_MAKEHANDLER4(FramebufferRenderbufferEXT)
				GML_MAKEHANDLER2(LineStipple)
				GML_MAKEHANDLER1V(LoadMatrixd)
				GML_MAKEHANDLER2(SetFenceNV)
				GML_MAKEHANDLER3(StencilFunc)
				GML_MAKEHANDLER10S(TexImage3D)
				GML_MAKEHANDLER2(Uniform1f)
				GML_MAKEHANDLER1(ClearStencil)
				GML_MAKEHANDLER4(ColorPointer)
				GML_MAKEHANDLER1(DeleteShader)
				GML_MAKEHANDLER4VDE(DrawElements)
				GML_MAKEHANDLER1(GenerateMipmapEXT)
				GML_MAKEHANDLER3(Materialf)
				GML_MAKEHANDLER3(NormalPointer)
				GML_MAKEHANDLER3V(ProgramEnvParameter4fvARB)
				GML_MAKEHANDLER4(RenderbufferStorageEXT)
				GML_MAKEHANDLER1(StencilMask)
				GML_MAKEHANDLER4(Uniform3f)
				GML_MAKEHANDLER4(uPerspective)
				GML_MAKEHANDLER1(ActiveStencilFaceEXT)
				GML_MAKEHANDLER2(AttachShader)
				GML_MAKEHANDLER10(BlitFramebufferEXT)
				GML_MAKEHANDLER4VS(BufferDataARB)
				GML_MAKEHANDLER1(ClearDepth)
				GML_MAKEHANDLER3(Color3ub)
				GML_MAKEHANDLER7VP(CompressedTexImage1DARB)
				GML_MAKEHANDLER9VP(CompressedTexImage3DARB)
				GML_MAKEHANDLER1(DrawBuffer)
				GML_MAKEHANDLER1(FrontFace)
				GML_MAKEHANDLER6(Frustum)
				GML_MAKEHANDLER1(LinkProgramARB)
				GML_MAKEHANDLER2(MultiTexCoord1f)
				GML_MAKEHANDLER3(MultiTexCoord2f)
				GML_MAKEHANDLER4(MultiTexCoord3f)
				GML_MAKEHANDLER5(MultiTexCoord4f)
				GML_MAKEHANDLER2V(PointParameterfv)
				GML_MAKEHANDLER1(PointSize)
				GML_MAKEHANDLER4V(ProgramStringARB)
				GML_MAKEHANDLER3(SecondaryColor3f)
				GML_MAKEHANDLER1(TexCoord1f)
				GML_MAKEHANDLER4(TexCoord4f)
				GML_MAKEHANDLER3(TexEnvf)
				GML_MAKEHANDLER3(TexGenf)
				GML_MAKEHANDLER8S(TexImage1D)
				GML_MAKEHANDLER2(Uniform1iARB)
				GML_MAKEHANDLER3(Uniform2f)
				GML_MAKEHANDLER3(Uniform2fARB)
				GML_MAKEHANDLER3(Uniform2i)
				GML_MAKEHANDLER4(Uniform3fARB)
				GML_MAKEHANDLER4(Uniform3i)
				GML_MAKEHANDLER5(Uniform4f)
				GML_MAKEHANDLER5(Uniform4i)
				GML_MAKEHANDLER4V(UniformMatrix2fv)
				GML_MAKEHANDLER4V(UniformMatrix3fv)
				GML_MAKEHANDLER4(Vertex4f)
				GML_MAKEHANDLER1(uDeleteQuadric)
				GML_MAKEHANDLER2(uQuadricDrawStyle)
				GML_MAKEHANDLER4(uSphere)
				GML_MAKEHANDLER4(ClearAccum)
				GML_MAKEHANDLER4(Color4ub)
				GML_MAKEHANDLER1V(Color4ubv)
				GML_MAKEHANDLER1(CompileShader)
				GML_MAKEHANDLER1(CompileShaderARB)
				GML_MAKEHANDLER8(CopyTexImage2D)
				GML_MAKEHANDLER2V(DeleteBuffersARB)
				GML_MAKEHANDLER2V(DeleteFencesNV)
				GML_MAKEHANDLER1(DeleteProgram)
				GML_MAKEHANDLER2V(DeleteProgramsARB)
				GML_MAKEHANDLER2(DetachShader)
				GML_MAKEHANDLER1(DisableVertexAttribArrayARB)
				GML_MAKEHANDLER2V(DrawBuffersARB)
				GML_MAKEHANDLER1(EdgeFlag)
				GML_MAKEHANDLER1(EnableVertexAttribArrayARB)
				GML_MAKEHANDLER0(Finish)
				GML_MAKEHANDLER1(FinishFenceNV)
				GML_MAKEHANDLER1(FogCoordf)
				GML_MAKEHANDLER3(Lightf)
				GML_MAKEHANDLER1(LinkProgram)
				GML_MAKEHANDLER1V(Normal3fv)
				GML_MAKEHANDLER2(RasterPos2i)
				GML_MAKEHANDLER1(ReadBuffer)
				GML_MAKEHANDLER4(Scissor)
				GML_MAKEHANDLER4VSS(ShaderSource,GLchar)
				GML_MAKEHANDLER4VSS(ShaderSourceARB,GLcharARB)
				GML_MAKEHANDLER1V(TexCoord2fv)
				GML_MAKEHANDLER3V(TexParameterfv)
				GML_MAKEHANDLER3(Translated)
				GML_MAKEHANDLER3V(Uniform1fv)
				GML_MAKEHANDLER5(Uniform4fARB)
				GML_MAKEHANDLER4V(UniformMatrix4fvARB)
				GML_MAKEHANDLER6(VertexAttribPointerARB)
				GML_MAKEHANDLER9(uLookAt)
				GML_MAKEHANDLER2V(LightModelfv)

				GML_MAKEHANDLER2V(DeleteQueries)
				GML_MAKEHANDLER1(BlendEquation)
				GML_MAKEHANDLER2(StencilMaskSeparate)
				GML_MAKEHANDLER4(StencilFuncSeparate)
				GML_MAKEHANDLER4(StencilOpSeparate)
				GML_MAKEHANDLER2(BeginQuery)
				GML_MAKEHANDLER1(EndQuery)
				GML_MAKEHANDLER3(GetQueryObjectuiv)
				GML_MAKEHANDLER2(BlendEquationSeparate)
				GML_MAKEHANDLER4(BlendFuncSeparate)
				GML_MAKEHANDLER6(uCylinder)

				GML_MAKEHANDLER2V(DeleteBuffers)
				GML_MAKEHANDLER2(BindBuffer)
				GML_MAKEHANDLER4VS(BufferData)
				GML_MAKEHANDLER2R(MapBuffer)
				GML_MAKEHANDLER1R(UnmapBuffer)
				GML_MAKEHANDLER8VP(CompressedTexImage2D)
				GML_MAKEHANDLER1R(IsShader)
				GML_MAKEHANDLER3(Vertex3i)

				GML_MAKEHANDLER2(GetIntegerv)
				GML_MAKEHANDLER1R(CheckFramebufferStatusEXT)
				GML_MAKEHANDLER2(GetFloatv)
				GML_MAKEHANDLER1R(GetString)
				GML_MAKEHANDLER2R(GetUniformLocationARB)
				GML_MAKEHANDLER7(ReadPixels)
				GML_MAKEHANDLER0R(GetError)
				GML_MAKEHANDLER3(GetObjectParameterivARB)
				GML_MAKEHANDLER2R(GetUniformLocation)
				GML_MAKEHANDLER2(GetDoublev)
				GML_MAKEHANDLER3(GetProgramiv)
				GML_MAKEHANDLER7(GetActiveUniform)
				GML_MAKEHANDLER2R(GetAttribLocationARB)
				GML_MAKEHANDLER4(GetInfoLogARB)
				GML_MAKEHANDLER4(GetProgramInfoLog)
				GML_MAKEHANDLER3(GetProgramivARB)
				GML_MAKEHANDLER4(GetShaderInfoLog)
				GML_MAKEHANDLER3(GetShaderiv)
				GML_MAKEHANDLER1R(IsRenderbufferEXT)
				GML_MAKEHANDLER2R(MapBufferARB)
				GML_MAKEHANDLER9R(uProject)
				GML_MAKEHANDLER9R(uScaleImage)
				GML_MAKEHANDLER1R(TestFenceNV)

				GML_MAKEHANDLER3(IndexPointer)
				GML_MAKEHANDLER2(EdgeFlagPointer)
				GML_MAKEHANDLER4(TrackMatrixNV)
		}
}

// Execute - executes all GL commands in the current read queue.
// Execution is non-synced
void gmlQueue::Execute() {
//	int procs=0;
  BYTE *p=Read;
  BYTE *e=ReadPos;
  BYTE *ptr=NULL;

  while(p<e) {
//   	GML_DEBUG("Cmd ",*(int *)p);
    QueueHandler(p,ptr);
//		++procs;
	}
//	GML_DEBUG("Execute ",procs);
}

extern void gmlUpdateServers();

// Execute - executes all GL commands in the current read queue.
// Execution is synced (this means it will stop at certain points
// to return values to the worker thread)
void gmlQueue::ExecuteSynced() {
//int procs=0;
#if GML_ALTERNATE_SYNCMODE
	BYTE *s;
	while(1) {
  	int updsrv=0;
		while((s=(BYTE *)Sync)==EXEC_RUN) {
			if(Reloc)
				Realloc();
      if((updsrv++%GML_UPDSRV_INTERVAL)==0 || *(volatile int *)&gmlItemsConsumed>=GML_UPDSRV_INTERVAL)
			  gmlUpdateServers();
			if(GetRead()) {
  			Execute();
	  		ReleaseRead();
			}
			boost::thread::yield();
		}

		if(s!=EXEC_SYNC) { // end addr
			Sync=EXEC_SYNC; //NEW
			GetRead(TRUE);
			Sync=EXEC_RUN; // cannot allow worker to continue before right queue acquired
			Execute();
			ReleaseRead();
			break;
		}

		Sync=EXEC_RUN; // sync confirmed
		GetRead(TRUE);
		Execute();
		Sync=EXEC_RES; // result available
		ReleaseRead();
		while(Sync==EXEC_RES) // waiting for worker to acquire result
			boost::thread::yield();
	}
#else
  BYTE *p=Write;
  BYTE *e=WritePos;
  BYTE *ptr=NULL;
	BOOL_ isq1=Write==Queue1;
	BOOL_ end=FALSE;
  int updsrv=0;

  while(TRUE) {
		if(!end) {
			while(TRUE) {
  			if(Reloc)
	  			e=Realloc(&p);
//				if(((++updsrv)%GML_UPDSRV_INTERVAL)==0)
        if((updsrv++%GML_UPDSRV_INTERVAL)==0 || *(volatile int *)&gmlItemsConsumed>=GML_UPDSRV_INTERVAL)
					gmlUpdateServers();
				BYTE *s=(BYTE *)Sync;
				if(s!=EXEC_RUN) {
  				if(s!=EXEC_SYNC) { // end addr ready
	  				end=TRUE;
		  			e=s;
     	  		Sync=EXEC_RUN;
						break;
					}
    			if(p==*(BYTE * volatile *)&WritePos) // reached sync point
    	  		Sync=EXEC_RUN;
				}
				if(p<*(BYTE * volatile *)&WritePos)
					break;
			}
		}
		if(end) {
			if(p==e)
				break;
		}
//   	GML_DEBUG("CmdSync ",*(int *)p);
		QueueHandler(p,ptr);
//		++procs;
	}
	if(isq1) {
		while(Locked1)
			boost::thread::yield();
		Pos1=Queue1;
	}
	else {
		while(Locked2)
			boost::thread::yield();
		Pos2=Queue2;
	}
#endif
//  GML_DEBUG("ExecuteSync ",procs);
}
