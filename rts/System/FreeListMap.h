#pragma once

#include <vector>
#include <stdexcept>

#include "System/creg/creg_cond.h"
#include "System/Cpp11Compat.hpp"
#include "System/UnorderedMap.hpp"
#include "System/ContainerUtil.h"

namespace spring {

	// Stored elements are not reordered, but the array might have gaps of TVal{}
	template<typename TVal>
	class FreeListMap {
		CR_DECLARE_STRUCT(FreeListMap<TVal>)
	public:
		const TVal& operator[](std::size_t id) const {
			assert(id < values.size());
			return values[id];
		}

		TVal& operator[](std::size_t id) {
			if (id + 1 >= values.size())
				values.resize(id + 1);

			return values[id];
		}

		template<typename ShuffleFunc>
		std::size_t Add(const TVal& val, ShuffleFunc&& shuffleFunc) {
			const std::size_t id = GetId(shuffleFunc);
			operator[](id) = val;

			return id;
		}

		std::size_t Add(const TVal& val) {
			const std::size_t id = GetId();
			operator[](id) = val;

			return id;
		}

		void Del(std::size_t id) {
			if (id > 0 && id == nextId) {
				--nextId;
				values.erase(values.cbegin() + id);
				return;
			}

			freeIDs.emplace_back(id);
			values[id] = TVal{};
		}

		void resize(std::size_t sz) { values.resize(sz); }
		void reserve(std::size_t sz) { values.reserve(sz); }
		void clear() {
			values.clear();
			freeIDs.clear();
			nextId = 0;
		}

		template<typename ShuffleFunc>
		void ShuffleFreeIDs(ShuffleFunc&& shuffleFunc) {
			spring::random_shuffle(freeIDs.begin(), freeIDs.end(), shuffleFunc);
		}

		constexpr auto begin()        { return values.begin(); }
		constexpr auto end()          { return values.end();   }
		constexpr auto cbegin() const { return values.begin(); }
		constexpr auto cend()   const { return values.end();   }
		constexpr auto begin()  const { return values.begin(); }
		constexpr auto end()    const { return values.end();   }

		constexpr auto size()   const { return values.size(); }
		constexpr auto ssize()  const {
			using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(values.size())>>;
			return static_cast<R>(values.size());
		}

		const std::vector<TVal>& GetData() const { return values; }
		      std::vector<TVal>& GetData()       { return values; }
	private:
		template<typename ShuffleFunc>
		std::size_t GetId(ShuffleFunc&& shuffleFunc) {
			if (freeIDs.empty())
				return nextId++;

			size_t idx = shuffleFunc(freeIDs.size());
			const std::size_t id = freeIDs[idx];

			freeIDs[idx] = freeIDs.back();
			freeIDs.pop_back();

			return id;
		}

		std::size_t GetId() {
			if (freeIDs.empty())
				return nextId++;

			const std::size_t id = freeIDs.back();
			freeIDs.pop_back();

			return id;
		}
	private:
		std::vector<TVal> values;
		std::vector<std::size_t> freeIDs;
		std::size_t nextId = 0;
	};

	// Stored elements might be reordered, but the array won't have any gaps
	template<typename TVal, typename TKey = int>
	class FreeListMapCompact {
		using TypedFreeListMapCompact = FreeListMapCompact<TVal, TKey>;
		CR_DECLARE_STRUCT(TypedFreeListMapCompact) //can't pass the original type with <A,B> due to macros limitations
	public:
		const TVal& operator[](std::size_t idx) const {
			assert(idx < vault.size());
			return vault[idx];
		}

		TVal& operator[](std::size_t idx) {
			assert(idx < vault.size());
			return vault[idx];
		}

		void SeedFreeKeys(const TKey& first, const TKey& last, bool init = false) {
			for (TKey k = first; k < last; ++k) {
				spring::VectorInsertUnique(freeKeys, k, !init);
			}
		}

