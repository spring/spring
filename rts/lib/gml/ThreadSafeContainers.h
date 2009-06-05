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

#include <list>
#include <vector>

#include "creg/creg_cond.h"

/////////////////////////////////////////////////////////
//

//FIXME class ThreadVector<class T>;
//class ThreadListSimRender<class T>;
//class ThreadVectorSimRender<class T>;

/////////////////////////////////////////////////////////

#ifndef USE_GML

/*FIXME (GML version needs work)
template <class T>
class ThreadVector {
private:
	typedef typename std::vector<T>::iterator VectorIT;

public:
	CR_DECLARE_STRUCT(ThreadVector);

	void PostLoad() {};

	void push(const T& x) {
		cont.push_back(x);
	};

	VectorIT insert(VectorIT& it, const T& x) {
		if (it != cont.end()) {
			//! fast insert
			cont.push_back(*it);
			*it = x;
			return it;
		}
		return cont.insert(it, x);
	};

	VectorIT erase(VectorIT& it) {
		VectorIT ito = it++;
		if (it == cont.end()) {
			cont.pop_back();
			return cont.end();
		}
		*ito = cont.back();
		cont.pop_back();
		return ito;
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	VectorIT begin() const {
		return cont.begin();
	};

	VectorIT end() const {
		return cont.end();
	};

public:
	inline void update() const {
	};

public:
	typedef VectorIT iterator;

public:
	std::vector<T> cont;
};
*/

template <class T>
class ThreadListSimRender {
private:
	typedef typename std::list<T>::iterator ListIT;
	typedef typename std::list<T>::const_iterator constIT;

public:
	CR_DECLARE_STRUCT(ThreadListSimRender);

	void PostLoad() {};

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
	};

	ListIT insert(ListIT& it, const T& x) {
		return cont.insert(it, x);
	};

	ListIT erase(ListIT& it) {
		/*VectorIT ito = it++;
		if (it == cont.end()) {
			cont.pop_back();
			return cont.end();
		}
		*ito = cont.back();
		cont.pop_back();
		return ito;*/
		return cont.erase(it);
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	ListIT begin() {
		return cont.begin();
	};

	ListIT end() {
		return cont.end();
	};

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() const {
	};

	size_t render_size() const {
		return cont.size();
	};

	bool render_empty() const {
		return cont.empty();
	};

	constIT render_begin() const {
		return cont.begin();
	};

	constIT render_end() const {
		return cont.end();
	};

public:
	typedef ListIT iterator;
	typedef constIT render_iterator;

//private:
public:
	std::list<T> cont;
};


template <class T>
class ThreadVectorSimRender {
private:
	typedef typename std::vector<T>::iterator VectorIT;
	typedef typename std::vector<T>::const_iterator constIT;

public:
	CR_DECLARE_STRUCT(ThreadVectorSimRender);

	void PostLoad() {};

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
	};

	VectorIT insert(VectorIT& it, const T& x) {
		if (it != cont.end()) {
			//! fast insert
			cont.push_back(*it);
			*it = x;
			return it;
		}
		return cont.insert(it, x);
	};

	VectorIT erase(VectorIT& it) {
		VectorIT ito = it++;
		if (it == cont.end()) {
			cont.pop_back();
			return cont.end();
		}
		*ito = cont.back();
		cont.pop_back();
		return ito;
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	VectorIT begin() {
		return cont.begin();
	};

	VectorIT end() {
		return cont.end();
	};

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() const {
	};

	size_t render_size() const {
		return cont.size();
	};

	bool render_empty() const {
		return cont.empty();
	};

	constIT render_begin() const {
		return cont.begin();
	};

	constIT render_end() const {
		return cont.end();
	};

public:
	typedef VectorIT iterator;
	typedef constIT render_iterator;

//private:
public:
	std::vector<T> cont;
};



#else

#include <set>

