/////////////////////////////////////////////////////////
// brief:
//   Implementation of multiple thread-safe containers.
// author:
//   jK
//
// Copyright (C) 2009.
// Licensed under the terms of the GNU GPL, v2 or later.
//

#ifndef TSC_H
#define TSC_H

#include <vector>
#include "System/creg/creg_cond.h"

/////////////////////////////////////////////////////////
//

template <class C, class T, class D>
class ThreadListRender {
private:
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListRender)

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		D::Add(x);
	}
	void insert(const T& x) {
		D::Add(x);
	}

	void erase_remove_synced(const T& x) {
		D::Remove(x);
	}

public:
	//! RENDER/UNSYNCED METHODS
	void erase_delete(const T& x) {
		D::Remove(x);
	}

	void enqueue(const T& x) {
		D::Add(x);
	}

	void dequeue(const C& x) {
		D::Remove(x);
	}

};

#endif // #ifndef TSC_H
