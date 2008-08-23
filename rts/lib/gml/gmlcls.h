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
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/version.hpp>
#define private public // we need access to the private value_ member of atomic_count
#include <boost/detail/atomic_count.hpp>
#undef private
#include <map>
#include <set>

#ifdef USE_GML
#define GML_ENABLE 1
#else
#define GML_ENABLE 0 // manually enable opengl multithreading here
#endif

#define GML_ENABLE_SIM (GML_ENABLE && 1) // runs the sim in a separate thread
#define GML_ENABLE_DRAWALL (GML_ENABLE && 0) // draws everything in a separate thread (for testing only, will degrade performance)
#define GML_ENABLE_DRAWUNIT (GML_ENABLE && 1) // draws units using multithreading
#define GML_ENABLE_DRAWUNITSHADOW (GML_ENABLE && 1) // draws unit shadows using multithreading
#define GML_ENABLE_DRAWGROUND (GML_ENABLE && 1) // draws ground using multithreading
#define GML_ENABLE_DRAWGROUNDSHADOW (GML_ENABLE && 1) // draws ground shadows using multithreading
#define GML_SERVER_GLCALL 1 // allow the server thread (0) to make direct GL calls
#define GML_INIT_QUEUE_SIZE 10 // initial queue size, will be reallocated, but must be >= 1
#define GML_USE_NO_ERROR 1 // glGetError always returns success (to improve performance)
#define GML_USE_DEFAULT 1// compile/link/buffer status always returns TRUE/COMPLETE (to improve performance)
#define GML_USE_CACHE 1 // certain glGet calls may use data cached during gmlInit (to improve performance)
//#define GML_USE_QUADRIC_SERVER 1 // use server thread to create/delete quadrics
#define GML_UPDSRV_INTERVAL 10
#define GML_ALTERNATE_SYNCMODE 1 // mutex-protected synced execution, slower but more portable

#ifdef _MSC_VER
#define GML_ORDERED_VOLATILE (_MSC_VER >= 1400) 
#else
#define GML_ORDERED_VOLATILE 0
#endif
// optimize by assuming volatile accesses are
// guaranteed not to be reordered (MSVS 2005 ONLY)
// http://msdn.microsoft.com/en-us/library/12a04hfd(VS.80).aspx
// http://msdn.microsoft.com/en-us/library/ms686355(VS.85).aspx
// http://msdn.microsoft.com/en-us/library/bb310595(VS.85).aspx

#if GML_ORDERED_VOLATILE
#define GML_VOLATILE(x) *(x volatile *)&
#define GML_MUTEX
#define GML_MUTEX_LOCK()
#define GML_MUTEX_UNLOCK()
#else
#define GML_VOLATILE(x)
#define GML_MUTEX gmlMutex mutex
#define GML_MUTEX_LOCK() mutex.Lock()
#define GML_MUTEX_UNLOCK() mutex.Unlock()
#endif

#ifdef _MSC_VER
#define GML_TYPENAME typename
#else
#define GML_TYPENAME
#endif

#if GML_ENABLE
#ifdef _MSC_VER
extern __declspec(thread) int gmlThreadNumber;
#else
extern __thread int gmlThreadNumber;
#endif
#else
extern int gmlThreadNumber;
#endif
extern int gmlThreadCount;
extern int gmlThreadCountOverride;
extern unsigned gmlCPUCount();
#define GML_CPU_COUNT (gmlThreadCountOverride?gmlThreadCountOverride:gmlCPUCount())
#define GML_MAX_NUM_THREADS 32
#define GML_IF_SERVER_THREAD() if(GML_SERVER_GLCALL && (!GML_ENABLE || gmlThreadNumber==0))
extern int gmlItemsConsumed;

typedef unsigned char BYTE;
typedef int BOOL_;

#define TRUE 1
#define FALSE 0
#define EXTERN
#define GML_VP_ARRAY_BUFFER (1<<(16+GL_VERTEX_ARRAY-GL_VERTEX_ARRAY)) //(1<<20)
#define GML_CP_ARRAY_BUFFER (1<<(16+GL_COLOR_ARRAY-GL_VERTEX_ARRAY)) //(1<<21)
#define GML_TCP_ARRAY_BUFFER (1<<(16+GL_TEXTURE_COORD_ARRAY-GL_VERTEX_ARRAY)) //(1<<22)
#define GML_IP_ARRAY_BUFFER (1<<(16+GL_INDEX_ARRAY-GL_VERTEX_ARRAY)) //(1<<23)
#define GML_NP_ARRAY_BUFFER (1<<(16+GL_NORMAL_ARRAY-GL_VERTEX_ARRAY)) //(1<<24)
#define GML_EFP_ARRAY_BUFFER (1<<(16+GL_EDGE_FLAG_ARRAY-GL_VERTEX_ARRAY)) //(1<<25)
#define GML_ELEMENT_ARRAY_BUFFER (1<<29)

#if defined(_WIN32)
#	if defined(__MINGW32__)
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

#define GML_ERROR(str,val)\
	FILE *f=fopen("C:\\GMLERR.TXT","a");\
	if(f) {\
		fprintf(f,"%s line %d: %s %d\n",__FILE__,__LINE__,str,val);\
		fclose(f);\
	}\
