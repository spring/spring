/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */



#include "System/Platform/CrashHandler.h"
#include "System/Platform/Threading.h"
#include "System/ThreadPool.h"
#include <vector>

#define BOOST_TEST_MODULE CrashHandler
#include <boost/test/unit_test.hpp>

/**
 * Uses ThreadPool::methods to start a few threads, then trigger stack traces in these threads using various methods.
 */
struct ThreadedStackFixture
{
    ThreadedStackFixture()
       : seriousErrorCondition(false)
    {
        Threading::InitThreadPool();
    }

    ~ThreadedStackFixture()
    {

    }

    bool seriousErrorCondition = false;



    void func1 () {
        func2();
    }

    void func2 () {
        if (seriousErrorCondition) {

        }
    }
};


BOOST_AUTO_TEST_CASE( testSingleThreadedCrash )
{
    ThreadedStackFixture f;

    f.seriousErrorCondition = true;

    f.func1();

}

BOOST_AUTO_TEST_CASE( testMultiThreadedCrash )
{


}

BOOST_AUTO_TEST_CASE( testConcurrentCrash )
{

}
