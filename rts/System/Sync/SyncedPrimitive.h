/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCEDPRIMITIVE_H
#define SYNCEDPRIMITIVE_H

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

#include "SyncChecker.h"  // for CSyncedPrimitiveBase (if SYNCCHECK is defined)
#ifdef SYNCDEBUG
	#include "SyncDebugger.h" // for CSyncedPrimitiveBase (if SYNCDEBUG is defined)
#endif // SYNCDEBUG
#include "SyncedPrimitiveBase.h"
#include "Upcast.h"       // for UPCAST macro

/*
Unfortunately I have to resort to preprocessor magic to expand all functions
for all C++ primitive types. Templates don't work because this confuses the
compiler, as that opens up too much possible conversions, and hence results
in errors like the following on GCC 4:

error: ISO C++ says that these are ambiguous, even though the worst conversion
       for the first is better than the worst conversion for the second:
note: candidate 1: operator==(int, int) <built-in>
note: candidate 2: bool operator==(T, SyncedPrimitive) [with T = short unsigned int]
*/
#ifdef UPCAST_USE_64_BIT_TYPES
#define FOR_EACH_PRIMITIVE_TYPE \
	DO(signed char) \
	DO(signed short) \
	DO(signed int) \
	DO(signed long) \
	DO(Sint64) \
	DO(unsigned char) \
	DO(unsigned short) \
	DO(unsigned int) \
	DO(unsigned long) \
	DO(Uint64) \
	DO(float) \
	DO(double) \
	DO(long double) \
	DO(bool)
#else // UPCAST_USE_64_BIT_TYPES
#define FOR_EACH_PRIMITIVE_TYPE \
	DO(signed char) \
	DO(signed short) \
	DO(signed int) \
	DO(signed long) \
	DO(unsigned char) \
	DO(unsigned short) \
	DO(unsigned int) \
	DO(unsigned long) \
	DO(float) \
	DO(double) \
	DO(long double) \
	DO(bool)
#endif // !UPCAST_USE_64_BIT_TYPES

