// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used freely for any purpose, as
// long as this notice remains unchanged

#ifndef GMLCLASSES_H
#define GMLCLASSES_H

#include <GL/glew.h>

#include "gmlcnt.h"

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/version.hpp>
#include <map>
#include <set>
#include <string.h>

#define GML_QUOTE(x) #x

#ifdef USE_GML
#	define GML_ENABLE 1 // multithreaded drawing of units and ground
#else
#	define GML_ENABLE 0 // manually enable opengl multithreading here
#endif

#ifdef USE_GML_SIM
#	define GML_ENABLE_SIM (GML_ENABLE && 1) // runs a completely independent thread loop for the Sim
#else
#	define GML_ENABLE_SIM 0  // manually enable sim thread here
#endif

#ifdef USE_GML_DEBUG
#	define GML_CALL_DEBUG (GML_ENABLE && GML_ENABLE_SIM && 1) // checks for calls made from the wrong thread
#else
#	define GML_CALL_DEBUG 0  // manually enable call debug here
#endif

#define GML_ENABLE_DRAW (GML_ENABLE && 0) // draws everything in a separate thread (for testing only, will degrade performance)
#define GML_SERVER_GLCALL 1 // allows the server thread (0) to make direct GL calls
#define GML_INIT_QUEUE_SIZE 10 // initial queue size, will be reallocated, but must be >= 4
#define GML_USE_NO_ERROR 1 // glGetError always returns success (to improve performance)
#define GML_USE_DEFAULT 1// compile/link/buffer status always returns TRUE/COMPLETE (to improve performance)
#define GML_USE_CACHE 1 // certain glGet calls may use data cached during gmlInit (to improve performance)
//#define GML_USE_QUADRIC_SERVER 1 // use server thread to create/delete quadrics
#define GML_AUX_PREALLOC 128*1024 // preallocation size for aux queue to reduce risk for hang if gl calls happen to be made from Sim thread
#define GML_ENABLE_ITEMSERVER_CHECK (GML_ENABLE_SIM && 1) // if calls to itemserver are made from Sim, output errors to log
#define GML_UPDSRV_INTERVAL 10
#define GML_ALTERNATE_SYNCMODE 1 // mutex-protected synced execution, slower but more portable
#define GML_ENABLE_TLS_CHECK 1 // check if Thread Local Storage appears to be working
#define GML_GCC_TLS_FIX 1 // fix buggy TLS in GCC by using the Win32 TIB (faster also!)
#define GML_MSC_TLS_OPT 1 // use the Win32 TIB for TLS in MSVC (possibly faster)
#define GML_64BIT_USE_GS 1 // 64-bit OS will use the GS register for TLS (untested feature)
#define GML_LOCKED_GMLCOUNT_ASSIGNMENT 0 // experimental feature, probably not needed
//#define BOOST_AC_USE_PTHREADS

// memory barriers for different platforms
#if defined(__APPLE__) || defined(__FreeBSD__)
#	include <libkern/OSAtomic.h>
#	define GML_MEMBAR OSMemoryBarrier()
#elif defined(__GNUC__)
#	if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#		define GML_MEMBAR __sync_synchronize()
#	elif defined( __ppc__ ) || defined( __powerpc__) || defined( __PPC__ )
#		define GML_MEMBAR asm volatile("sync":::"memory")
#	elif defined( __i386__ ) || defined( __i486__ ) || defined( __i586__ ) || defined( __i686__ ) || defined( __x86_64__ )
#		define GML_MEMBAR asm volatile("mfence":::"memory")
#	endif
#elif defined(_MSC_VER)
#	if (_MSC_VER >= 1400) 
#		define GML_MEMBAR // no barrier needed for MSVS 2005
#	else
#		define GML_MEMBAR MemoryBarrier() // _asm {lock add [esp], 0}
#	endif
#elif defined(__BORLANDC__)
#	define GML_MEMBAR _asm {lock add [esp], 0}
#endif

#ifdef GML_MEMBAR
#	define GML_ORDERED_VOLATILE 1
#else
#	define GML_ORDERED_VOLATILE 0
#	define GML_MEMBAR
#endif

// optimize by assuming volatile accesses are
// guaranteed not to be reordered (MSVS 2005 or memory barrier needed)
// http://msdn.microsoft.com/en-us/library/12a04hfd(VS.80).aspx
// http://msdn.microsoft.com/en-us/library/ms686355(VS.85).aspx
// http://msdn.microsoft.com/en-us/library/bb310595(VS.85).aspx

