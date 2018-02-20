// By Emil Ernerfeldt 2014-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#pragma once

#include <cstdlib>
#include <iterator>
#include <utility>

#define DCHECK_EQ_F(a, b)
#define DCHECK_LT_F(a, b)
#define DCHECK_NE_F(a, b)

namespace emilib {

// like std::equal_to but no need to #include <functional>
template<typename T>
struct HashMapEqualTo
{
	constexpr bool operator()(const T &lhs, const T &rhs) const
	{
		return lhs == rhs;
	}
};

// A cache-friendly hash table with open addressing, linear probing and power-of-two capacity
template <typename KeyT, typename ValueT, typename HashT = std::hash<KeyT>, typename CompT = HashMapEqualTo<KeyT>>
class HashMap
{
public:
	using MyType = HashMap<KeyT, ValueT, HashT, CompT>;

	using PairT = std::pair<KeyT, ValueT>;
public:
	using key_type        = KeyT;
	using mapped_type     = ValueT;
	using size_type       = size_t;
	using value_type      = PairT;
	using reference       = PairT&;
	using const_reference = const PairT&;

	class iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = size_t;
		using distance_type     = size_t;
		using value_type        = std::pair<KeyT, ValueT>;
		using pointer           = value_type*;
		using reference         = value_type&;

		iterator() { }

		iterator(MyType* hash_map, size_t bucket) : _map(hash_map), _bucket(bucket)
		{
		}

		iterator& operator++()
		{
			this->goto_next_element();
			return *this;
		}

		iterator operator++(int)
		{
			size_t old_index = _bucket;
			this->goto_next_element();
			return iterator(_map, old_index);
		}

		reference operator*() const
		{
			return _map->_pairs[_bucket];
		}

		pointer operator->() const
		{
			return _map->_pairs + _bucket;
		}

		bool operator==(const iterator& rhs) const
		{
			DCHECK_EQ_F(_map, rhs._map);
			return this->_bucket == rhs._bucket;
		}

		bool operator!=(const iterator& rhs) const
		{
			DCHECK_EQ_F(_map, rhs._map);
			return this->_bucket != rhs._bucket;
		}

	private:
		void goto_next_element()
		{
			DCHECK_LT_F(_bucket, _map->_num_buckets);
			do {
				_bucket++;
			} while (_bucket < _map->_num_buckets && _map->_states[_bucket] != State::FILLED);
		}

	//private:
	//	friend class MyType;
	public:
		MyType* _map;
		size_t  _bucket;
	};

	class const_iterator
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using difference_type   = size_t;
		using distance_type     = size_t;
		using value_type        = const std::pair<KeyT, ValueT>;
		using pointer           = value_type*;
		using reference         = value_type&;

		const_iterator() { }

		const_iterator(iterator proto) : _map(proto._map), _bucket(proto._bucket)
		{
		}

		const_iterator(const MyType* hash_map, size_t bucket) : _map(hash_map), _bucket(bucket)
		{
		}

		const_iterator& operator++()
		{
			this->goto_next_element();
			return *this;
		}

		const_iterator operator++(int)
		{
			size_t old_index = _bucket;
			this->goto_next_element();
			return const_iterator(_map, old_index);
		}

		reference operator*() const
		{
			return _map->_pairs[_bucket];
		}

		pointer operator->() const
		{
			return _map->_pairs + _bucket;
		}

		bool operator==(const const_iterator& rhs) const
		{
			DCHECK_EQ_F(_map, rhs._map);
			return this->_bucket == rhs._bucket;
		}

		bool operator!=(const const_iterator& rhs) const
		{
			DCHECK_EQ_F(_map, rhs._map);
			return this->_bucket != rhs._bucket;
		}

	private:
		void goto_next_element()
		{
			DCHECK_LT_F(_bucket, _map->_num_buckets);
			do {
				_bucket++;
			} while (_bucket < _map->_num_buckets && _map->_states[_bucket] != State::FILLED);
		}

