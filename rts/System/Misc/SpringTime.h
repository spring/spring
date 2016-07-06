/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTIME_H
#define SPRINGTIME_H

#include "System/creg/creg_cond.h"

#include <boost/cstdint.hpp>
#include <cassert>

// glibc's chrono is non monotonic/not steady atm (it depends on set timezone and can change at runtime!)
// we don't want to specially handle all the problems caused by this, so just use boost version instead
// not possible either: boost::chrono uses extremely broken HPE timers on Windows 7 (but so does std::)
//
// #define FORCE_CHRONO_TIMERS
// #define FORCE_BOOST_CHRONO

#if (__cplusplus > 199711L) && !defined(FORCE_BOOST_CHRONO) || defined(_MSC_VER)
	#define SPRINGTIME_USING_STDCHRONO
	#undef gt
	#include <chrono>
	namespace chrono { using namespace std::chrono; }
#else
	#define SPRINGTIME_USING_BOOST
	#undef gt
	#include <boost/chrono/include.hpp>
	namespace chrono { using namespace boost::chrono; };
#endif





namespace spring_clock {
	// NOTE:
	//   1e-x are double-precision literals but T can be float
	//   floats only provide ~6 decimal digits of precision so
	//   ToSecs is inaccurate in that case
	//   these cannot be written as integer divisions or tests
	//   will fail because of intermediate conversions to FP32
	template<typename T> static T ToSecs     (const boost::int64_t ns) { return (ns * 1e-9); }
	template<typename T> static T ToMilliSecs(const boost::int64_t ns) { return (ns * 1e-6); }
	template<typename T> static T ToMicroSecs(const boost::int64_t ns) { return (ns * 1e-3); }
	template<typename T> static T ToNanoSecs (const boost::int64_t ns) { return (ns       ); }

	// specializations
	template<> boost::int64_t ToSecs     <boost::int64_t>(const boost::int64_t ns) { return (ns / boost::int64_t(1e9)); }
	template<> boost::int64_t ToMilliSecs<boost::int64_t>(const boost::int64_t ns) { return (ns / boost::int64_t(1e6)); }
	template<> boost::int64_t ToMicroSecs<boost::int64_t>(const boost::int64_t ns) { return (ns / boost::int64_t(1e3)); }

	template<typename T> static boost::int64_t FromSecs     (const T  s) { return ( s * boost::int64_t(1e9)); }
	template<typename T> static boost::int64_t FromMilliSecs(const T ms) { return (ms * boost::int64_t(1e6)); }
	template<typename T> static boost::int64_t FromMicroSecs(const T us) { return (us * boost::int64_t(1e3)); }
	template<typename T> static boost::int64_t FromNanoSecs (const T ns) { return (ns                      ); }

	void PushTickRate(bool hres = false);
	void PopTickRate();

	// number of ticks since clock epoch
	boost::int64_t GetTicks();
	const char* GetName();
}



// class Timer
struct spring_time {
private:
	CR_DECLARE_STRUCT(spring_time)

	typedef boost::int64_t int64;

public:
	spring_time(): x(0) {}
	template<typename T> explicit spring_time(const T millis): x(spring_clock::FromMilliSecs(millis)) {}

	spring_time& operator+=(const spring_time st)       { x += st.x; return *this; }
	spring_time& operator-=(const spring_time st)       { x -= st.x; return *this; }
	spring_time& operator%=(const spring_time mt)       { x %= mt.x; return *this;    }
	spring_time   operator-(const spring_time st) const { return spring_time_native(x - st.x); }
	spring_time   operator+(const spring_time st) const { return spring_time_native(x + st.x); }
	spring_time   operator%(const spring_time mt) const { return spring_time_native(x % mt.x); }
	bool          operator<(const spring_time st) const { return (x <  st.x); }
	bool          operator>(const spring_time st) const { return (x >  st.x); }
	bool         operator<=(const spring_time st) const { return (x <= st.x); }
	bool         operator>=(const spring_time st) const { return (x >= st.x); }

	// short-hands
	int64 toSecsi()        const { return (toSecs     <int64>()); }
	int64 toMilliSecsi()   const { return (toMilliSecs<int64>()); }
	int64 toMicroSecsi()   const { return (toMicroSecs<int64>()); }
	int64 toNanoSecsi()    const { return (toNanoSecs <int64>()); }

	float toSecsf()      const { return (toSecs     <float>()); }
	float toMilliSecsf() const { return (toMilliSecs<float>()); }
	float toMicroSecsf() const { return (toMicroSecs<float>()); }
	float toNanoSecsf()  const { return (toNanoSecs <float>()); }

	// wrappers
	template<typename T> T toSecs()      const { return spring_clock::ToSecs     <T>(x); }
	template<typename T> T toMilliSecs() const { return spring_clock::ToMilliSecs<T>(x); }
	template<typename T> T toMicroSecs() const { return spring_clock::ToMicroSecs<T>(x); }
	template<typename T> T toNanoSecs()  const { return spring_clock::ToNanoSecs <T>(x); }


	bool isDuration() const { return (x != 0); }
	bool isTime() const { return (x > 0); }

	void sleep();
	void sleep_until();


	static spring_time gettime(bool init = false) { assert(xs != 0 || init); return spring_time_native(spring_clock::GetTicks()); }
	static spring_time getstarttime() { assert(xs != 0); return spring_time_native(xs); }
	static spring_time getelapsedtime() { return (gettime() - getstarttime()); }

	static void setstarttime(const spring_time t) { assert(xs == 0); xs = t.x; assert(xs != 0); }

	static spring_time fromNanoSecs (const int64 ns) { return spring_time_native(spring_clock::FromNanoSecs( ns)); }
	static spring_time fromMicroSecs(const int64 us) { return spring_time_native(spring_clock::FromMicroSecs(us)); }
	static spring_time fromMilliSecs(const int64 ms) { return spring_time_native(spring_clock::FromMilliSecs(ms)); }
	static spring_time fromSecs     (const int64  s) { return spring_time_native(spring_clock::FromSecs     ( s)); }

private:
	// convert integer to spring_time (n is interpreted as number of nanoseconds)
	static spring_time spring_time_native(const int64 n) { spring_time s; s.x = n; return s; }
	void Serialize(creg::ISerializer* s);

private:
	int64 x;

	// initial time (the "Spring epoch", program start)
	// all other time-points *must* be larger than this
	// if the clock is monotonically increasing
	static int64 xs;
};



static const spring_time spring_notime(0);
static const spring_time spring_nulltime(0);

//#define spring_gettime()      spring_time::gettime()
#define spring_gettime()      spring_time::getelapsedtime()
#define spring_getstarttime() spring_time::getstarttime()
#define spring_now()          spring_time::getelapsedtime()

#define spring_tomsecs(t) ((t).toMilliSecsi())
#define spring_istime(t) ((t).isTime())
#define spring_sleep(t) ((t).sleep())

#define spring_msecs(msecs) spring_time(msecs)
#define spring_secs(secs) spring_time((secs) * 1000)





#define spring_difftime(now, before)  (now - before)
#define spring_diffsecs(now, before)  ((now - before).toSecsi())
#define spring_diffmsecs(now, before) ((now - before).toMilliSecsi())

#endif // SPRINGTIME_H
