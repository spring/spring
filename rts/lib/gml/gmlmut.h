// GML - OpenGL Multithreading Library
// for Spring http://springrts.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

// MUTEX TYPES:
// xxx below refers to a mutex named named xxxmutex, declared in gml.cpp and also below as extern
// If you want to protect a newly introduced shared resource, you may need to declare a new mutex
// To protect access to an existing shared resource, check if an existing named mutex is used to protect it in other locations, and use that same one
//
// Basic mutex lock types
// GML_STDMUTEX_LOCK(xxx) - scope locks the specified standard (non recursive, boost::mutex) mutex
// GML_RECMUTEX_LOCK(xxx) - scope locks the specified recursive (boost::recursive_mutex) mutex
//
// Specialized mutex lock types - they are rarely needed
// GML_MSTMUTEX_LOCK(xxx) - scope locks the specified manually lockable/unlockable (gmlMutex) mutex
// GML_MSTMUTEX_DOLOCK(xxx) - locks the specified manually lockable/unlockable (gmlMutex) mutex
// GML_MSTMUTEX_DOUNLOCK(xxx) - unlocks the specified manually lockable/unlockable (gmlMutex) mutex
// GML_THRMUTEX_LOCK(xxx,thr) - scope locks the specified recursive (boost::recursive_mutex) mutex depending on if the current thread matches thr = GML_SIM, GML_DRAW or GML_SIM|GML_DRAW
// GML_OBJMUTEX_LOCK(xxx,thr,arg) - scope locks the specified recursive (boost::recursive_mutex) mutex depending on if the current thread matches thr = GML_SIM, GML_DRAW or GML_SIM|GML_DRAW, arg will be prefixed to xxx
// GML_LODMUTEX_LOCK(xxx) - shorthand for GML_OBJMUTEX_LOCK(lod, GML_DRAW|GML_SIM, xxx->), i.e. will lock xxx->lodmutex for sim/draw threads
// GML_DRCMUTEX_LOCK(xxx) - shorthand for GML_MEASURE_LOCK_TIME(GML_OBJMUTEX_LOCK(xxx, GML_DRAW|GML_SIM, *L->)), i.e. will lock *L->xxxmutex for sim/draw threads and measure the locking overhead
// GML_LINEMUTEX_LOCK(...) - special mutex lock replacement for profiling only
// GML_PROFMUTEX_LOCK(...) - special mutex lock replacement for profiling only

#ifndef GMLMUTEX_H
#define GMLMUTEX_H

#define GML_DRAW 1
#define GML_SIM 2

#ifdef USE_GML

#include "gmlcnf.h"

#if GML_ENABLE_SIM
#include <boost/thread/mutex.hpp>
extern boost::mutex caimutex;
extern boost::mutex decalmutex;
extern boost::mutex treemutex;
extern boost::mutex mapmutex;
extern boost::mutex inmapmutex;
extern boost::mutex tempmutex;
extern boost::mutex posmutex;
extern boost::mutex runitmutex;
extern boost::mutex netmutex;
extern boost::mutex histmutex;
extern boost::mutex timemutex;
extern boost::mutex watermutex;
extern boost::mutex dquemutex;
extern boost::mutex scarmutex;
extern boost::mutex trackmutex;
extern boost::mutex rprojmutex;
extern boost::mutex rflashmutex;
extern boost::mutex rpiecemutex;
extern boost::mutex rfeatmutex;
extern boost::mutex drawmutex;
extern boost::mutex scallmutex;
extern boost::mutex ulbatchmutex;
extern boost::mutex flbatchmutex;
extern boost::mutex olbatchmutex;
extern boost::mutex plbatchmutex;
extern boost::mutex glbatchmutex;
extern boost::mutex mlbatchmutex;
extern boost::mutex llbatchmutex;
extern boost::mutex cmdmutex;
extern boost::mutex xcallmutex;
extern boost::mutex blockmutex;
extern boost::mutex tnummutex;
extern boost::mutex ntexmutex;
extern boost::mutex catmutex;
extern boost::mutex grpchgmutex;
extern boost::mutex laycmdmutex;

#include <boost/thread/recursive_mutex.hpp>
extern boost::recursive_mutex unitmutex;
extern boost::recursive_mutex quadmutex;
extern boost::recursive_mutex selmutex;
extern boost::recursive_mutex featmutex;
extern boost::recursive_mutex grassmutex;
extern boost::recursive_mutex &guimutex;
extern boost::recursive_mutex filemutex;
extern boost::recursive_mutex &qnummutex;
extern boost::recursive_mutex &groupmutex;
extern boost::recursive_mutex &grpselmutex;
extern boost::recursive_mutex projmutex;
extern boost::recursive_mutex objmutex;
extern boost::recursive_mutex modelmutex;
extern boost::recursive_mutex cammutex;

#include "gmlcls.h"

extern gmlMutex simmutex;

