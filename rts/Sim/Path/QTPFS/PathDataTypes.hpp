/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATH_DATA_TYPES_HDR
#define QTPFS_PATH_DATA_TYPES_HDR

#include <queue>

namespace QTPFS {
	template <class Element, class Container, class Comparator>
	class reservable_priority_queue: public std::priority_queue<Element, Container, Comparator> {
	public:
		typedef typename std::priority_queue<Element, Container, Comparator>::size_type size_type;

		reservable_priority_queue(size_type capacity = 0) { reserve(capacity); };
		~reservable_priority_queue() { this->c.clear(); }

		void reserve(size_type capacity) { this->c.reserve(capacity); } 
		size_type capacity() const { return this->c.capacity(); } 
	};
};

#endif