#if GML_ORDERED_VOLATILE
#	define GML_VOLATILE(x) *(x volatile *)&
#	define GML_MUTEX
#	define GML_MUTEX_LOCK()
#	define GML_MUTEX_UNLOCK()
#else
#	define GML_VOLATILE(x)
#	define GML_MUTEX gmlMutex mutex
#	define GML_MUTEX_LOCK() mutex.Lock()
#	define GML_MUTEX_UNLOCK() mutex.Unlock()
#endif

#ifdef _MSC_VER
#	define GML_TYPENAME typename
#else
#	define GML_TYPENAME
#endif

#ifndef _WIN32 //defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#	define GML_USE_SPEEDY_TLS 1
#	include "System/Platform/errorhandler.h"
#	include "speedy-tls.h"
#else
#	define GML_USE_SPEEDY_TLS 0
#endif

#define set_threadnum(val) gmlThreadNumber=val

#if GML_ENABLE
#	ifdef _MSC_VER
#		if GML_MSC_TLS_OPT
inline int get_threadnum(void) {
	int val;
	__asm {
#			if !defined(_WIN64) || !GML_64BIT_USE_GS
		mov EAX, FS:[14h]
#			else
		mov EAX, GS:[28h]
#			endif
		mov [val], EAX
	}
	return val;
}
#			define gmlThreadNumber get_threadnum()
#			undef set_threadnum
inline void set_threadnum(int val) {
	__asm {
		mov EAX, [val]
#			if !defined(_WIN64) || !GML_64BIT_USE_GS
		mov FS:[14h], EAX
#			else
		mov GS:[28h], EAX
#			endif
	}
}
#		else
extern __declspec(thread) int gmlThreadNumber;
#		endif
#	else
#		if GML_GCC_TLS_FIX || GML_USE_SPEEDY_TLS
inline int get_threadnum(void) {
	int val;
#			if GML_USE_SPEEDY_TLS
	speedy_tls_get_int32(0, 0, 4, val);
#			else
#				if !defined(_WIN64) || !GML_64BIT_USE_GS
	__asm__("mov %%fs:0x14, %0" : "=r" (val) : : );
#				else
	__asm__("mov %%gs:0x28, %0" : "=r" (val) : : );
#				endif
#			endif
	return val;
}
#			define gmlThreadNumber get_threadnum()
#			undef set_threadnum
inline void set_threadnum(int val) {
#			if GML_USE_SPEEDY_TLS
	if (speedy_tls_init(sizeof(int))<0) { // this works because we only set the thread number once per thread
		handleerror(NULL, "Failed to initialize Thread Local Storage", "GML error:", MBF_OK | MBF_EXCL);
	}
	speedy_tls_put_int32(0, 0, 4, val);
#			else
#				if !defined(_WIN64) || !GML_64BIT_USE_GS
	__asm__ __volatile__("mov %0,%%fs:0x14" : : "r" (val));
#				else
	__asm__ __volatile__("mov %0,%%gs:0x28" : : "r" (val));
#				endif
#			endif
}
#		else
extern __thread int gmlThreadNumber;
#		endif
#	endif
#else
extern int gmlThreadNumber;
#endif

extern int gmlThreadCount;
extern int gmlThreadCountOverride;
extern unsigned gmlCPUCount();
#if (BOOST_VERSION >= 103500)
#	define GML_CPU_COUNT (gmlThreadCountOverride ? gmlThreadCountOverride : boost::thread::hardware_concurrency() )
#else
#	define GML_CPU_COUNT (gmlThreadCountOverride ? gmlThreadCountOverride : gmlCPUCount() )
#endif
#define GML_MAX_NUM_THREADS (32+1) // one extra for the aux (Sim) thread
#define GML_IF_SERVER_THREAD() if(GML_SERVER_GLCALL && (!GML_ENABLE || gmlThreadNumber==0))
extern int gmlItemsConsumed;

typedef unsigned char BYTE;
typedef int BOOL_;