	//private:
	//	friend class MyType;
	public:
		const MyType* _map;
		size_t        _bucket;
	};

	// ------------------------------------------------------------------------

	HashMap() = default;
	HashMap(size_t num_elems) { reserve(num_elems); }
	HashMap(const std::initializer_list< std::pair<KeyT, ValueT> >& l)
	{
		reserve(l.size());
		for (const auto& pair: l) {
			insert(pair.first, pair.second);
		}
	}

	HashMap(const HashMap& other)
	{
		reserve(other.size());
		insert(other.cbegin(), other.cend());
	}

	HashMap(HashMap&& other)
	{
		*this = std::move(other);
	}

	HashMap& operator=(const HashMap& other)
	{
		clear();
		reserve(other.size());
		insert(other.cbegin(), other.cend());
		return *this;
	}

	void operator=(HashMap&& other)
	{
		this->swap(other);
	}

	~HashMap()
	{
		for (size_t bucket=0; bucket<_num_buckets; ++bucket) {
			if (_states[bucket] == State::FILLED) {
				_pairs[bucket].~PairT();
			}
		}
		free(_states);
		free(_pairs);
	}

	void swap(HashMap& other)
	{
		std::swap(_hasher,           other._hasher);
		std::swap(_comp,             other._comp);
		std::swap(_states,           other._states);
		std::swap(_pairs,            other._pairs);
		std::swap(_num_buckets,      other._num_buckets);
		std::swap(_num_filled,       other._num_filled);
		std::swap(_max_probe_length, other._max_probe_length);
		std::swap(_mask,             other._mask);
	}

	// -------------------------------------------------------------

	iterator begin()
	{
		size_t bucket = 0;
		while (bucket<_num_buckets && _states[bucket] != State::FILLED) {
			++bucket;
		}
		return iterator(this, bucket);
	}

	const_iterator cbegin() const { return (begin()); }
	const_iterator begin() const
	{
		size_t bucket = 0;
		while (bucket<_num_buckets && _states[bucket] != State::FILLED) {
			++bucket;
		}
		return const_iterator(this, bucket);
	}

	iterator end()
	{ return iterator(this, _num_buckets); }

	const_iterator cend() const { return (end()); }
	const_iterator end() const
	{ return const_iterator(this, _num_buckets); }

	size_t size() const
	{
		return _num_filled;
	}

	bool empty() const
	{
		return _num_filled==0;
	}

	// ------------------------------------------------------------

	iterator find(const KeyT& key)
	{
		auto bucket = this->find_filled_bucket(key);
		if (bucket == (size_t)-1) {
			return this->end();
		}
		return iterator(this, bucket);
	}

	const_iterator find(const KeyT& key) const
	{
		auto bucket = this->find_filled_bucket(key);
		if (bucket == (size_t)-1)
		{
			return this->end();
		}
		return const_iterator(this, bucket);
	}

	bool contains(const KeyT& k) const
	{
		return find_filled_bucket(k) != (size_t)-1;
	}

	size_t count(const KeyT& k) const
	{
		return find_filled_bucket(k) != (size_t)-1 ? 1 : 0;
	}

	// Returns the matching ValueT or nullptr if k isn't found.
	ValueT* try_get(const KeyT& k)
	{
		auto bucket = find_filled_bucket(k);
		if (bucket != (size_t)-1) {
			return &_pairs[bucket].second;
		} else {
			return nullptr;
		}
	}

	// Const version of the above
	const ValueT* try_get(const KeyT& k) const
	{
		auto bucket = find_filled_bucket(k);
		if (bucket != (size_t)-1) {
			return &_pairs[bucket].second;
		} else {
			return nullptr;
		}
	}

	// Convenience function.
	const ValueT get_or_return_default(const KeyT& k) const
	{
		const ValueT* ret = try_get(k);
		if (ret) {
			return *ret;
		} else {
			return ValueT();
		}
	}

	// -----------------------------------------------------

	// Returns a pair consisting of an iterator to the inserted element
	// (or to the element that prevented the insertion)
	// and a bool denoting whether the insertion took place.
	std::pair<iterator, bool> insert(const KeyT& key, const ValueT& value)
	{
		check_expand_need();

		auto bucket = find_or_allocate(key);

		if (_states[bucket] == State::FILLED) {
			return { iterator(this, bucket), false };
		} else {
			_states[bucket] = State::FILLED;
			new(_pairs + bucket) PairT(key, value);
			_num_filled++;
			return { iterator(this, bucket), true };
		}
	}
	std::pair<iterator, bool> emplace(const KeyT& key, const ValueT& value) {
		return (insert(key, value)); // [SPRING] just here for compatibility with std::unordered_map
	}

	std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT>& p)
	{
		return insert(p.first, p.second);
	}

	void insert(const_iterator begin, const_iterator end)
	{
		for (; begin != end; ++begin) {
			insert(begin->first, begin->second);
		}
	}

	// Same as above, but contains(key) MUST be false
	void insert_unique(KeyT&& key, ValueT&& value)
	{
		assert(!contains(key));
		check_expand_need();
		auto bucket = find_empty_bucket(key);
		_states[bucket] = State::FILLED;
		new(_pairs + bucket) PairT(std::move(key), std::move(value));
		_num_filled++;
	}

	void insert_unique(std::pair<KeyT, ValueT>&& p)
	{
		insert_unique(std::move(p.first), std::move(p.second));
	}

	// Return the old value or ValueT() if it didn't exist.
	ValueT set_get(const KeyT& key, const ValueT& new_value)
	{
		check_expand_need();

		auto bucket = find_or_allocate(key);

		// Check if inserting a new value rather than overwriting an old entry
		if (_states[bucket] == State::FILLED) {
			ValueT old_value = _pairs[bucket].second;
			_pairs[bucket] = new_value.second;
			return old_value;
		} else {
			_states[bucket] = State::FILLED;
			new(_pairs + bucket) PairT(key, new_value);
			_num_filled++;
			return ValueT();
		}
	}

	// Like std::map<KeyT,ValueT>::operator[].
	ValueT& operator[](const KeyT& key)
	{
		check_expand_need();

		auto bucket = find_or_allocate(key);

		/* Check if inserting a new value rather than overwriting an old entry */
		if (_states[bucket] != State::FILLED) {
			_states[bucket] = State::FILLED;
			new(_pairs + bucket) PairT(key, ValueT());
			_num_filled++;
		}

		return _pairs[bucket].second;
	}

	// -------------------------------------------------------

	/* Erase an element from the hash table.
	   return false if element was not found */
	bool erase(const KeyT& key)
	{
		auto bucket = find_filled_bucket(key);
		if (bucket != (size_t)-1) {
			_states[bucket] = State::ACTIVE;
			_pairs[bucket].~PairT();
			_num_filled -= 1;
			return true;
		} else {
			return false;
		}
	}

	/* Erase an element using an iterator.
	   Returns an iterator to the next element (or end()). */
	iterator erase(iterator it)
	{
		DCHECK_EQ_F(it._map, this);
		DCHECK_LT_F(it._bucket, _num_buckets);
		_states[it._bucket] = State::ACTIVE;
		_pairs[it._bucket].~PairT();
		_num_filled -= 1;
		return ++it;
	}

	// Remove all elements, keeping full capacity.
	void clear()
	{
		for (size_t bucket=0; bucket<_num_buckets; ++bucket) {
			if (_states[bucket] == State::FILLED) {
				_states[bucket] = State::INACTIVE;
				_pairs[bucket].~PairT();
			}
		}
		_num_filled = 0;
		_max_probe_length = -1;
	}

	// Make room for this many elements
	void reserve(size_t num_elems)
	{
		size_t required_buckets = num_elems + num_elems/2 + 1;
		if (required_buckets <= _num_buckets) {
			return;
		}
		size_t num_buckets = 4;
		while (num_buckets < required_buckets) { num_buckets *= 2; }

		auto new_states = (State*)malloc(num_buckets * sizeof(State));
		auto new_pairs  = (PairT*)malloc(num_buckets * sizeof(PairT));

		if (!new_states || !new_pairs) {
			free(new_states);
			free(new_pairs);
			throw std::bad_alloc();
		}

		//auto old_num_filled  = _num_filled;
		auto old_num_buckets = _num_buckets;
		auto old_states      = _states;
		auto old_pairs       = _pairs;

		_num_filled  = 0;
		_num_buckets = num_buckets;
		_mask        = _num_buckets - 1;
		_states      = new_states;
		_pairs       = new_pairs;

		std::fill_n(_states, num_buckets, State::INACTIVE);

		_max_probe_length = -1;

		for (size_t src_bucket=0; src_bucket<old_num_buckets; src_bucket++) {
			if (old_states[src_bucket] == State::FILLED) {
				auto& src_pair = old_pairs[src_bucket];

				auto dst_bucket = find_empty_bucket(src_pair.first);
				DCHECK_NE_F(dst_bucket, (size_t)-1);
				DCHECK_NE_F(_states[dst_bucket], State::FILLED);
				_states[dst_bucket] = State::FILLED;
				new(_pairs + dst_bucket) PairT(std::move(src_pair));
				_num_filled += 1;

				src_pair.~PairT();
			}
		}

		//DCHECK_EQ_F(old_num_filled, _num_filled);

		free(old_states);
		free(old_pairs);
	}

