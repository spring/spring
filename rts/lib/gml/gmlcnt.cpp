#ifndef GML_COMPATIBLE_ATOMIC_COUNT
#      ifdef BOOST_DETAIL_ATOMIC_COUNT_HPP_INCLUDED
#              error "Please make sure myGL.h is included before anything that includes boost"
#      endif
#      define GML_COMPATIBLE_ATOMIC_COUNT
//FIXME doing it wrong
#      define private public
#      include <boost/detail/atomic_count.hpp>
#      undef private
#endif

#include "gmlcnt.h"

// this will assign the counter of a boost atomic_count object
void operator%=(gmlCount& a, long val) {
#if GML_LOCKED_GMLCOUNT_ASSIGNMENT
#if defined(BOOST_AC_USE_PTHREADS)
	boost::mutex::scoped_lock lock(a.mutex_);
	a.value_=val;
#elif (BOOST_VERSION>=103500) && (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)))
	__asm__ __volatile__("lock\n\txchgl %0,%1\n\t" : "=r" (val) : "m" (a.value_), "0" (val) : "memory");
#elif defined(_WIN32)
	return BOOST_INTERLOCKED_EXCHANGE(&a.value_,val);
#elif (BOOST_VERSION>=103500) && (defined(__GNUC__) && (__GNUC__*100+__GNUC_MINOR__>=401))
	__sync_exchange_FIXME(&a.value_, val);
#elif defined(__GLIBCXX__)
	__gnu_cxx::__exchange_FIXME(&a.value_, val);
#elif defined(__GLIBCPP__)
	__exchange_FIXME(&a.value_, val);
#elif defined(BOOST_HAS_PTHREADS)
#define BOOST_AC_USE_PTHREADS
	boost::mutex::scoped_lock lock(a.mutex_);
	a.value_=val;
#else
#error Unrecognized threading platform
#endif
#else
	a.value_=val;
#endif
}


