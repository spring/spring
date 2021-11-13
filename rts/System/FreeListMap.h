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
			if (id >= values.size())
				values.resize(id);

			return values[id];
		}

		std::size_t GetId() {
			if (freeIDs.empty())
				return nextId++;

			std::size_t id = freeIDs.back();
			freeIDs.pop_back();

			return id;
		}

		void FreeId(std::size_t id) {
			if (id > 0 && id == nextId) {
				--nextId;
				values.erase(id);
				return;
			}

			freeIDs.emplace_back(id);
			values.erase(id);
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