		template<typename ShuffleFunc>
		TKey Add(const TVal& val, ShuffleFunc&& shuffleFunc) {
			TKey key = GetKey(shuffleFunc);
			const auto pos = vault.size();
			kpMap[key] = pos;
			pkMap[pos] = key;
			vault.emplace_back(val);

			return key;
		}

		TKey Add(const TVal& val) {
			TKey key = GetKey();
			const auto pos = vault.size();
			kpMap[key] = pos;
			pkMap[pos] = key;
			vault.emplace_back(val);

			return key;
		}

		void Del(const TKey& key) {
			const auto it = kpMap.find(key);

			assert(it != kpMap.end());

			//last elem in vault
			if (it->second == vault.size() - 1) {
				vault.pop_back();
				freeKeys.emplace_back(key);
				pkMap.erase(it->second);
				kpMap.erase(it);
				return;
			}

			const auto it2 = pkMap.find(vault.size() - 1); //find the key that corresponds to the last position in vault
			assert(it2 != pkMap.end());

			//iterators might become invalid, use values
			const auto kp = *it;
			const auto pk = *it2;

			kpMap.erase(it);
			pkMap.erase(it2);

			vault[kp.second] = vault.back();
			kpMap[pk.second] = kp.second;
			pkMap[kp.second] = pk.second;

			vault.pop_back();
			freeKeys.emplace_back(key);
		}

		size_t Find(const TKey& key) const {
			const auto it = kpMap.find(key);
			if (it == kpMap.cend())
				return size_t(-1);

			return it->second;
		}

		void reserve(std::size_t sz) { vault.reserve(sz); }
		void clear() {
			vault.clear();
			kpMap.clear();
			pkMap.clear();
			freeKeys.clear();
		}

		constexpr auto begin()        { return vault.begin(); }
		constexpr auto end()          { return vault.end();   }
		constexpr auto cbegin() const { return vault.begin(); }
		constexpr auto cend()   const { return vault.end();   }
		constexpr auto begin()  const { return vault.begin(); }
		constexpr auto end()    const { return vault.end();   }

		constexpr auto size()   const { return vault.size(); }
		constexpr auto ssize()  const {
			using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(vault.size())>>;
			return static_cast<R>(vault.size());
		}

		const std::vector<TVal>& GetData() const { return vault; }
		      std::vector<TVal>& GetData()       { return vault; }
	private:
		template<typename ShuffleFunc>
		TKey GetKey(ShuffleFunc&& shuffleFunc) {
			if (freeKeys.empty())
				return kpMap.size();

			size_t idx = shuffleFunc(freeKeys.size());
			const TKey key = freeKeys[idx];

			freeKeys[idx] = freeKeys.back();
			freeKeys.pop_back();

			return key;
		}

		std::size_t GetKey() {
			if (freeKeys.empty())
				return kpMap.size();

			const TKey key = freeKeys.back();
			freeKeys.pop_back();

			return key;
		}
	private:
		std::vector<TVal> vault;
		std::vector<TKey> freeKeys;
		spring::unordered_map<TKey, size_t> kpMap; // key --> pos
		spring::unordered_map<size_t, TKey> pkMap; // pos --> key
	};
}


CR_BIND_TEMPLATE_1TYPED(spring::FreeListMap, TVal, )
CR_REG_METADATA_TEMPLATE_1TYPED(spring::FreeListMap, TVal, (
	CR_MEMBER(values),
	CR_MEMBER(freeIDs),
	CR_MEMBER(nextId)
))

CR_BIND_TEMPLATE_2TYPED(spring::FreeListMapCompact, TVal, Tkey, )
CR_REG_METADATA_TEMPLATE_2TYPED(spring::FreeListMapCompact, TVal, TKey, (
	CR_MEMBER(vault),
	CR_MEMBER(freeKeys),
	CR_MEMBER(kpMap),
	CR_MEMBER(pkMap)
))
