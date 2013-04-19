/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGTIME_H
#define SPRINGTIME_H

#include "System/creg/creg_cond.h"
#include <boost/thread/thread.hpp>
#include <boost/cstdint.hpp>

typedef boost::int64_t int64_t;

#if __cplusplus > 199711L
	#include <chrono>
	namespace chrono { using namespace std::chrono; };
	namespace this_thread { using namespace std::this_thread; };
#else
	#define SPRINGTIME_USING_BOOST
	#undef gt
	#include <boost/chrono/include.hpp>
	namespace chrono { using namespace boost::chrono; };
	namespace this_thread { using namespace boost::this_thread; };
#endif



struct Cpp11Clock {
	#define i10E6 1000000LL
	#define i10E9 1000000000LL
	#define d10E6_rcp (1./1e6)
	#define d10E9_rcp (1./1e9)

	// Cause we limit FloatingPoint precision to 32bit it fails with our int64.
	// To preserve max precision for integer, we need to overload float-target.
	static inline float ToSecs(int64_t x) { return x * d10E9_rcp; }
	static inline float ToMs(int64_t x)   { return x * d10E6_rcp; }
	template<typename T> static inline T ToSecs(int64_t x) { return x * d10E9_rcp; }
	template<typename T> static inline T ToMs(int64_t x)   { return x * d10E6_rcp; }
	template<typename T> static inline T ToNs(int64_t x)   { return x; }
	static inline int64_t FromMs(int64_t ms) { return ms * i10E6; }
	static inline std::string GetName() { return "Cpp11Clock"; }
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

public:
	spring_time() : x(0) {}
	explicit spring_time(const int64_t ms) : x(Cpp11Clock::FromMs(ms)) {}

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
	#if (BOOST_VERSION < 105000) && defined(SPRINGTIME_USING_BOOST)
		// boost <1.50 was missing sleep_for
		boost::this_thread::sleep(boost::posix_time::milliseconds(toMilliSecs()));
	#else
		this_thread::sleep_for(chrono::nanoseconds( toNanoSecs() ));
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