/** \p SyncedPrimitive class. Variables of this type are automagically
downcasted to their value_type, preventing them to be implicitly used for
anything but carefully selected places. The goal of this class is to call
\p CSyncedPrimitiveBase::Sync() on each write to the data member \p x. */
template <class T> struct SyncedPrimitive : public CSyncedPrimitiveBase {
	T x;
	void Sync(const char* op) {CSyncedPrimitiveBase::Sync((void*)&x, sizeof(T), op);}
	SyncedPrimitive(): x(0) {}
	/* unary functions */
	bool operator!() const {return !x;}
	UPCAST(T,T) operator~() const {return ~x;}
	UPCAST(T,T) operator-() const {return -x;}
	/* prefix/postfix increment/decrement */
	UPCAST(T,T) operator++() {++x; Sync("++pre"); return x;}
	UPCAST(T,T) operator--() {--x; Sync("--pre"); return x;}
	UPCAST(T,T) operator++(int) {T r=x++; Sync("post++"); return r;}
	UPCAST(T,T) operator--(int) {T r=x--; Sync("post--"); return r;}
	/* assignment */
	template<class U> SyncedPrimitive(const SyncedPrimitive<U>& f): x(f.x) {Sync("copy");}
	template<class U> SyncedPrimitive& operator=(const SyncedPrimitive<U>& f) {x=f.x; Sync("="); return *this;}
	template<class U> SyncedPrimitive& operator+=(const SyncedPrimitive<U>& f) {x+=f.x; Sync("+="); return *this;}
	template<class U> SyncedPrimitive& operator-=(const SyncedPrimitive<U>& f) {x-=f.x; Sync("-="); return *this;}
	template<class U> SyncedPrimitive& operator*=(const SyncedPrimitive<U>& f) {x*=f.x; Sync("*="); return *this;}
	template<class U> SyncedPrimitive& operator/=(const SyncedPrimitive<U>& f) {x/=f.x; Sync("/="); return *this;}
	template<class U> SyncedPrimitive& operator%=(const SyncedPrimitive<U>& f) {x%=f.x; Sync("%="); return *this;}
	template<class U> SyncedPrimitive& operator&=(const SyncedPrimitive<U>& f) {x&=f.x; Sync("&="); return *this;}
	template<class U> SyncedPrimitive& operator|=(const SyncedPrimitive<U>& f) {x|=f.x; Sync("|="); return *this;}
	template<class U> SyncedPrimitive& operator^=(const SyncedPrimitive<U>& f) {x^=f.x; Sync("^="); return *this;}
	template<class U> SyncedPrimitive& operator<<=(const SyncedPrimitive<U>& f) {x<<=f.x; Sync("<<="); return *this;}
	template<class U> SyncedPrimitive& operator>>=(const SyncedPrimitive<U>& f) {x>>=f.x; Sync(">>="); return *this;}

#define DO(T) \
	SyncedPrimitive(T f): x(f) {Sync("copy" #T);} \
	SyncedPrimitive& operator=(const T f) {x=f; Sync("=" #T); return *this; } \
	SyncedPrimitive& operator+=(const T f) {x+=f; Sync("+=" #T); return *this; } \
	SyncedPrimitive& operator-=(const T f) {x-=f; Sync("-=" #T); return *this; } \
	SyncedPrimitive& operator*=(const T f) {x*=f; Sync("*=" #T); return *this; } \
	SyncedPrimitive& operator/=(const T f) {x/=f; Sync("/=" #T); return *this; } \
	SyncedPrimitive& operator%=(const T f) {x%=f; Sync("%=" #T); return *this; } \
	SyncedPrimitive& operator&=(const T f) {x&=f; Sync("&=" #T); return *this; } \
	SyncedPrimitive& operator|=(const T f) {x|=f; Sync("|=" #T); return *this; } \
	SyncedPrimitive& operator^=(const T f) {x^=f; Sync("^=" #T); return *this; } \
	SyncedPrimitive& operator<<=(const T f) {x<<=f; Sync("<<=" #T); return *this; } \
	SyncedPrimitive& operator>>=(const T f) {x>>=f; Sync(">>=" #T); return *this; }

	FOR_EACH_PRIMITIVE_TYPE
#undef DO

	operator T () const { return x; }
};

template<class U, class V> inline UPCAST(U,V) operator+(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x+g.x;}
template<class U, class V> inline UPCAST(U,V) operator-(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x-g.x;}
template<class U, class V> inline UPCAST(U,V) operator*(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x*g.x;}
template<class U, class V> inline UPCAST(U,V) operator/(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x/g.x;}
template<class U, class V> inline UPCAST(U,V) operator%(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x%g.x;}
template<class U, class V> inline UPCAST(U,V) operator&(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x&g.x;}
template<class U, class V> inline UPCAST(U,V) operator|(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x|g.x;}
template<class U, class V> inline UPCAST(U,V) operator^(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x^g.x;}
template<class U, class V> inline UPCAST(U,V) operator<<(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x<<g.x;}
template<class U, class V> inline UPCAST(U,V) operator>>(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x>>g.x;}
template<class U, class V> inline bool operator<(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x<g.x;}
template<class U, class V> inline bool operator>(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x>g.x;}
template<class U, class V> inline bool operator<=(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x<=g.x;}
template<class U, class V> inline bool operator>=(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x>=g.x;}
template<class U, class V> inline bool operator==(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x==g.x;}
template<class U, class V> inline bool operator!=(const SyncedPrimitive<U>& f, const SyncedPrimitive<V>& g) {return f.x!=g.x;}

#define DO(T) \
	template<class U> inline UPCAST(T,U) operator+(const SyncedPrimitive<U>& f, const T g) {return f.x+g;} \
	template<class U> inline UPCAST(T,U) operator-(const SyncedPrimitive<U>& f, const T g) {return f.x-g;} \
	template<class U> inline UPCAST(T,U) operator*(const SyncedPrimitive<U>& f, const T g) {return f.x*g;} \
	template<class U> inline UPCAST(T,U) operator/(const SyncedPrimitive<U>& f, const T g) {return f.x/g;} \
	template<class U> inline UPCAST(T,U) operator%(const SyncedPrimitive<U>& f, const T g) {return f.x%g;} \
	template<class U> inline UPCAST(T,U) operator&(const SyncedPrimitive<U>& f, const T g) {return f.x&g;} \
	template<class U> inline UPCAST(T,U) operator|(const SyncedPrimitive<U>& f, const T g) {return f.x|g;} \
	template<class U> inline UPCAST(T,U) operator^(const SyncedPrimitive<U>& f, const T g) {return f.x^g;} \
	template<class U> inline UPCAST(T,U) operator<<(const SyncedPrimitive<U>& f, const T g) {return f.x<<g;} \
	template<class U> inline UPCAST(T,U) operator>>(const SyncedPrimitive<U>& f, const T g) {return f.x>>g;} \
	template<class V> inline UPCAST(T,V) operator+(const T f, const SyncedPrimitive<V>& g) {return f+g.x;} \
	template<class V> inline UPCAST(T,V) operator-(const T f, const SyncedPrimitive<V>& g) {return f-g.x;} \
	template<class V> inline UPCAST(T,V) operator*(const T f, const SyncedPrimitive<V>& g) {return f*g.x;} \
	template<class V> inline UPCAST(T,V) operator/(const T f, const SyncedPrimitive<V>& g) {return f/g.x;} \
	template<class V> inline UPCAST(T,V) operator%(const T f, const SyncedPrimitive<V>& g) {return f%g.x;} \
	template<class V> inline UPCAST(T,V) operator&(const T f, const SyncedPrimitive<V>& g) {return f&g.x;} \
	template<class V> inline UPCAST(T,V) operator|(const T f, const SyncedPrimitive<V>& g) {return f|g.x;} \
	template<class V> inline UPCAST(T,V) operator^(const T f, const SyncedPrimitive<V>& g) {return f^g.x;} \
	template<class V> inline UPCAST(T,V) operator<<(const T f, const SyncedPrimitive<V>& g) {return f<<g.x;} \
	template<class V> inline UPCAST(T,V) operator>>(const T f, const SyncedPrimitive<V>& g) {return f>>g.x;} \
	template<class U> inline bool operator<(const SyncedPrimitive<U>& f, const T g) {return f.x<g;} \
	template<class U> inline bool operator>(const SyncedPrimitive<U>& f, const T g) {return f.x>g;} \
	template<class U> inline bool operator<=(const SyncedPrimitive<U>& f, const T g) {return f.x<=g;} \
	template<class U> inline bool operator>=(const SyncedPrimitive<U>& f, const T g) {return f.x>=g;} \
	template<class U> inline bool operator==(const SyncedPrimitive<U>& f, const T g) {return f.x==g;} \
	template<class U> inline bool operator!=(const SyncedPrimitive<U>& f, const T g) {return f.x!=g;} \
	template<class V> inline bool operator<(const T f, const SyncedPrimitive<V>& g) {return f<g.x;} \
	template<class V> inline bool operator>(const T f, const SyncedPrimitive<V>& g) {return f>g.x;} \
	template<class V> inline bool operator<=(const T f, const SyncedPrimitive<V>& g) {return f<=g.x;} \
	template<class V> inline bool operator>=(const T f, const SyncedPrimitive<V>& g) {return f>=g.x;} \
	template<class V> inline bool operator==(const T f, const SyncedPrimitive<V>& g) {return f==g.x;} \
	template<class V> inline bool operator!=(const T f, const SyncedPrimitive<V>& g) {return f!=g.x;}

	FOR_EACH_PRIMITIVE_TYPE
#undef DO
template<class T> inline T min(const T f, const SyncedPrimitive<T>& g) {return std::min(f,g.x);}
template<class T> inline T min(const SyncedPrimitive<T>& f, const T g) {return std::min(f.x,g);}
template<class T> inline T max(const T f, const SyncedPrimitive<T>& g) {return std::max(f,g.x);}
template<class T> inline T max(const SyncedPrimitive<T>& f, const T g) {return std::max(f.x,g);}

typedef SyncedPrimitive<          bool  > SyncedBool;
typedef SyncedPrimitive<   signed char  > SyncedSchar;
typedef SyncedPrimitive<   signed short > SyncedSshort;
typedef SyncedPrimitive<   signed int   > SyncedSint;
typedef SyncedPrimitive<   signed long  > SyncedSlong;
typedef SyncedPrimitive< unsigned char  > SyncedUchar;
typedef SyncedPrimitive< unsigned short > SyncedUshort;
typedef SyncedPrimitive< unsigned int   > SyncedUint;
typedef SyncedPrimitive< unsigned long  > SyncedUlong;
typedef SyncedPrimitive<          float > SyncedFloat;
typedef SyncedPrimitive<         double > SyncedDouble;
typedef SyncedPrimitive<    long double > SyncedLongDouble;

#ifdef UPCAST_USE_64_BIT_TYPES
typedef SyncedPrimitive<         int64_t > SyncedSint64;
typedef SyncedPrimitive<         uint64_t > SyncedUint64;
#endif // UPCAST_USE_64_BIT_TYPES


// overload some useful functions
// this is barely legal
// can't just put template functions in namespace std, this confuses several things
namespace std {
	inline float min(SyncedFloat& a, float b)
	{
		if (a < b) return a;
		else return b;
	}

	inline float min(float a, SyncedFloat& b)
	{
		if (a < b) return a;
		else return b;
	}

	inline float max(SyncedFloat& a, float b)
	{
		if (a > b) return a;
		else return b;
	}

	inline float max(float a, SyncedFloat& b)
	{
		if (a > b) return a;
		else return b;
	}
}

#else // SYNCDEBUG || SYNCCHECK

typedef          bool  SyncedBool;
typedef   signed char  SyncedSchar;
typedef   signed short SyncedSshort;
typedef   signed int   SyncedSint;
typedef   signed long  SyncedSlong;
typedef unsigned char  SyncedUchar;
typedef unsigned short SyncedUshort;
typedef unsigned int   SyncedUint;
typedef unsigned long  SyncedUlong;
typedef          float SyncedFloat;
typedef         double SyncedDouble;
typedef    long double SyncedLongDouble;

#endif // !SYNCDEBUG && !SYNCCHECK

// this macro looks like a noop, but causes checksum update
#ifdef SYNCDEBUG
#  ifdef __GNUC__
#    define ASSERT_SYNCED_PRIMITIVE(x) { SyncedPrimitive<typeof(x)>(x); }
#  else
#    include <boost/typeof/typeof.hpp>
#    define ASSERT_SYNCED_PRIMITIVE(x) { SyncedPrimitive<BOOST_TYPEOF(x)>(x); }
#  endif
#else
#  define ASSERT_SYNCED_PRIMITIVE(x)
#endif

#endif // SYNCEDPRIMITIVE_H