private:
	// Can we fit another element?
	void check_expand_need()
	{
		reserve(_num_filled + 1);
	}

	// Find the bucket with this key, or return nullptr
	size_t find_filled_bucket(const KeyT& key) const
	{
		if (empty()) { return (size_t)-1; } // Optimization

		auto hash_value = _hasher(key);
		for (int offset=0; offset<=_max_probe_length; ++offset) {
			auto bucket = (hash_value + offset) & _mask;
			if (_states[bucket] == State::FILLED && _comp(_pairs[bucket].first, key)) {
				return bucket;
			}
			if (_states[bucket] == State::INACTIVE) {
				return (size_t)-1; // End of the chain!
			}
		}
		return (size_t)-1;
	}

	// Find the bucket with this key, or return a good empty bucket to place the key in.
	// In the latter case, the bucket is expected to be filled.
	size_t find_or_allocate(const KeyT& key)
	{
		auto hash_value = _hasher(key);
		size_t hole = (size_t)-1;
		int offset=0;
		for (; offset<=_max_probe_length; ++offset) {
			auto bucket = (hash_value + offset) & _mask;

			if (_states[bucket] == State::FILLED) {
				if (_comp(_pairs[bucket].first, key)) {
					return bucket;
				}
			} else if (_states[bucket] == State::INACTIVE) {
				return bucket;
			} else {
				// ACTIVE: keep searching
				if (hole == (size_t)-1) {
					hole = bucket;
				}
			}
		}

		// No key found - but maybe a hole for it

		DCHECK_EQ_F(offset, _max_probe_length+1);

		if (hole != (size_t)-1) {
			return hole;
		}

		// No hole found within _max_probe_length
		for (; ; ++offset) {
			auto bucket = (hash_value + offset) & _mask;

			if (_states[bucket] != State::FILLED) {
				_max_probe_length = offset;
				return bucket;
			}
		}
	}

	// key is not in this map. Find a place to put it.
	size_t find_empty_bucket(const KeyT& key)
	{
		auto hash_value = _hasher(key);
		for (int offset=0; ; ++offset) {
			auto bucket = (hash_value + offset) & _mask;
			if (_states[bucket] != State::FILLED) {
				if (offset > _max_probe_length) {
					_max_probe_length = offset;
				}
				return bucket;
			}
		}
		return (size_t(-1));
	}

private:
	enum class State : uint8_t
	{
		INACTIVE, // Never been touched
		ACTIVE,   // Is inside a search-chain, but is empty
		FILLED    // Is set with key/value
	};

	HashT   _hasher;
	CompT   _comp;
	State*  _states           = nullptr;
	PairT*  _pairs            = nullptr;
	size_t  _num_buckets      =  0;
	size_t  _num_filled       =  0;
	int     _max_probe_length = -1; // Our longest bucket-brigade is this long. ONLY when we have zero elements is this ever negative (-1).
	size_t  _mask             =  0;  // _num_buckets minus one
};

} // namespace emilib

