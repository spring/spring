// GML - OpenGL Multithreading Library
// for Spring http://spring.clan-sy.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

// This code is largely based on boost::detail::atomic_count
//
// Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GMLCNT_H
#define GMLCNT_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_HAS_THREADS

typedef long gmlCount;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined(BOOST_AC_USE_PTHREADS)

#include <pthread.h>

class gmlCount {
private:

    class scoped_lock {
    public:

        scoped_lock(pthread_mutex_t & m): m_(m) {
            pthread_mutex_lock(&m_);
        }

        ~scoped_lock() {
            pthread_mutex_unlock(&m_);
        }

    private:

        pthread_mutex_t & m_;
    };

public:

    explicit gmlCount(long v): value_(v) {
        pthread_mutex_init(&mutex_, 0);
    }

    ~gmlCount() {
        pthread_mutex_destroy(&mutex_);
    }

    long operator++() {
        scoped_lock lock(mutex_);
        return ++value_;
    }

    long operator--() {
        scoped_lock lock(mutex_);
        return --value_;
    }

    operator long() const {
        scoped_lock lock(mutex_);
        return value_;
    }

    long value_;

private:

    gmlCount(gmlCount const &);
    gmlCount & operator=(gmlCount const &);

    mutable pthread_mutex_t mutex_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined( __GNUC__ ) && ( defined( __i386__ ) || defined( __x86_64__ ) )

class gmlCount {
public:

    explicit gmlCount( long v ) : value_( static_cast< int >( v ) ) {}

    long operator++() {
        return atomic_exchange_and_add( &value_, 1 ) + 1;
    }

    long operator--() {
        return atomic_exchange_and_add( &value_, -1 ) - 1;
    }

    operator long() const {
        return atomic_exchange_and_add( &value_, 0 );
    }

    mutable int value_;

private:

    gmlCount(gmlCount const &);
    gmlCount & operator=(gmlCount const &);

private:

    static int atomic_exchange_and_add( int * pw, int dv ) {
        // int r = *pw;
        // *pw += dv;
        // return r;

        int r;

        __asm__ __volatile__
        (
            "lock\n\t"
            "xadd %1, %0":
            "+m"( *pw ), "=r"( r ): // outputs (%0, %1)
            "1"( dv ): // inputs (%2 == %1)
            "memory", "cc" // clobbers
        );

        return r;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/detail/interlocked.hpp>

class gmlCount {
public:

    explicit gmlCount( long v ): value_( v ) {}

    long operator++() {
        return BOOST_INTERLOCKED_INCREMENT( &value_ );
    }

    long operator--() {
        return BOOST_INTERLOCKED_DECREMENT( &value_ );
    }

    operator long() const {
        return static_cast<long const volatile &>( value_ );
    }

    long value_;

private:

    gmlCount( gmlCount const & );
    gmlCount & operator=( gmlCount const & );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined( __GNUC__ ) && ( __GNUC__ * 100 + __GNUC_MINOR__ >= 401 ) && !defined( __arm__ ) && !defined( __hppa ) && ( !defined( __INTEL_COMPILER ) || defined( __ia64__ ) )

#if defined( __ia64__ ) && defined( __INTEL_COMPILER )
# include <ia64intrin.h>
#endif

class gmlCount {
public:

    explicit gmlCount( long v ) : value_( v ) {}

    long operator++() {
        return __sync_add_and_fetch( &value_, 1 );
    }

    long operator--() {
        return __sync_add_and_fetch( &value_, -1 );
    }

    operator long() const {
        return __sync_fetch_and_add( &value_, 0 );
    }

    mutable long value_;

private:

    gmlCount(gmlCount const &);
    gmlCount & operator=(gmlCount const &);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)

#include <bits/atomicity.h>

#if defined(__GLIBCXX__) // g++ 3.4+

using __gnu_cxx::__atomic_add;
using __gnu_cxx::__exchange_and_add;

#endif

class gmlCount {
public:

    explicit gmlCount(long v) : value_(v) {}

    long operator++() {
        return __exchange_and_add(&value_, 1) + 1;
    }

    long operator--() {
        return __exchange_and_add(&value_, -1) - 1;
    }

    operator long() const {
        return __exchange_and_add(&value_, 0);
    }

    mutable _Atomic_word value_;

private:

    gmlCount(gmlCount const &);
    gmlCount & operator=(gmlCount const &);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#elif defined(BOOST_HAS_PTHREADS)

#  define BOOST_AC_USE_PTHREADS

#include <pthread.h>

class gmlCount {
private:

    class scoped_lock {
    public:

        scoped_lock(pthread_mutex_t & m): m_(m) {
            pthread_mutex_lock(&m_);
        }

        ~scoped_lock() {
            pthread_mutex_unlock(&m_);
        }

    private:

        pthread_mutex_t & m_;
    };

public:

    explicit gmlCount(long v): value_(v) {
        pthread_mutex_init(&mutex_, 0);
    }

    ~gmlCount() {
        pthread_mutex_destroy(&mutex_);
    }

    long operator++() {
        scoped_lock lock(mutex_);
        return ++value_;
    }

    long operator--() {
        scoped_lock lock(mutex_);
        return --value_;
    }

    operator long() const {
        scoped_lock lock(mutex_);
        return value_;
    }

    long value_;

private:

    gmlCount(gmlCount const &);
    gmlCount & operator=(gmlCount const &);

    mutable pthread_mutex_t mutex_;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
#else

#error Unrecognized threading platform

#endif

#ifdef BOOST_HAS_THREADS

inline void operator%=(gmlCount& a, long val) {
	a.value_=val;
}

#endif

#endif // #ifndef GMLCNT_H