#define TRUE 1
#define FALSE 0
#define EXTERN
#define GML_VP_ARRAY_BUFFER (1<<(16+GL_VERTEX_ARRAY-GL_VERTEX_ARRAY))
#define GML_CP_ARRAY_BUFFER (1<<(16+GL_COLOR_ARRAY-GL_VERTEX_ARRAY))
#define GML_TCP_ARRAY_BUFFER (1<<(16+GL_TEXTURE_COORD_ARRAY-GL_VERTEX_ARRAY))
#define GML_IP_ARRAY_BUFFER (1<<(16+GL_INDEX_ARRAY-GL_VERTEX_ARRAY))
#define GML_NP_ARRAY_BUFFER (1<<(16+GL_NORMAL_ARRAY-GL_VERTEX_ARRAY))
#define GML_EFP_ARRAY_BUFFER (1<<(16+GL_EDGE_FLAG_ARRAY-GL_VERTEX_ARRAY))
#define GML_ELEMENT_ARRAY_BUFFER (1<<29)

#ifdef _WIN32
#	ifdef __MINGW32__
#		define GML_APIENTRY __stdcall
#	elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#		define GML_APIENTRY __stdcall
#	else
#		define GML_APIENTRY
#	endif
#	ifndef GML_GLAPIENTRY
#		define GML_GLAPIENTRY GML_APIENTRY
#	endif
#else /* _UNIX */
#	define GML_APIENTRY
#	ifndef GML_GLAPIENTRY
#		define GML_GLAPIENTRY
#	endif
#endif /* _WIN32 */

// gmlMutex - exploits the boost mutex to get direct access to the Lock/Unlock methods
class gmlMutex {
	boost::mutex sl_mutex;
	BYTE sl_lock[sizeof(boost::mutex::scoped_lock)*GML_MAX_NUM_THREADS];
public:
	gmlMutex() {
	}
	virtual ~gmlMutex() {
	}
	void Lock() {
		new (((boost::mutex::scoped_lock *)sl_lock)+gmlThreadNumber) boost::mutex::scoped_lock(sl_mutex);
	}
	void Unlock() {
#if (BOOST_VERSION >= 103500)
		(((boost::mutex::scoped_lock *)sl_lock)+gmlThreadNumber)->~unique_lock();
#else
		(((boost::mutex::scoped_lock *)sl_lock)+gmlThreadNumber)->~scoped_lock();
#endif
	}
};

