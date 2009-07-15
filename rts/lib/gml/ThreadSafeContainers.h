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
#include "gml.h"

/////////////////////////////////////////////////////////
//

//FIXME class ThreadVector<class T>;
//class ThreadListSimRender<class T>;
//class ThreadVectorSimRender<class T>;

/////////////////////////////////////////////////////////

#if !defined(USE_GML) || !GML_ENABLE_SIM

template <class C, class R, class T>
class ThreadListSimRender {
private:
	typedef typename C::iterator ListIT;
	typedef typename C::const_iterator constIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListSimRender);

	void PostLoad() {};

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
	}

	void insert(const T& x) {
		cont.insert(x);
	}

	ListIT insert(ListIT& it, const T& x) {
		return cont.insert(it, x);
	}

	// keep same deletion order in MT and non-MT version to reduce risk for desync
	ListIT erase_delete_synced(ListIT& it) {
		delObj.push_back(*it);
		return cont.erase(it);
	}

	ListIT erase_delete_set_synced(ListIT& it) {
		delObj.push_back(*it);
#ifdef _MSC_VER
		return cont.erase(it);
#else
		cont.erase(it++);
		return it;
#endif
	}

	bool can_delete_synced() const {
		return !delObj.empty();
	}

	void delete_erased_synced() {
		for (VecIT it = delObj.begin(); it != delObj.end(); ++it) {
			delete *it;
		}
		delObj.clear();
	}

	void resize(const size_t& s) {
		cont.resize(s);
	}

	size_t size() const {
		return cont.size();
	}

	bool empty() const {
		return cont.empty();
	}

	ListIT begin() {
		return cont.begin();
	}

	ListIT end() {
		return cont.end();
	}

public:
	//! RENDER/UNSYNCED METHODS
	void delay_add() const {
	}

	void add_delayed() const {
	}

	bool can_delay_add() const {
		return false;
	}

	ListIT erase_delete(ListIT& it) {
		delete *it;
		return cont.erase(it);
	}

	ListIT erase_delete_set(ListIT& it) {
		delete *it;
#ifdef _MSC_VER
		return cont.erase(it);
#else
		cont.erase(it++);
		return it;
#endif
	}

	bool can_delete() const {
		return false;
	}

	void delete_erased() const {
	}

	void delay_delete() const {
	}

	void delete_delayed() const {
	}

	size_t render_size() const {
		return cont.size();
	}

	bool render_empty() const {
		return cont.empty();
	}

	constIT render_begin() const {
		return cont.begin();
	}

	constIT render_end() const {
		return cont.end();
	}

public:
	typedef ListIT iterator;
	typedef constIT render_iterator;

public:
	C cont;
	std::vector<T> delObj;
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
	}

	VectorIT insert(VectorIT& it, const T& x) {
		if (it != cont.end()) {
			//! fast insert
			cont.push_back(*it);
			*it = x;
			return it;
		}
		return cont.insert(it, x);
	}

	VectorIT erase(VectorIT& it) {
		VectorIT ito = it++;
		if (it == cont.end()) {
			cont.pop_back();
			return cont.end();
		}
		*ito = cont.back();
		cont.pop_back();
		return ito;
	}

	void resize(const size_t& s) {
		cont.resize(s);
	}

	size_t size() const {
		return cont.size();
	}

	bool empty() const {
		return cont.empty();
	}

	VectorIT begin() {
		return cont.begin();
	}

	VectorIT end() {
		return cont.end();
	}

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() const {
	}

	size_t render_size() const {
		return cont.size();
	}

	bool render_empty() const {
		return cont.empty();
	}

	constIT render_begin() const {
		return cont.begin();
	}

	constIT render_end() const {
		return cont.end();
	}

public:
	typedef VectorIT iterator;
	typedef constIT render_iterator;

//private:
public:
	std::vector<T> cont;
};

#else

#include <set>

template <class C, class R, class T>
class ThreadListSimRender {
private:
	typedef typename C::iterator SimIT;
	typedef typename R::const_iterator RenderIT;
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListSimRender);

	~ThreadListSimRender() {
		for(SimIT it = cont.begin(); it != cont.end(); ++it) {
			delete *it;
		}
		delete_erased();
		delete_delayed();
	}

	void PostLoad() {
		for (SimIT it = cont.begin(); it != cont.end(); ++it) {
			preAddRender.insert(*it);
		}
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		preAddRender.insert(x);
		cont.push_back(x);
	}
	void insert(const T& x) {
		preAddRender.insert(x);
		cont.insert(x);
	}

	SimIT insert(SimIT& it, const T& x) {
		preAddRender.insert(x);
		return cont.insert(it, x);
	}

	// keep same deletion order in MT and non-MT version to reduce risk for desync
	SimIT erase_delete_synced(SimIT& it) {
		delRender.push_back(*it);
		return cont.erase(it);
	}

	SimIT erase_delete_set_synced(SimIT& it) {
		delRender.push_back(*it);
#ifdef _MSC_VER
		return cont.erase(it);
#else
		cont.erase(it++);
		return it;
#endif
	}

	bool can_delete_synced() {
		return !delRender.empty();
	}

	void delete_erased_synced() {
		for (VecIT it = delRender.begin(); it != delRender.end(); ++it) {
			T s = *it;
			if(!contRender.erase(s) && !addRender.erase(s) && !preAddRender.erase(s))
				assert(false);
			delete s;
		}
		delRender.clear();
	}

	void resize(const size_t& s) {
		cont.resize(s);
	}

	size_t size() const {
		return cont.size();
	}

	bool empty() const {
		return cont.empty();
	}

	SimIT begin() {
		return cont.begin();
	}

	SimIT end() {
		return cont.end();
	}