/*FIXME (how to erase items?)
template <class T>
class ThreadVector {
private:
	typedef typename std::vector<T>::iterator VectorIT;

public:
	CR_DECLARE_STRUCT(ThreadVector);

	void PostLoad();

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		add.push_back(x);
	};

	void erase(VectorIT& it) {
		del.push(*it); //FAIL (don't use iterators?)
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	VectorIT begin() {
		return cont.begin();
	};

	VectorIT end() {
		return cont.end();
	};

public:
	inline void update() {
		for (std::vector<T>::iterator it = del.begin(); it != del.end(); it++)
		{
			cont.push_back(*it); //FIXME
		}
		del.clear();

		for (std::vector<T>::iterator it = add.begin(); it != add.end(); it++)
		{
			cont.push_back(*it);
		}
		add.clear();
	};

public:
	typedef VectorIT iterator;

private:
	std::vector<T> cont;
	std::vector<T> add;
	std::vector<T> del;
};*/


template <class T>
class ThreadListSimRender {
private:
	typedef typename std::list<T>::iterator SimIT;
	typedef typename std::set<T>::const_iterator RenderIT;
	typedef typename std::set<T>::const_iterator setIT;

public:
	CR_DECLARE_STRUCT(ThreadListSimRender);

	void PostLoad() {
		for (SimIT it = cont.begin(); it != cont.end(); it++) {
			addRender.insert(*it);
		}
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
		addRender.insert(x);
	};

	SimIT insert(SimIT& it, const T& x) {
		addRender.insert(x);
		return cont.insert(it, x);
	};

	SimIT erase(SimIT& it) {
		delRender.insert(*it);
		return cont.erase(it);
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	SimIT begin() {
		return cont.begin();
	};

	SimIT end() {
		return cont.end();
	};

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() {
		for (setIT it = addRender.begin(); it != addRender.end(); it++)
		{
			contRender.insert(*it);
		}
		addRender.clear();
		for (setIT it = delRender.begin(); it != delRender.end(); it++)
		{
			contRender.erase(*it);
		}
		delRender.clear();
	};

	size_t render_size() const {
		return contRender.size();
	};

	bool render_empty() const {
		return contRender.empty();
	};

	RenderIT render_begin() const {
		return contRender.begin();
	};

	RenderIT render_end() const {
		return contRender.end();
	};

public:
	typedef SimIT iterator;
	typedef RenderIT render_iterator;

public: //!needed by CREG
	std::list<T> cont;

private:
	std::set<T> contRender;
	std::set<T> addRender;
	std::set<T> delRender;
};


template <class T>
class ThreadVectorSimRender {
private:
	typedef typename std::vector<T>::iterator SimIT;
	typedef typename std::set<T>::const_iterator RenderIT;
	typedef typename std::set<T>::const_iterator setIT;

public:
	CR_DECLARE_STRUCT(ThreadVectorSimRender);

	void PostLoad() {
		for (SimIT it = cont.begin(); it != cont.end(); it++) {
			addRender.insert(*it);
		}
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
		addRender.insert(x);
	};

	SimIT insert(SimIT& it, const T& x) {
		addRender.insert(x);
		if (it != cont.end()) {
			//! fast insert
			cont.push_back(*it);
			*it = x;
			return it;
		}
		return cont.insert(it, x);
	};

	SimIT erase(SimIT& it) {
		delRender.insert(*it);

		SimIT ito = it++;
		if (it == cont.end()) {
			cont.pop_back();
			return cont.end();
		}
		*ito = cont.back();
		cont.pop_back();
		return ito;
	};

	void resize(const size_t& s) {
		cont.resize(s);
	};

	size_t size() const {
		return cont.size();
	};

	bool empty() const {
		return cont.empty();
	};

	SimIT begin() {
		return cont.begin();
	};

	SimIT end() {
		return cont.end();
	};

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() {
		for (setIT it = addRender.begin(); it != addRender.end(); it++)
		{
			contRender.insert(*it);
		}
		addRender.clear();
		for (setIT it = delRender.begin(); it != delRender.end(); it++)
		{
			contRender.erase(*it);
		}
		delRender.clear();
	};

	size_t render_size() const {
		return contRender.size();
	};

	bool render_empty() const {
		return contRender.empty();
	};

	RenderIT render_begin() const {
		return contRender.begin();
	};

	RenderIT render_end() const {
		return contRender.end();
	};

public:
	typedef SimIT iterator;
	typedef RenderIT render_iterator;

public: //!needed by CREG
	std::vector<T> cont;

private:
	std::set<T> contRender;
	std::set<T> addRender;
	std::set<T> delRender;
};


#endif

#endif // #ifndef TSC_H
