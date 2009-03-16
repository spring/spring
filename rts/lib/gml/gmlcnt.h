#ifndef GMLCNT_H
#define GMLCNT_H

#include <boost/version.hpp>
#include <boost/detail/atomic_count.hpp>

class gmlCount : public boost::detail::atomic_count {
public:
	gmlCount(int init) : atomic_count(init)
	{};
	long operator++() {
		boost::detail::atomic_count::operator++();
		return long(*this);
	}
};


// this will assign the counter of a boost atomic_count object
void operator%=(gmlCount& a, long val);

#endif