#if GML_MUTEX_PROFILER
#	include "System/TimeProfiler.h"
#define GML_MUTEX_TYPE(name, type) boost::type::scoped_lock name##lock(name##mutex)
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
				new (st##name) ScopedTimer(" " GML_QUOTE(name ## line ## Mutex));\
			type;\
			if(stc##name)\
				((ScopedTimer *)st##name)->~ScopedTimer()
#		define GML_PROFMUTEX_LOCK(name, type, line) GML_LINEMUTEX_LOCK(name, type, line)
#		define GML_STDMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, GML_MUTEX_TYPE(name, mutex), __LINE__)
#		define GML_RECMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, GML_MUTEX_TYPE(name, recursive_mutex), __LINE__)
#		define GML_THRMUTEX_LOCK(name,thr) GML_PROFMUTEX_LOCK(name, gmlRecursiveScopedLock name##lock(name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())), __LINE__)
#		define GML_OBJMUTEX_LOCK(name,thr,...) GML_PROFMUTEX_LOCK(name, gmlRecursiveScopedLock name##lock(__VA_ARGS__ name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())), __LINE__)
#	else
#		define GML_PROFMUTEX_LOCK(name, type)\
			char st##name[sizeof(ScopedTimer)];\
			new (st##name) ScopedTimer(" " GML_QUOTE(name##Mutex));\
			type;\
			((ScopedTimer *)st##name)->~ScopedTimer()
#		define GML_STDMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, GML_MUTEX_TYPE(name, mutex))
#		define GML_RECMUTEX_LOCK(name) GML_PROFMUTEX_LOCK(name, GML_MUTEX_TYPE(name, recursive_mutex))
#		define GML_THRMUTEX_LOCK(name,thr) GML_PROFMUTEX_LOCK(name, gmlRecursiveScopedLock name##lock(name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())))
#		define GML_OBJMUTEX_LOCK(name,thr,...) GML_PROFMUTEX_LOCK(name, gmlRecursiveScopedLock name##lock(__VA_ARGS__ name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())))
#	endif
#else
#	define GML_STDMUTEX_LOCK(name) gmlMutexScopedLock name##lock(name##mutex)
#if GML_DEBUG_MUTEX
extern std::map<std::string, int> lockmaps[GML_MAX_NUM_THREADS];
extern std::map<boost::recursive_mutex *, int> lockmmaps[GML_MAX_NUM_THREADS];
extern boost::mutex lmmutex;
class gmlRecursiveMutexLockDebug:public gmlRecursiveMutexScopedLock {
public:
	std::string s1;
	gmlRecursiveMutexLockDebug(boost::recursive_mutex &m, std::string s) : s1(s), gmlRecursiveMutexScopedLock(m) {
		GML_STDMUTEX_LOCK(lm);
		std::map<std::string, int> &lockmap = lockmaps[gmlThreadNumber];
		std::map<std::string, int>::iterator locki = lockmap.find(s);
		if(locki == lockmap.end())
			lockmap[s1] = 1;
		else
			lockmap[s1] = (*locki).second + 1;
	}
	virtual ~gmlRecursiveMutexLockDebug() {
		GML_STDMUTEX_LOCK(lm);
		std::map<std::string, int> &lockmap = lockmaps[gmlThreadNumber];
		lockmap[s1] = (*lockmap.find(s1)).second - 1;
	}
};
#	define GML_RECMUTEX_LOCK(name) gmlRecursiveMutexLockDebug name##lock(name##mutex, #name)
#	define GML_THRMUTEX_LOCK(name,thr) gmlRecursiveScopedLock name##lock(name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())) // TODO
#	define GML_OBJMUTEX_LOCK(name,thr,...) gmlRecursiveScopedLock name##lock(__VA_ARGS__ name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread())) // TODO

#else
#	define GML_RECMUTEX_LOCK(name) gmlRecursiveMutexScopedLock name##lock(name##mutex)
#	define GML_THRMUTEX_LOCK(name,thr) gmlRecursiveScopedLock name##lock(name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread()))
#	define GML_OBJMUTEX_LOCK(name,thr,...) gmlRecursiveScopedLock name##lock(__VA_ARGS__ name##mutex, (((thr)&GML_DRAW) && ((thr)&GML_SIM)) || (((thr)&GML_DRAW) && !Threading::IsSimThread()) || (((thr)&GML_SIM) && Threading::IsSimThread()))
#endif
#endif
class gmlMutexLock {
	gmlMutex &mtx;
public:
	gmlMutexLock(gmlMutex &m, int wait = 0) : mtx(m) {
		mtx.Lock(wait);
	}
	virtual ~gmlMutexLock() { mtx.Unlock(); }
};
#define GML_MSTMUTEX_LOCK(name, ...) gmlMutexLock name##mutexlock(name##mutex, __VA_ARGS__)
#define GML_MSTMUTEX_DOLOCK(name) name##mutex.Lock()
#define GML_MSTMUTEX_DOUNLOCK(name) name##mutex.Unlock()
#define GML_STDMUTEX_LOCK_NOPROF(name) gmlMutexScopedLock name##lock(name##mutex)

#endif

#endif

#if !defined(USE_GML) || !GML_ENABLE_SIM

#define GML_STDMUTEX_LOCK(name)
#define GML_RECMUTEX_LOCK(name)
#define GML_THRMUTEX_LOCK(name,thr)
#define GML_OBJMUTEX_LOCK(name,thr,...)
#define GML_STDMUTEX_LOCK_NOPROF(name)
#define GML_MSTMUTEX_LOCK(name, ...)
#define GML_MSTMUTEX_DOLOCK(name)
#define GML_MSTMUTEX_DOUNLOCK(name)

#endif

#define GML_LODMUTEX_LOCK(unit) GML_OBJMUTEX_LOCK(lod, GML_DRAW|GML_SIM, unit->)

#endif

