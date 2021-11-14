#pragma once

#include <vector>
#include "System/creg/creg_cond.h"

namespace spring {
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

		const std::vector<TVal>& GetData() const { return values; }
		std::vector<TVal>& GetData() { return values; }
	private:
		std::size_t GetId() {
			if (freeIDs.empty())
				return nextId++;

			std::size_t id = freeIDs.back();
			freeIDs.pop_back();

			return id;
		}


	private:
		std::vector<TVal> values;
		std::vector<std::size_t> freeIDs;
		std::size_t nextId = 0;
	};

	template<typename TVal>
	class FreeListMapSaved : public FreeListMap<TVal> {
		CR_DECLARE_STRUCT(FreeListMapSaved<TVal>)
	};
}