/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTIME_H
#define SPRINGTIME_H

#include "System/creg/creg_cond.h"

#include <boost/cstdint.hpp>

// glibc's chrono is non monotonic/not steady atm (it depends on set timezone and can change at runtime!)
// we don't want to specially handle all the problems caused by this, so just use boost version instead
// not possible either: boost::chrono uses extremely broken HPE timers on Windows 7 (but so does std::)
//
// #define FORCE_HIGHRES_TIMERS
// #define FORCE_BOOST_CHRONO

#if (__cplusplus > 199711L) && !defined(FORCE_BOOST_CHRONO)
	#define SPRINGTIME_USING_STDCHRONO
	#undef gt
	#include <chrono>
	namespace chrono { using namespace std::chrono; };
#else
	#define SPRINGTIME_USING_BOOST
	#undef gt
	#include <boost/chrono/include.hpp>
	namespace chrono { using namespace boost::chrono; };
#endif



namespace Cpp11Clock {
	#define i1e3 1000LL
	#define i1e6 1000000LL
	#define i1e9 1000000000LL

	// NOTE:
	//   1e-x are double-precision literals but T can be float
	//   floats only provide ~6 decimal digits of precision so
	//   ToSecs is inaccurate in that case
	template<typename T> static T ToSecs     (const boost::int64_t x) { return (x * 1e-9); }
	template<typename T> static T ToMilliSecs(const boost::int64_t x) { return (x * 1e-6); }
	template<typename T> static T ToMicroSecs(const boost::int64_t x) { return (x * 1e-3); }
	template<typename T> static T ToNanoSecs (const boost::int64_t x) { return (x       ); }

	// specializations
	template<> boost::int64_t ToSecs     <boost::int64_t>(const boost::int64_t x) { return (x / i1e9); }
	template<> boost::int64_t ToMilliSecs<boost::int64_t>(const boost::int64_t x) { return (x / i1e6); }
	template<> boost::int64_t ToMicroSecs<boost::int64_t>(const boost::int64_t x) { return (x / i1e3); }

	template<typename T> static boost::int64_t FromSecs     (const T      s) { return (     s * i1e9); }
	template<typename T> static boost::int64_t FromMilliSecs(const T millis) { return (millis * i1e6); }
	template<typename T> static boost::int64_t FromMicroSecs(const T micros) { return (micros * i1e3); }
	template<typename T> static boost::int64_t FromNanoSecs (const T  nanos) { return ( nanos       ); }

	// number of ticks since clock epoch
	boost::int64_t Get();

	const char* GetName();
};



// class Timer
struct spring_time {
private:
	CR_DECLARE_STRUCT(spring_time);

public:
	spring_time(): x(0) {}
	template<typename T> explicit spring_time(const T millis): x(Cpp11Clock::FromMilliSecs(millis)) {}

	spring_time& operator+=(const spring_time st)       { x += st.x; return *this; }
	spring_time& operator-=(const spring_time st)       { x -= st.x; return *this; }
	spring_time   operator-(const spring_time st) const { return spring_time_native(x - st.x); }
	spring_time   operator+(const spring_time st) const { return spring_time_native(x + st.x); }
	bool          operator<(const spring_time st) const { return (x <  st.x); }
	bool          operator>(const spring_time st) const { return (x >  st.x); }
	bool         operator<=(const spring_time st) const { return (x <= st.x); }
	bool         operator>=(const spring_time st) const { return (x >= st.x); }

	// short-hands (FIXME: int64 values forced into int32's??)
	int toSecsi()        const { return (toSecs     <boost::int64_t>()); }
	int toMilliSecsi()   const { return (toMilliSecs<boost::int64_t>()); }
	int toMicroSecsi()   const { return (toMicroSecs<boost::int64_t>()); }
	int toNanoSecsi()    const { return (toNanoSecs <boost::int64_t>()); }

	float toSecsf()      const { return (toSecs     <float>()); }
	float toMilliSecsf() const { return (toMilliSecs<float>()); }
	float toMicroSecsf() const { return (toMicroSecs<float>()); }
	float toNanoSecsf()  const { return (toNanoSecs <float>()); }

	// wrappers
	template<typename T> T toSecs()      const { return Cpp11Clock::ToSecs     <T>(x); }
	template<typename T> T toMilliSecs() const { return Cpp11Clock::ToMilliSecs<T>(x); }
	template<typename T> T toMicroSecs() const { return Cpp11Clock::ToMicroSecs<T>(x); }
	template<typename T> T toNanoSecs()  const { return Cpp11Clock::ToNanoSecs <T>(x); }


	bool isDuration() const { return (x != 0); }
	bool isTime() const { return (x > 0); }

	void sleep();
	void sleep_until();


	static spring_time gettime() { assert(xs != 0); return spring_time_native(Cpp11Clock::Get()); }
	static spring_time getstarttime() { assert(xs != 0); return spring_time_native(xs); }
	static spring_time getelapsedtime() { return (gettime() - getstarttime()); }

	static void setstarttime(const spring_time t) { assert(xs == 0); xs = t.x; assert(xs != 0); }

	static spring_time fromNanoSecs (const boost::int64_t  nanos) { return spring_time_native(Cpp11Clock::FromNanoSecs(  nanos)); }
	static spring_time fromMicroSecs(const boost::int64_t micros) { return spring_time_native(Cpp11Clock::FromMicroSecs(micros)); }
	static spring_time fromMilliSecs(const boost::int64_t millis) { return spring_time_native(Cpp11Clock::FromMilliSecs(millis)); }
	static spring_time fromSecs     (const boost::int64_t      s) { return spring_time_native(Cpp11Clock::FromSecs     (     s)); }

private:
	// convert integer to spring_time (n is interpreted as number of nanoseconds)
	static spring_time spring_time_native(const boost::int64_t n) { spring_time s; s.x = n; return s; }

	void Serialize(creg::ISerializer& s);

private:
	boost::int64_t x;

	// initial time (the "Spring epoch", program start)
	// all other time-points *must* be larger than this
	// if the clock is monotonically increasing
	static boost::int64_t xs;
};



static const spring_time spring_notime(0);
static const spring_time spring_nulltime(0);

#define spring_gettime()      spring_time::gettime()
#define spring_getstarttime() spring_time::getstarttime()
#define spring_now()          spring_time::getelapsedtime()

#define spring_tomsecs(t) ((t).toMilliSecsi())
#define spring_istime(t) ((t).isTime())
#define spring_sleep(t) ((t).sleep())

#define spring_msecs(msecs) spring_time(msecs)
#define spring_secs(secs) spring_time((secs) * 1000)





#define spring_difftime(now, before)  (now - before)
#define spring_diffsecs(now, before)  (spring_tomsecs(now - before) * 0.001f)
#define spring_diffmsecs(now, before) (spring_tomsecs(now - before))

#endif // SPRINGTIME_H
