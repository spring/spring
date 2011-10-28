/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_PATH_DATA_TYPES_HDR
#define QTPFS_PATH_DATA_TYPES_HDR

#include <queue>

namespace QTPFS {
	template<class Element>
	class clearable_vector: public std::vector<Element> {
	public:
		// reset vector in O(1) time WITHOUT erasing elements
		// (subsequent pushes simply displace them in memory)
		void shallow_clear() {
			typename std::vector<Element>::_Vector_impl& v = this->_M_impl;
			v._M_finish = v._M_start;
		}
	};

	template <class Element, class Container, class Comparator>
	class reservable_priority_queue: public std::priority_queue<Element, Container, Comparator> {
	public:
		typedef typename std::priority_queue<Element, Container, Comparator>::size_type size_type;

		reservable_priority_queue(size_type capacity = 0) { reserve(capacity); };
		~reservable_priority_queue() { this->c.clear(); }

		void clear() { Container& c = this->c; c.clear(); }
		void reset() { Container& c = this->c; c.shallow_clear(); }
		void reserve(size_type capacity) { this->c.reserve(capacity); } 
		size_type capacity() const { return this->c.capacity(); } 
	};
};

#endif

