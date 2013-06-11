/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNCEDPRIMITIVE_H
#define SYNCEDPRIMITIVE_H

#if defined(SYNCDEBUG) || defined(SYNCCHECK)

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
#define FOR_EACH_PRIMITIVE_TYPE \
	DO(signed char) \
	DO(signed short) \
	DO(signed int) \
	DO(unsigned char) \
	DO(unsigned short) \
	DO(unsigned int) \
	DO(float) \
	DO(double) \
	DO(long double) \
	DO(bool)

/** \p SyncedPrimitive class. Variables of this type are automagically
downcasted to their value_type, preventing them to be implicitly used for
anything but carefully selected places. The goal of this class is to call
\p CSyncedPrimitiveBase::Sync() on each write to the data member \p x. */
template <class T>
struct SyncedPrimitive
{
private:
	T x;
	void Sync(const char* op) {Sync::Assert(x, op);}

public:
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

	operator const T& () const { return x; }
};

typedef SyncedPrimitive<          bool  > SyncedBool;
typedef SyncedPrimitive<   signed char  > SyncedSchar;
typedef SyncedPrimitive<   signed short > SyncedSshort;
typedef SyncedPrimitive<   signed int   > SyncedSint;
typedef SyncedPrimitive< unsigned char  > SyncedUchar;
typedef SyncedPrimitive< unsigned short > SyncedUshort;
typedef SyncedPrimitive< unsigned int   > SyncedUint;
typedef SyncedPrimitive<          float > SyncedFloat;
typedef SyncedPrimitive<         double > SyncedDouble;
typedef SyncedPrimitive<    long double > SyncedLongDouble;

#else // SYNCDEBUG || SYNCCHECK

typedef          bool  SyncedBool;
typedef   signed char  SyncedSchar;
typedef   signed short SyncedSshort;
typedef   signed int   SyncedSint;
typedef unsigned char  SyncedUchar;
typedef unsigned short SyncedUshort;
typedef unsigned int   SyncedUint;
typedef          float SyncedFloat;
typedef         double SyncedDouble;
typedef    long double SyncedLongDouble;

#endif // !SYNCDEBUG && !SYNCCHECK

#endif // SYNCEDPRIMITIVE_H
