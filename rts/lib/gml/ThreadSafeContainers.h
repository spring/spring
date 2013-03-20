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
#include "gmlcnf.h"
#include <set>
#include <map>

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





template <class C, class X, class R, class T, class D>
class ThreadListRender {
private:
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListRender);

	~ThreadListRender() {
		clear();
	}

	void clear() {
	}

	void PostLoad() {
	}

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

	void remove_erased_synced() {
	}

public:
	//! RENDER/UNSYNCED METHODS
	void delay_add() {
	}

	void add_delayed() {
	}

	void erase_delete(const T& x) {
		D::Remove(x);
	}

	void delay_delete() {
	}

	void delete_delayed() {
	}

	void enqueue(const T& x) {
		D::Add(x);
	}

	void delay() {
	}

	void execute() {
	}

	void clean() {
	}

	void clean(std::vector<C> *delQueue) {
	}

	std::vector<C> *to_destroy() {
		return NULL;
	}

	void dequeue(const T& x) {
		D::Remove(x);
	}

	void dequeue_synced(const C& x) {
		simDelQueue.push_back(x);
	}

	void destroy() {
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


template <class C, class K, class V, class I>
class ThreadMapRender {
private:
	typedef std::map<K,V> TMapC;

public:
	CR_DECLARE_STRUCT(ThreadMapRender);

	~ThreadMapRender() {
		clear();
	}

	const TMapC& get_render_map() const { return contRender; }

	void clear() {
	}

	void PostLoad() {
	}

	//! SIMULATION/SYNCED METHODS
	void push(const C& x, const V& y) {
		contRender[I::Index(x)] = y;
	}

public:
	//! RENDER/UNSYNCED METHODS
	void delay_add() {
	}

	void add_delayed() {
	}

	void erase_delete(const C& x) {
		contRender.erase(I::Index(x));
	}

	void delay_delete() {
	}

	void delete_delayed() {
	}

private:
	TMapC contRender;
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
		clear();
	}

	void clear() {
		for(SimIT it = cont.begin(); it != cont.end(); ++it)
			delete *it;
		for (VecIT it = delRender.begin(); it != delRender.end(); ++it)
			delete *it;
		for (SetIT it = postDelRender.begin(); it != postDelRender.end(); ++it)
			delete *it;
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
		return set_erase(cont, it);
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
		return set_erase(cont, it);
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
				it = set_erase(postDelRender, it);
				delete s;
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








template <class C, class X, class R, class T, class D>
class ThreadListRender {
private:
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
	typedef typename std::vector<T>::const_iterator VecIT;
	typedef typename std::vector<C>::const_iterator VecITC;
	typedef typename std::vector<T>::iterator VecITT;
	typedef typename std::vector<X>::iterator VecITX;

public:
	CR_DECLARE_STRUCT(ThreadListRender);

	~ThreadListRender() {
		clear();
	}

	void clear() {
		delay_delete();
		for(typename R::iterator it = contRender.begin(); it!=contRender.end(); ++it)
			addRender.insert(*it);
		contRender.clear();
		delete_delayed();
	}

	void PostLoad() {
	}

	//! SIMULATION/SYNCED METHODS
	void push(const T& x) {
		preAddRender.insert(x);
	}
	void insert(const T& x) {
		preAddRender.insert(x);
	}

	void erase_remove_synced(const T& x) {
		removeRender.push_back(x);
	}

	void remove_erased_synced() {
		for (VecIT it = removeRender.begin(); it != removeRender.end(); ++it) {
			T s = *it;
			size_t d;
			if(!(d = contRender.erase(s)) && !addRender.erase(s) && !preAddRender.erase(s))
				assert(false);
			if(d)
				D::Remove(s);
		}
		removeRender.clear();
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
			D::Add(*it);
			contRender.insert(*it);
		}
		addRender.clear();
	}

	void erase_delete(const T& x) {
		delRender.push_back(x);
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
			size_t d;
			if((d = contRender.erase(s)) || addRender.erase(s)) {
				it = set_erase(postDelRender, it);
				if(d)
					D::Remove(s);
				D::Delete(s);
			}
			else
				++it;
		}
	}

	void enqueue(const T& x) {
		simQueue.push_back(x);
	}

	void delay() {
		for (VecIT it = simQueue.begin(); it != simQueue.end(); ++it) {
			sharedQueue.push_back(*it);
		}
		simQueue.clear();
	}

	void execute() {
		for (VecIT it = sharedQueue.begin(); it != sharedQueue.end(); ++it) {
			D::Add(*it);
		}
		sharedQueue.clear();
	}

	void clean() {
		cleandef(&simDelQueue);
	}

	void cleandef(std::vector<T> *delQueue) {
		delay();
		for (VecITT it = sharedQueue.begin(); it != sharedQueue.end(); ) {
			bool found = false;
			for (VecITT it2 = delQueue->begin(); it2 != delQueue->end(); ++it2) {
				if(*it == *it2) {
					found = true;
					break;
				}
			}
			if(found)
				it = sharedQueue.erase(it);
			else
				++it;
		}
	}

	void clean(std::vector<X> *delQueue) {
		delay();
		for (VecITT it = sharedQueue.begin(); it != sharedQueue.end(); ) {
			bool found = false;
			for (VecITX it2 = delQueue->begin(); it2 != delQueue->end(); ++it2) {
				if(*it == *it2) {
					found = true;
					break;
				}
			}
			if(found)
				it = sharedQueue.erase(it);
			else
				++it;
		}
	}

	std::vector<T> *to_destroy() {
		return &simDelQueue;
	}

	void dequeue(const T& x) {
		simDelQueue.push_back(x);
	}

	void dequeue_synced(const T& x) {
		simDelQueue.push_back(x);
	}

	void destroy() {
		for (VecITT it = simDelQueue.begin(); it != simDelQueue.end(); ++it) {
			D::Remove(*it);
		}
		simDelQueue.clear();
	}

	void destroy_synced() {
		for (VecITT it = simDelQueue.begin(); it != simDelQueue.end(); ++it) {
			D::Remove(*it);
		}
		simDelQueue.clear();
	}

private:
	std::vector<T> simQueue;
	std::vector<T> sharedQueue;
	std::vector<T> simDelQueue;

	std::set<T> preAddRender;
	std::set<T> addRender;
	R contRender;
	std::vector<T> delRender;
	std::set<T> postDelRender;
	std::vector<T> removeRender;
};