Sleep(1000);

#define GML_DEBUG(str,val)\
	FILE *f=fopen("C:\\GMLDBG.TXT","a");\
	if(f) {\
		fprintf(f,"%s line %d: %s %d\n",__FILE__,__LINE__,str,val);\
		fclose(f);\
	}

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

#if !defined(BOOST_HAS_THREADS) || (!defined(BOOST_AC_USE_PTHREADS) && (BOOST_VERSION<103500 || !(defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)))
#define gmlCount boost::detail::atomic_count
#else
class gmlCount : public boost::detail::atomic_count {
public:
	gmlCount(long val):boost::detail::atomic_count(val) {
	}

	virtual ~gmlCount() {}

	long operator++() {
	#if defined(BOOST_AC_USE_PTHREADS)
		scoped_lock lock(mutex_);
		return ++value_;
	#elif (BOOST_VERSION>=103500) && (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
		return atomic_exchange_and_add(&value_, 1)+1;
//	#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
//		return BOOST_INTERLOCKED_INCREMENT(&value_);
	#elif (BOOST_VERSION>=103500) && (defined(__GNUC__) && (__GNUC__*100+__GNUC_MINOR__>=401))
		return __sync_add_and_fetch(&value_, 1);
	#elif defined(__GLIBCXX__)
		return __gnu_cxx::__exchange_and_add(&value_, 1)+1;
	#elif defined(__GLIBCPP__)
		return __exchange_and_add(&value_, 1)+1;
	#elif defined(BOOST_HAS_PTHREADS)
		#define BOOST_AC_USE_PTHREADS
		scoped_lock lock(mutex_);
		return ++value_;
	#else
		#error Unrecognized threading platform
	#endif
	}
};
#endif

// this hack will assign the counter of a boost atomic_count object
inline void operator%=(gmlCount& a, long val) {
	a.value_=val;
/*#ifdef BOOST_AC_USE_PTHREADS
	a.~gmlCount();
#endif
	new (&a) gmlCount(val);*/
}

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
							new (da+sz-1) T();
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
				new (da+sz-1) T(d);
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
	
	gmlVector<T> &operator=(const GML_TYPENAME gmlVector<T> &vec) {
		free(data);
		memcpy(this,&vec,sizeof(GML_TYPENAME gmlVector<T>));
		data=(T *)malloc(vec.maxsize*sizeof(T));
		memcpy(data,vec.data,vec.maxsize*sizeof(T));
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
#if (BOOST_VERSION >= 103500)
		new (lock) boost::try_mutex::scoped_try_lock(sl_mutex,boost::try_to_lock);
		if(lock->owns_lock())
#else
		new (lock) boost::try_mutex::scoped_try_lock(sl_mutex);
		if(lock->locked())
#endif
			return true;
#if (BOOST_VERSION >= 103500)
		lock->~unique_lock();
#else
		lock->~scoped_try_lock();
#endif
		return false;
	}
	void Unlock() {
#if (BOOST_VERSION >= 103500)
		(((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber)->~unique_lock();
#else
		(((boost::try_mutex::scoped_try_lock *)sl_lock)+gmlThreadNumber)->~scoped_try_lock();
#endif
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
	GLvoid *VPpointer;
	// ColorPointer
	GLint CPsize;
	GLenum CPtype;
	GLsizei CPstride;
	GLvoid *CPpointer;
	// EdgeFlagPointer
	GLsizei EFPstride;
	GLboolean *EFPpointer;
	// IndexPointer
	GLenum IPtype;
	GLsizei IPstride;
	GLvoid *IPpointer;
	// NormalPointer
	GLenum NPtype;
	GLsizei NPstride;
	GLvoid *NPpointer;
	// TexCoordPointer
	GLint TCPsize;
	GLenum TCPtype;
	GLsizei TCPstride;
	GLvoid *TCPpointer;

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
	void ExecuteSynced();
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
	T *large_size_arr;
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
		large_size_arr=new T[large_arr_size];
		memset(large_item_arr,0,large_arr_size*sizeof(T));
		memset(large_size_arr,0,large_arr_size*sizeof(T));
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
			GML_MUTEX_UNLOCK();
			++avail;
		}
		// large
		while(avail_large<req_large+pregen_large && large_size_arr[i=(avail_large%large_arr_size)]==0) {
			int gensize=size_large;
			GML_MUTEX_LOCK();
			*(volatile T *)(large_item_arr+i)=(*genfun)(gensize);
			*(volatile T *)(large_size_arr+i)=gensize;
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
			T *sz=large_size_arr+idx;
			T szv=*(volatile T *)sz;
			*(volatile T *)sz=0;
			GML_MUTEX_UNLOCK();
			if(szv>static_cast<T>(n))
				(*delfun)(ip+szv,szv-n); // del excessive
			if(szv<static_cast<T>(n))
				(*delfun)(ip,szv); // del all
			if(szv>=static_cast<T>(n))
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
			*(volatile T *)ip=0;
			GML_MUTEX_UNLOCK();
		}
	}
};

#endif
