/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTIME_H
#define SPRINGTIME_H

#include "System/creg/creg_cond.h"

#include <boost/cstdint.hpp>
typedef boost::int64_t int64_t;

// glibc's chrono is non monotonic/not steady atm (it depends on set timezone and can change at runtime!)
// we don't want to special handles all the problems caused by this, so just use boost one instead
#define FORCE_BOOST_CHRONO

// mingw doesn't support std::thread (yet?)
#if (__cplusplus > 199711L) && !defined(__MINGW32__) && !defined(FORCE_BOOST_CHRONO)
	#define SPRINGTIME_USING_STDCHRONO
	#include <chrono>
	#include <thread>
	namespace chrono { using namespace std::chrono; };
	namespace this_thread { using namespace std::this_thread; };
#else
	#define SPRINGTIME_USING_BOOST
	#undef gt
	#include <boost/chrono/include.hpp>
	#include <boost/thread/thread.hpp>
	namespace chrono { using namespace boost::chrono; };
	namespace this_thread { using namespace boost::this_thread; };
#endif



struct Cpp11Clock {
	#define i1e3 1000LL
	#define i1e6 1000000LL
	#define i1e9 1000000000LL

	static inline int64_t ToSecs(const int64_t x) { return x / i1e9; }
	static inline int64_t ToMs(const int64_t x)   { return x / i1e6; }
	template<typename T> static inline T ToSecs(const int64_t x) { return int(x / i1e6) * 1e-3; }
	template<typename T> static inline T ToMs(const int64_t x)   { return int(x / i1e3) * 1e-3; }
	template<typename T> static inline T ToNs(const int64_t x)   { return x; }
	template<typename T> static inline int64_t FromSecs(const T s) { return s * i1e9; }
	template<typename T> static inline int64_t FromMs(const T ms)  { return ms * i1e6; }
	template<typename T> static inline int64_t FromNs(const T ns)  { return ns; }
	static inline int64_t Get() {
		return chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
	}
};



// class Timer
struct spring_time {
private:
	CR_DECLARE_STRUCT(spring_time);

public:
	inline static spring_time gettime() { return spring_time_native(Cpp11Clock::Get()); }
	inline static spring_time fromNanoSecs(const int64_t ns) { return spring_time_native(Cpp11Clock::FromNs(ns)); }

public:
	spring_time() : x(0) {}
	template<typename T> explicit spring_time(const T ms) : x(Cpp11Clock::FromMs(ms)) {}

	spring_time& operator+=(const spring_time v)       { x += v.x; return *this; }
	spring_time& operator-=(const spring_time v)       { x -= v.x; return *this; }
	spring_time   operator-(const spring_time v) const { return spring_time_native(x - v.x); }
	spring_time   operator+(const spring_time v) const { return spring_time_native(x + v.x); }
	bool          operator<(const spring_time v) const { return (x < v.x); }
	bool          operator>(const spring_time v) const { return (x > v.x); }
	bool         operator<=(const spring_time v) const { return (x <= v.x); }
	bool         operator>=(const spring_time v) const { return (x >= v.x); }


	inline int toSecs()         const { return toSecs<int>(); }
	inline int toMilliSecs()    const { return toMilliSecs<int>(); }
	inline int toNanoSecs()     const { return toNanoSecs<int>(); }
	inline float toSecsf()      const { return toSecs<float>(); }
	inline float toMilliSecsf() const { return toMilliSecs<float>(); }
	inline float toNanoSecsf()  const { return toNanoSecs<float>(); }
	template<typename T> inline T toSecs()      const { return Cpp11Clock::ToSecs<T>(x); }
	template<typename T> inline T toMilliSecs() const { return Cpp11Clock::ToMs<T>(x); }
	template<typename T> inline T toNanoSecs()  const { return Cpp11Clock::ToNs<T>(x); }


	inline bool isTime() const { return (x > 0); }
	inline void sleep() const {
	#if defined(SPRINGTIME_USING_STDCHRONO)
		this_thread::sleep_for(chrono::nanoseconds( toNanoSecs() ));
	#else
		boost::this_thread::sleep(boost::posix_time::milliseconds(toMilliSecs()));
	#endif
	}

	//inline void sleep_until() const {
	//	this_thread::sleep_until(chrono::nanoseconds( toNanoSecs() ));
	//}

private:
	inline static spring_time spring_time_native(const int64_t n) { spring_time s; s.x = n; return s; }

	void Serialize(creg::ISerializer& s);
	boost::int64_t x;
};



static const spring_time spring_notime(0);
static const spring_time spring_nulltime(0);
static const spring_time spring_starttime = spring_time::gettime();

#define spring_now() (spring_time::gettime() - spring_starttime)
#define spring_gettime() spring_time::gettime()

#define spring_tomsecs(t) ((t).toMilliSecs())
#define spring_istime(t) ((t).isTime())
#define spring_sleep(t) ((t).sleep())

#define spring_msecs(msecs) spring_time(msecs)
#define spring_secs(secs) spring_time((secs) * 1000)





#define spring_difftime(now, before)  (now - before)
#define spring_diffsecs(now, before)  (spring_tomsecs(now - before) * 0.001f)
#define spring_diffmsecs(now, before) (spring_tomsecs(now - before))

#endif // SPRINGTIME_H