template <class C, class K, class V, class I>
class ThreadMapRender {
private:
	typedef std::map<K,V> TMapC;
	typedef std::map<C,V> TMap;
	typedef std::set<C> TSet;
	typedef typename TMap::const_iterator constMapIT;
	typedef typename TMap::iterator MapIT;
	typedef typename TSet::const_iterator constSetIT;
	typedef typename TSet::iterator SetIT;

public:
	CR_DECLARE_STRUCT(ThreadMapRender);

	~ThreadMapRender() {
		clear();
	}

	const TMapC& get_render_map() const { return contRender; }

	void clear() {
	}

	void PostLoad() {
	}

	//! SIMULATION/SYNCED METHODS
	void push(const C& x, const V& y) {
		preAddRender[x] = y;
	}

public:
	//! RENDER/UNSYNCED METHODS
	void delay_add() {
		for (constMapIT it = preAddRender.begin(); it != preAddRender.end(); ++it) {
			addRender[it->first] = it->second;
		}
		preAddRender.clear();
	}

	void add_delayed() {
		for (constMapIT it = addRender.begin(); it != addRender.end(); ++it) {
			contRender[I::Index(it->first)] = it->second;
		}
		addRender.clear();
	}

	void erase_delete(const C& x) {
		delRender.insert(x);
	}

	void delay_delete() {
		for (constSetIT it = delRender.begin(); it != delRender.end(); ++it) {
			postDelRender.insert(*it);
		}
		delRender.clear();
	}

	void delete_delayed() {
		for (SetIT it = postDelRender.begin(); it != postDelRender.end();) {
			C s = *it;
			if((contRender.erase(I::Index(s))) || addRender.erase(s)) {
				it = set_erase(postDelRender, it);
			}
			else {
				++it;
			}
		}
	}

private:
	TMap preAddRender;
	TMap addRender;
	TMapC contRender;
	TSet delRender;
	TSet postDelRender;
};

#endif



template <class C, class R, class T, class D>
class ThreadListSim {
private:
	typedef typename C::iterator SimIT;
	typedef typename std::set<T>::const_iterator constSetIT;
	typedef typename std::set<T>::iterator SetIT;
	typedef typename std::vector<T>::const_iterator VecIT;

public:
	CR_DECLARE_STRUCT(ThreadListSim);

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
#if !defined(USE_GML) || !GML_ENABLE_SIM
			delete *it;
#endif
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
#if !defined(USE_GML) || !GML_ENABLE_SIM
		delete *it;
#endif
		return cont.erase(it);
	}

	SimIT erase_detach(SimIT& it) {
#if !defined(USE_GML) || !GML_ENABLE_SIM
		delete *it;
#else
		D::Detach(*it);
#endif
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