// gmlLock - combines boost mutex+lock into one covenient package
class gmlLock {
	boost::try_mutex sl_mutex;
	BYTE sl_lock[sizeof(boost::try_mutex::scoped_try_lock)*GML_MAX_NUM_THREADS];
	
public:
	gmlLock() {
	}
	virtual ~gmlLock() {
	}
	bool Lock() {
		boost::try_mutex::scoped_try_lock *lock=((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber;
#if (BOOST_VERSION >= 103600)
		new (lock) boost::try_mutex::scoped_try_lock(sl_mutex);
		if(lock->owns_lock())
			return true;
		lock->~try_lock_wrapper();
#elif (BOOST_VERSION >= 103500)
		new (lock) boost::try_mutex::scoped_try_lock(sl_mutex,boost::try_to_lock);
		if(lock->owns_lock())
			return true;
		lock->~unique_lock();
#else
		new (lock) boost::try_mutex::scoped_try_lock(sl_mutex);
		if(lock->locked())
			return true;
		lock->~scoped_try_lock();
#endif
		return false;
	}
	void Unlock() {
#if (BOOST_VERSION >= 103600)
		(((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber)->~try_lock_wrapper();
#elif (BOOST_VERSION >= 103500)
		(((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber)->~unique_lock();
#else
		(((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber)->~scoped_try_lock();
#endif
	}
};

#include <iterator>

template<class T>
class gmlVectorIter {
public:
	T *p; 
	gmlVectorIter() {} 
	gmlVectorIter(T *d) {p=d;} 
	void operator=(const GML_TYPENAME gmlVectorIter<T> &i) {p=i.p;}
	GML_TYPENAME gmlVectorIter<T> &operator++() {++p; return *this;} 
	GML_TYPENAME gmlVectorIter<T> operator++(int) {return GML_TYPENAME gmlVectorIter<T>(p++);} 
	int operator!=(const GML_TYPENAME gmlVectorIter<T> &i) const {return p!=i.p;}
	
	ptrdiff_t operator-(const GML_TYPENAME gmlVectorIter<T> &i) const {return p-i.p;}
	int operator<(const GML_TYPENAME gmlVectorIter<T> &i) const {return p<i.p;}
	GML_TYPENAME gmlVectorIter<T> &operator--() {--p; return *this;} 
	GML_TYPENAME gmlVectorIter<T> operator--(int) {return GML_TYPENAME gmlVectorIter<T>(p--);} 
	GML_TYPENAME gmlVectorIter<T> operator+(int i) const {return GML_TYPENAME gmlVectorIter<T>(p+i);}
	GML_TYPENAME gmlVectorIter<T> operator-(int i) const {return GML_TYPENAME gmlVectorIter<T>(p-i);}
	int operator==(const GML_TYPENAME gmlVectorIter<T> &i) const {return p==i.p;}
	T &operator*() {return *p;}
	
	typedef std::random_access_iterator_tag iterator_category;
	typedef T value_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef T& reference;
};

// gmlClassVector - partially thread safe vector class for storing advanced types (classes etc.)
// The array will be resized automatically when using the functions acquire() and release()
// using "volatile" optimizations result in about 300% performance gain under normal load
// high load will make "volatile" optimizations run about 3 times slower compared to mutex
template<class T>
class gmlClassVector {
	T *data;
#if GML_ORDERED_VOLATILE
	gmlCount count;
	volatile long added;
#else
	gmlMutex mutex;
	long added;
#endif
	int maxsize;
	int doshrink;
	int shrinksize;
	int nalloc;
	
public:
	gmlClassVector():doshrink(0),shrinksize(0),nalloc(0),
#if GML_ORDERED_VOLATILE
		count(0),
#endif
		added(0) {
		data=(T *)malloc(1*sizeof(T));
		maxsize=1;
	}
	
	~gmlClassVector() {
		if(added>nalloc)
			nalloc=added;
		for(int i=0; i<nalloc; ++i)
			data[i].~T();
		free(data);
	}
	
	typedef GML_TYPENAME gmlVectorIter<T> iterator;
	
	iterator begin() {
		return iterator(data);
	}
	
	iterator end() {
		return iterator(data+added);
	}
	
	long size() const {
		return added;
	}
	
	const T &operator[](int i) const {
		return data[i];
	}
	
	T &operator[](int i) {
		return data[i];
	}
	
	T &acquire(int i) { // thread safe
#if GML_ORDERED_VOLATILE
		long sz;
		while(TRUE) {
			if(added<=i) {
				if(count<=i) {
					if((sz=++count)<=i+1) {
						if(added==sz-1) {
							T *da=GML_VOLATILE(T *) data;
							int ms;
							if(sz==(ms=GML_VOLATILE(int) maxsize))
								Expand(da,ms);
							new ((void *)(volatile T *)(da+sz-1)) T();
							GML_MEMBAR;
							++added;
						}
						else {
							--count;
							while(count!=added)
								boost::thread::yield();
						}
					}
					else {
						--count;
					}
				}
			}
			else {
				long sz=++count;
				if(added==sz-1) {
					return (GML_VOLATILE(T *)data)[i];
				}
				--count;
				while(count!=added)
					boost::thread::yield();
			}
		}
#else
		mutex.Lock();
		while(added<=i) {
			long sz=++added;
			if(sz==maxsize)
				Expand(data,maxsize);
			new (data+sz-1) T();
		}
		return data[i];
#endif
	}
	
	void release() { // thread safe
#if GML_ORDERED_VOLATILE
		--count;
#else
		mutex.Unlock();
#endif
	}
	
	void push_back(const T &d) { // thread safe
#if GML_ORDERED_VOLATILE
		while(TRUE) {
			long sz=++count;
			if(added==sz-1) {
				T *da=GML_VOLATILE(T *) data;
				int ms;
				if(sz==(ms=GML_VOLATILE(int) maxsize))
					Expand(da,ms);
				new ((void *)(volatile T *)(da+sz-1)) T(d);
				GML_MEMBAR;
				++added;
				return;
			}
			else {
				--count;
				while(count!=added)
					boost::thread::yield();
			}
		}
#else
		mutex.Lock();
		long sz=++added;
		if(sz==maxsize)
			Expand(data,maxsize);
		new (data+sz-1) T(d);
		mutex.Unlock();
#endif
	}
	
	// this is probably overkill since realloced memory will never be cached in registers anyway
	BYTE *volatile_realloc(BYTE *dt, const int osz, const int sz) {
#if GML_ORDERED_VOLATILE 
		BYTE *dtn=(BYTE *)malloc(sz);
		for(int i=0; i<osz; ++i)
			*(volatile BYTE *)dtn++=*(volatile BYTE *)dt++;
		free(dt-osz);
		return dtn-osz;
#else
		return (BYTE *)realloc(dt,sz);
#endif
	}

	void Expand(T *&da, const int ms) {
		shrinksize=ms;
		int ms2=ms<<1;
		da=(T *)volatile_realloc((BYTE *)da,ms*sizeof(T),ms2*sizeof(T));
		GML_VOLATILE(T *) data=da; 
		GML_MEMBAR;
		GML_VOLATILE(int) maxsize=ms2;
	}
	
	void Shrink() {
		int ms=shrinksize;
		shrinksize=ms>>1;
		doshrink=0;
		for(int i=ms; i<nalloc; ++i)
			data[i].~T();
		if(ms<nalloc)
			nalloc=ms;
		data=(T *)realloc(data,ms*sizeof(T));
		maxsize=ms;
	}
	
	void clear() {
		if(added>nalloc)
			nalloc=added;
		long sz=added;
#if GML_ORDERED_VOLATILE
		count%=0;
#endif
		added=0;
		if(sz>=shrinksize)
			doshrink=0;
		else if(++doshrink>=10)
			Shrink();
	}
};

// gmlVector - partially thread safe vector class for storing simple types (int, pointer etc)
// using "volatile" optimizations result in about 300% performance gain under normal load
// high load will make "volatile" optimizations run about 3 times slower compared to mutex
template<class T>
class gmlVector {
	T *data;
#if GML_ORDERED_VOLATILE
	gmlCount count;
	volatile long added;
#else
	gmlMutex mutex;
	long added;
#endif
	int maxsize;
	int doshrink;
	int shrinksize;
	
public:
	gmlVector():doshrink(0),shrinksize(0),
#if GML_ORDERED_VOLATILE
		count(0),
#endif
		added(0) {
		data=(T *)malloc(1*sizeof(T));
		maxsize=1;
	}
	gmlVector(const GML_TYPENAME gmlVector<T> &vec):
#if GML_ORDERED_VOLATILE
		count(0),
#endif
		added(0) {
		memcpy(this,&vec,sizeof(GML_TYPENAME gmlVector<T>));
		data=(T *)malloc(vec.maxsize*sizeof(T));
		memcpy(data,vec.data,vec.maxsize*sizeof(T));
	}

	void swap(GML_TYPENAME gmlVector<T> &vec) {
#if GML_ORDERED_VOLATILE
		long tcount = count;
		count %= vec.count;
		vec.count %= tcount;
#endif
		T *tdata = data;
		long tadded = added;
		int tmaxsize = maxsize;
		int tdoshrink = doshrink;
		int tshrinksize = shrinksize;
		data = vec.data;
		added = vec.added;
		maxsize = vec.maxsize;
		doshrink = vec.doshrink;
		shrinksize = vec.shrinksize;
		vec.data = tdata;
		vec.added = tadded;
		vec.maxsize = tmaxsize;
		vec.doshrink = tdoshrink;
		vec.shrinksize = tshrinksize;
	}

	gmlVector<T> &operator=(const GML_TYPENAME gmlVector<T> &vec) {
#if GML_ORDERED_VOLATILE
		count%=vec.count;
#endif
		added=vec.added;
		if(added>=maxsize) {
			maxsize=vec.maxsize;
			data=(T *)realloc(data, maxsize*sizeof(T));
			shrinksize=vec.shrinksize;
		}
		memcpy(data,vec.data,added*sizeof(T));
		if(added>=shrinksize)
			doshrink=0;
		else if(++doshrink>=10)
			Shrink();
		return *this;
	}
	
	~gmlVector() {
		free(data);
	}
	
	typedef GML_TYPENAME gmlVectorIter<T> iterator;
	
	iterator begin() const {
		return iterator(data);
	}
	
	iterator end() const {
		return iterator(data+added);
	}
	
	const long size() const {
		return added;
	}
	
	const T &operator[](const int i) const {
		return data[i];
	}
	
	T &operator[](const int i) {
		return data[i];
	}
	
	void push_back(const T &d) { // thread safe
#if GML_ORDERED_VOLATILE
		long sz=++count;
		while(added!=sz-1)
			boost::thread::yield();
		T *da=GML_VOLATILE(T *) data;
		int ms;
		if(sz==(ms=GML_VOLATILE(int) maxsize))
			Expand(da,ms);
		*(volatile T *)(da+sz-1)=d;
		GML_MEMBAR;
		++added;
#else
		mutex.Lock();
		long sz=++added;
		if(sz==maxsize)
			Expand(data,maxsize);
		data[sz-1]=d;
		mutex.Unlock();
#endif
	}
	
	// this is probably overkill since realloced memory will never be cached in registers anyway
	BYTE *volatile_realloc(BYTE *dt, const int osz, const int sz) {
#if GML_ORDERED_VOLATILE 
		BYTE *dtn=(BYTE *)malloc(sz);
		for(int i=0; i<osz; ++i)
			*(volatile BYTE *)dtn++=*(volatile BYTE *)dt++;
		free(dt-osz);
		return dtn-osz;
#else
		return (BYTE *)realloc(dt,sz);
#endif
	}
	
	void Expand(T *&da, const int ms) {
		shrinksize=ms;
		int ms2=ms<<1;
		da=(T *)volatile_realloc((BYTE *)da,ms*sizeof(T),ms2*sizeof(T));
		GML_VOLATILE(T *) data=da;
		GML_MEMBAR;
		GML_VOLATILE(int) maxsize=ms2;
	}
	
	void Shrink() {
		int ms=shrinksize;
		shrinksize=ms>>1;
		doshrink=0;
		data=(T *)realloc(data,ms*sizeof(T));
		maxsize=ms;
	}
	
	void clear() {
		long sz=added;
#if GML_ORDERED_VOLATILE
		count%=0;
#endif
		added=0;
		if(sz>=shrinksize)
			doshrink=0;
		else if(++doshrink>=10)
			Shrink();
	}
};

struct VAdata {
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	const GLvoid *pointer;
	GLuint buffer;
	VAdata(){}
	VAdata(GLint si, GLenum ty, GLboolean no, GLsizei st, const GLvoid *po, GLuint buf):
	size(si),type(ty),normalized(no),stride(st),pointer(po),buffer(buf) {}
};

struct VAstruct {
	GLuint target;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLvoid * pointer;
	GLuint buffer;
	int totalsize;
};


struct gmlQueue {
	std::map<GLuint,VAdata> VAmap;
	std::set<GLuint> VAset;
	
	BYTE *ReadPos;
	BYTE *WritePos;
	BYTE *Pos1;
	BYTE *Pos2;
	
	BYTE *WriteSize;
	BYTE *Size1;
	BYTE *Size2;
	
	BYTE *Read;
	BYTE *Write;
	BYTE *Queue1;
	BYTE *Queue2;
	
	gmlLock Locks1;
	gmlLock Locks2;
	volatile BOOL_ Locked1;
	volatile BOOL_ Locked2;
	
	volatile BOOL_ Reloc;
	BYTE * volatile Sync;
	BOOL_ WasSynced;
	
	GLenum ClientState;
	// VertexPointer
	GLint VPsize;
	GLenum VPtype;
	GLsizei VPstride;
	const GLvoid *VPpointer;
	// ColorPointer
	GLint CPsize;
	GLenum CPtype;
	GLsizei CPstride;
	const GLvoid *CPpointer;
	// EdgeFlagPointer
	GLsizei EFPstride;
	const GLboolean *EFPpointer;
	// IndexPointer
	GLenum IPtype;
	GLsizei IPstride;
	const GLvoid *IPpointer;
	// NormalPointer
	GLenum NPtype;
	GLsizei NPstride;
	const GLvoid *NPpointer;
	// TexCoordPointer
	GLint TCPsize;
	GLenum TCPtype;
	GLsizei TCPstride;
	const GLvoid *TCPpointer;

	GLuint ArrayBuffer;
	GLuint ElementArrayBuffer;
	GLuint PixelPackBuffer;
	GLuint PixelUnpackBuffer;
	
	gmlQueue();
	
	BYTE *Realloc(BYTE **e=NULL);
	BYTE *WaitRealloc(BYTE **e=NULL);
	void ReleaseWrite(BOOL_ final=TRUE);
	BOOL_ GetWrite(BOOL_ critical);
	void ReleaseRead();
	BOOL_ GetRead(BOOL_ critical=FALSE);
	void SyncRequest();
	void Execute();
	void ExecuteSynced(void (gmlQueue::*execfun)() =&gmlQueue::Execute);
	void ExecuteDebug();
};



template<class T,class S, class C>
class gmlItemSequenceServer {
	typedef void (*delitemseqfun)(T, S);
	C genfun;
	delitemseqfun delfun;
	gmlCount req;
	gmlCount avail;
	int pregen;
	int arr_size;
	T *item_arr;
	gmlCount req_large;
	gmlCount avail_large;
	gmlCount size_large;
	int pregen_large;	
	int large_arr_size;
	T *large_item_arr;
	S *large_size_arr;
	GML_MUTEX;
	
public:
	gmlItemSequenceServer(C gf, delitemseqfun df, int sz, int pg, int sz_l, int pg_l):
		req(0),avail(0),req_large(0),avail_large(0),size_large(2) {
		genfun=gf;
		delfun=df;
		pregen=pg;
		arr_size=sz;
		item_arr=new T[arr_size];
		memset(item_arr,0,arr_size*sizeof(T));
		pregen_large=pg_l;
		large_arr_size=sz_l;
		large_item_arr=new T[large_arr_size];
		large_size_arr=new S[large_arr_size];
		memset(large_item_arr,0,large_arr_size*sizeof(T));
		memset(large_size_arr,0,large_arr_size*sizeof(S));
	}
	
	virtual ~gmlItemSequenceServer() {
		delete [] item_arr;
		delete [] large_item_arr;
		delete [] large_size_arr;
	}
	
	inline void GenerateItems() {
		// small
		int i;		
		while(avail<req+pregen && item_arr[i=(avail%arr_size)]==0) {
			GML_MUTEX_LOCK();
			*(volatile T *)(item_arr+i)=(*genfun)(1);
			GML_MEMBAR; // perhaps not needed, because ++avail acts as a barrier
			GML_MUTEX_UNLOCK();
			++avail;
		}
		// large
		while(avail_large<req_large+pregen_large && large_size_arr[i=(avail_large%large_arr_size)]==0) {
			S gensize=(S)size_large;
			GML_MUTEX_LOCK();
			*(volatile T *)(large_item_arr+i)=(*genfun)(gensize);
			GML_MEMBAR;
			*(volatile S *)(large_size_arr+i)=gensize;
			GML_MEMBAR; // perhaps not needed, because ++avail_large acts as a barrier
			GML_MUTEX_UNLOCK();
			++avail_large;
		}
	}
	
	inline T GetItems(S n) {
		GML_IF_SERVER_THREAD() {
			return (*genfun)(n);
		}
		++gmlItemsConsumed;
		if(n==1) {
			long num=++req;
			while(avail<num) // waiting
				boost::thread::yield();
			T *ip=item_arr+(num-1)%arr_size;
			GML_MUTEX_LOCK();
			T ipv=*(volatile T *)ip;
			*(volatile T *)ip=0;
			GML_MUTEX_UNLOCK();
			return ipv;
		}
		if(n==0)
			return 0;
		while(size_large<n)
			++size_large;
		while(TRUE) {
			long num=++req_large;
			while(avail_large<num) // waiting
				boost::thread::yield();
			int idx=(num-1)%large_arr_size;
			GML_MUTEX_LOCK();
			T ip=*(volatile T *)(large_item_arr+idx);
			S *sz=large_size_arr+idx;
			S szv=*(volatile S *)sz;
			GML_MEMBAR;
			*(volatile S *)sz=0;
			GML_MUTEX_UNLOCK();
			if(szv>n)
				(*delfun)(ip+n,szv-n); // del excessive
			if(szv<n)
				(*delfun)(ip,szv); // del all
			if(szv>=n)
				return ip;
		}
	}
};


template<class T, class C>
class gmlSingleItemServer {
	C genfun;
	gmlCount req;
	gmlCount avail;
	int pregen;
	int arr_size;
	T *arr;
	GML_MUTEX;
	
public:
	gmlSingleItemServer(C gf, int sz, int pg):req(0),avail(0) {
		genfun=gf;
		pregen=pg;
		arr_size=sz;
		arr=new T[arr_size];
		memset(arr,0,arr_size*sizeof(T));
	}
	
	virtual ~gmlSingleItemServer() {
		delete [] arr;
	}
	
	inline void GenerateItems() {
		int i;		
		while(avail<req+pregen && arr[i=(avail%arr_size)]==0) {
			GML_MUTEX_LOCK();
			*(volatile T *)(arr+i)=(*genfun)();
			GML_MEMBAR; // perhaps not needed, because ++avail acts as a barrier
			GML_MUTEX_UNLOCK();
			++avail;
		}
	}
	
	inline T GetItems() {
		GML_IF_SERVER_THREAD() {
			return (*genfun)();
		}
		++gmlItemsConsumed;
		long num=++req;
		while(avail<num) // waiting
			boost::thread::yield();
		T *ip=arr+(num-1)%arr_size;
		GML_MUTEX_LOCK();
		T ret=*(volatile T *)ip;
		GML_MEMBAR; // probably not needed, becase reordering is not possible
		*(volatile T *)ip=0;
		GML_MUTEX_UNLOCK();
		return ret;
	}
};


template<class T, class S, class C>
class gmlMultiItemServer {
	C genfun;
	gmlCount req;
	gmlCount avail;
	int pregen;
	int arr_size;
	T *arr;
	GML_MUTEX;
	
public:
	gmlMultiItemServer(C gf, int sz, int pg):req(0),avail(0) {
		genfun=gf;
		pregen=pg;
		arr_size=sz;
		arr=new T[arr_size];
		memset(arr,0,arr_size*sizeof(T));
	}
	
	virtual ~gmlMultiItemServer() {
		delete [] arr;
	}
	
	inline void GenerateItems() {
		int i;		
		while(avail<req+pregen && arr[i=(avail%arr_size)]==0) {
			T val;
			(*genfun)(1,&val);
			GML_MUTEX_LOCK();
			*(volatile T *)(arr+i)=val;
			GML_MEMBAR; // perhaps not needed, because ++avail acts as a barrier
			GML_MUTEX_UNLOCK();
			++avail;
		}
	}
	
	inline void GetItems(S n, T *data) {
		GML_IF_SERVER_THREAD() {
			(*genfun)(n,data);
			return;
		}
		gmlItemsConsumed+=n;
		for(int i=0; i<n; ++i) {
			long num=++req;
			while(avail<num) // waiting
				boost::thread::yield();
			T *ip=arr+(num-1)%arr_size;
			GML_MUTEX_LOCK();
			data[i]=*(volatile T *)ip;
			GML_MEMBAR; // probably not needed, becase reordering is not possible
			*(volatile T *)ip=0;
			GML_MUTEX_UNLOCK();
		}
	}
};


// Circular Queue - a "crash free" queue because it wraps around and keeps array index within bounds
#include "creg/creg_cond.h"

template<class T,int S>
class gmlCircularQueue {
	CR_DECLARE_STRUCT(gmlCircularQueue);
	T elements[S+1];
	size_t front,back;
	size_t csize,msize;
public:
	gmlCircularQueue(): front(0), back(0), csize(0), msize(S) {
	}
	~gmlCircularQueue() {
	}
	void push_back(T &a) {
		elements[back] = a;
		if(csize == msize) {
			if(front == msize)
				front = 0;
			else
				++front;
		}
		else
			++csize;
		if(back == msize)
			back = 0;
		else
			++back;
	}
	void push_front(T &a) {
		int newfront = (front == 0) ? msize : front - 1;
		elements[newfront] = a;
		front = newfront;
		if(csize != msize)
			++csize;
		else if(back == 0)
			back = msize;
		else
			--back;
	}
	T &pop_back() {
#ifdef _DEBUG
		assert(csize != 0);
#endif
		--csize;
		if(back == 0)
			back = msize;
		else
			--back;
		return elements[back];
	}
	T &pop_front() {
#ifdef _DEBUG
		assert(csize != 0);
#endif
		--csize;
		T &ret = elements[front];
		if(front == msize)
			front = 0;
		else
			++front;
		return ret;
	}
	volatile size_t size() {
		return *(volatile size_t *)&csize;
	}
	T &operator[](size_t i) {
		return elements[(front + i) % (msize + 1)];
	}
	bool empty() {
		return csize == 0;
	}
	void clear() {
		csize = 0;
		back = front;
	}
	void resize(size_t i) {
		if(i > msize)
			i = msize;
		csize = i;
		back = (front + i) % (msize + 1);
	}

template<class U>
	class CQIter {
		size_t p;
		gmlCircularQueue *q;
	public:
		CQIter() {} 
		CQIter(size_t d, gmlCircularQueue *r) {p=d; q=r;}
		void operator=(const CQIter<U> &i) {p=i.p;}
		CQIter<U> &operator++() {++p; return *this;} 
		CQIter<U> operator++(int) {return CQIter<U>(p++);} 
		int operator!=(const CQIter<U> &i) const {return p<i.p;}
		U &operator*() {return (*q)[p];}
		U *operator->() {return &(*q)[p];}
	};
	typedef CQIter<T> iterator;

	iterator begin() {
		return iterator(front, this);
	}
	iterator end() {
		return iterator(front + csize, this);
	}
};




#endif
