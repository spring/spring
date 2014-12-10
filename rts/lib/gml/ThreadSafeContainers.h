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

#include "System/creg/creg_cond.h"
#include <set>
#include <map>

/////////////////////////////////////////////////////////
//

template <class C, class R, class T>
class ThreadListSimRender {
private:
	typedef typename C::iterator ListIT;
	typedef typename C::const_iterator constIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListSimRender)

	void PostLoad() {};

	~ThreadListSimRender() {
		clear();
	}

	void clear() {
		for(ListIT it = cont.begin(); it != cont.end(); ++it) {
			delete *it;
		}
		cont.clear();
		delete_erased_synced();
	}

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
		return set_erase(cont, it);
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
		return set_erase(cont, it);
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



/////////////////////////////////////////////////////////
//

template <class C, class R, class T, class D>
class ThreadListRender {
private:
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
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

	void dequeue_synced(const C& x) {
		simDelQueue.push_back(x);
	}

	void destroy_synced() {
		for (VecIT it = simDelQueue.begin(); it != simDelQueue.end(); ++it) {
			D::Remove(*it);
		}
		simDelQueue.clear();
	}

private:
	std::vector<T> simDelQueue;
};


/////////////////////////////////////////////////////////
//

template <class Key, class Value, class Indexer>
class ThreadMapRender {
private:
	typedef std::map<int,Value> TMapC;

public:
	CR_DECLARE_STRUCT(ThreadMapRender)

	const TMapC& get_render_map() const { return contRender; }

	void PostLoad() {
	}

	//! SIMULATION/SYNCED METHODS
	void push(const Key& x, const Value& y) {
		contRender[Indexer::Index(x)] = y;
	}

public:
	//! RENDER/UNSYNCED METHODS
	void erase_delete(const Key& x) {
		contRender.erase(Indexer::Index(x));
	}


private:
	TMapC contRender;
};


/////////////////////////////////////////////////////////
//

template <class C, class R, class T, class D>
class ThreadListSim {
private:
	typedef typename C::iterator SimIT;
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListSim)

	~ThreadListSim() {
		clear();
	}

	void clear() {
		for(SimIT it = cont.begin(); it != cont.end(); ++it) {
			delete *it;
		}
		cont.clear();
		delete_erased_synced();
	}

	void detach_all() {
		for(SimIT it = cont.begin(); it != cont.end(); ++it) {
			D::Detach(*it);
		}
	}

	void PostLoad() {
		for (SimIT it = cont.begin(); it != cont.end(); ++it) {
		}
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		cont.push_back(x);
	}
	void insert(const T& x) {
		cont.insert(x);
	}
	SimIT insert(SimIT& it, const T& x) {
		return cont.insert(it, x);
	}

	// keep same deletion order in MT and non-MT version to reduce risk for desync
	SimIT erase_delete_synced(SimIT& it) {
		del.push_back(*it);
		return cont.erase(it);
	}

	bool can_delete_synced() {
		return !del.empty();
	}

	void delete_erased_synced() {
		for (VecIT it = del.begin(); it != del.end(); ++it) {
			delete *it;
		}
		del.clear();
	}

	void detach_erased_synced() {
		for (VecIT it = del.begin(); it != del.end(); ++it) {
			D::Detach(*it);
			delete *it;
		}
		del.clear();
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

	SimIT erase_delete(SimIT& it) {
		delete *it;
		return cont.erase(it);
	}

	SimIT erase_detach(SimIT& it) {
		delete *it;
		return cont.erase(it);
	}

public:
	typedef SimIT iterator;


public: //!needed by CREG
	C cont;
	std::vector<T> del;

private:
};


#endif // #ifndef TSC_H