public:
	//! RENDER/UNSYNCED METHODS
	void delay_add() {
		for (constSetIT it = preAddRender.begin(); it != preAddRender.end(); ++it) {
			addRender.insert(*it);
		}
		preAddRender.clear();
	}

	void add_delayed() {
		for (constSetIT it = addRender.begin(); it != addRender.end(); ++it) {
			contRender.insert(*it);
		}
		addRender.clear();
	}

	bool can_delay_add() {
		return !preAddRender.empty();
	}

	SimIT erase_delete(SimIT& it) {
		delRender.push_back(*it);
		return cont.erase(it);
	}

	SimIT erase_delete_set(SimIT& it) {
		delRender.push_back(*it);
#ifdef _MSC_VER
		return cont.erase(it);
#else
		cont.erase(it++);
		return it;
#endif
	}

	bool can_delete() {
		return !delRender.empty();
	}

	void delete_erased() {
		for (VecIT it = delRender.begin(); it != delRender.end(); ++it) {
			T s = *it;
			if(!contRender.erase(s) && !addRender.erase(s) && !preAddRender.erase(s))
				assert(false);
			delete s;
		}
		delRender.clear();
	}

	void delay_delete() {
		for (VecIT it = delRender.begin(); it != delRender.end(); ++it) {
			postDelRender.insert(*it);
		}
		delRender.clear();
	}

	void delete_delayed() {
		for (SetIT it = postDelRender.begin(); it != postDelRender.end();) {
			T s = *it;
			if(contRender.erase(s) || addRender.erase(s)) {
#ifdef _MSC_VER
				it = postDelRender.erase(it);
#else
				postDelRender.erase(it++);
#endif
			}
			else
				++it;
		}
	}

	size_t render_size() const {
		return contRender.size();
	}

	bool render_empty() const {
		return contRender.empty();
	}

	RenderIT render_begin() const {
		return contRender.begin();
	}

	RenderIT render_end() const {
		return contRender.end();
	}

public:
	typedef SimIT iterator;
	typedef RenderIT render_iterator;

public: //!needed by CREG
	C cont;

private:
	std::set<T> preAddRender;
	std::set<T> addRender;
	R contRender;
	std::vector<T> delRender;
	std::set<T> postDelRender;
};


template <class T>
class ThreadVectorSimRender {
private:
	typedef typename std::vector<T>::iterator SimIT;
	typedef typename std::set<T>::const_iterator RenderIT;
	typedef typename std::set<T>::const_iterator constSetIT;

public:
	CR_DECLARE_STRUCT(ThreadVectorSimRender);

	void PostLoad() {
		for (SimIT it = cont.begin(); it != cont.end(); ++it) {
			addRender.insert(*it);
		}
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
		addRender.insert(x);
	}

	SimIT insert(SimIT& it, const T& x) {
		addRender.insert(x);
		if (it != cont.end()) {
			//! fast insert
			cont.push_back(*it);
			*it = x;
			return it;
		}
		return cont.insert(it, x);
	}

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
	}

	void resize(const size_t& s) {
		cont.resize(s);
	}

	size_t size() const {
		return cont.size();
	}

	bool empty() const {
		return cont.empty();
	}

	SimIT begin() {
		return cont.begin();
	}

	SimIT end() {
		return cont.end();
	}

public:
	//! RENDER/UNSYNCED METHODS
	inline void update() {
		for (constSetIT it = addRender.begin(); it != addRender.end(); ++it)
		{
			contRender.insert(*it);
		}
		addRender.clear();
		for (constSetIT it = delRender.begin(); it != delRender.end(); ++it)
		{
			contRender.erase(*it);
		}
		delRender.clear();
	}

	size_t render_size() const {
		return contRender.size();
	}

	bool render_empty() const {
		return contRender.empty();
	}

	RenderIT render_begin() const {
		return contRender.begin();
	}

	RenderIT render_end() const {
		return contRender.end();
	}

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
