//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#ifndef JC_PTR_VEC_H
#define JC_PTR_VEC_H

#include <vector>

// ptrvec is a container with constant time adding and removing of elements
// element order is not kept

template<typename T>
class ptrvec_getindex {
public:
	int& operator()(T *v) { return v->index; }
};

template<typename T, typename getindex_t = ptrvec_getindex<T>, typename STL_Alloc=std::allocator<T> >
class ptrvec {
public:
	typedef std::vector <T*, typename STL_Alloc::template rebind<T*>::other > vec_t;

	struct iterator
	{
		typename vec_t::iterator it;
		iterator(typename vec_t::iterator p) : it(p) {}
		iterator operator++() { ++it; return *this; }
		iterator operator++(int) { iterator n=*this; ++it; return n; }
		iterator operator--() { --it; return *this; }
		iterator operator--(int) { iterator n=*this; --it; return n; }
		T& operator*() { return **it; }
		T* operator->() { return *it; }
		bool operator==(const iterator& t) const { return t.it == it; }
		bool operator!=(const iterator& t) const { return t.it != it; }
	};

	iterator begin() { return iterator(vec.begin()); }
	iterator end() { return iterator(vec.end()); }

	ptrvec() {}
	~ptrvec() { clear(); }
	void clear() {
		typename vec_t::iterator i;
		for(i=vec.begin();i!=vec.end();++i)
			delete *i;
		vec.clear();
	}
	T* operator[](int i) { return vec[i]; }
	const T* operator[](int i) const { return vec[i]; }
	void erase(T* elem) {
		assert (vec[getindex(elem)]==elem);
		if (getindex(elem) < (int)(vec.size()-1)) {
			getindex(vec.back()) = getindex(elem);
			swap (vec[getindex(elem)], vec.back());
		}
		vec.pop_back();
	}
	void del(T* elem) {
		erase(elem);
		delete elem;
	}
	T* add() {
		T *p = new T;
		getindex( p ) = (int)vec.size();
		vec.push_back (p);
		return p;
	}
	T* add(T* elem) {
		getindex( elem ) = (int)vec.size();
		vec.push_back (elem);
		return elem;
	}
	size_t size() const { return vec.size(); }
	bool empty() const { return vec.empty(); }

	vec_t vec;
private:
	ptrvec(const ptrvec& v){}
	void operator=(const ptrvec& v){}
	getindex_t getindex;
};


#endif // JC_PTR_VEC_H
