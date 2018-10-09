/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include <array>
#include <vector>

#include "Sim/Objects/SolidObject.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"


template<typename T, size_t S> struct ArrayVector {
public:
	CR_DECLARE_STRUCT(ArrayVector)

	ArrayVector() = default;
	ArrayVector(size_t n) { vec.reserve(n); }
	ArrayVector(const ArrayVector& c) = delete;
	ArrayVector(ArrayVector&& c) { *this = std::move(c); }

	ArrayVector& operator = (const ArrayVector& c) = delete;
	ArrayVector& operator = (ArrayVector&& c) {
		arr = c.arr;
		vec = std::move(c.vec);

		arrSize = c.arrSize;
		return *this;
	}

	void clear() {
		arr.fill(nullptr);
		vec.clear();

		arrSize = 0;
	}


	T* operator [] (size_t i) const {
		assert(i < size());
		assert(i < arrSize || (i - arrSize) < vec.size());

		return ((i < arrSize)? arr[i]: vec[i - arrSize]);
	}


	size_t size() const { return (arrSize + vec.size()); }

	bool empty() const { return (arrSize == 0); }
	bool contains(const T* o) const {
		if (empty())
			return false;

		const auto ab = arr.begin();
		const auto ae = arr.begin() + arrSize;

		return ((std::find(ab, ae, o) != ae) || (std::find(vec.begin(), vec.end(), o) != vec.end()));
	}

	bool insert_unique(T* o) { return (!contains(o) && insert(o)); }
	bool insert(T* o) {
		if (arrSize < S) {
			arr[arrSize++] = o;
		} else {
			vec.push_back(o);
		}
		return true;
	}

	bool erase(T* o) {
		const auto it = std::find(arr.begin(), arr.end(), o);

		if (it != arr.end()) {
			if (vec.empty()) {
				*it = arr[--arrSize];
				arr[arrSize] = nullptr;
			} else {
				// never allow a hole between array and vector parts
				*it = vec.back();
				vec.pop_back();
			}

			return true;
		}

		const auto jt = std::find(vec.begin(), vec.end(), o);

		if (jt != vec.end()) {
			*jt = vec.back();
			vec.pop_back();
			return true;
		}

		return false;
	}

public:
	// how many objects currently exist in arr; vec will
	// always be empty if this is smaller than arr.size()
	size_t arrSize = 0;

	std::array<T*, S> arr;
	std::vector<T*> vec;
};



typedef ArrayVector<CSolidObject, 8> BlockingMapCell;
typedef std::vector<BlockingMapCell> BlockingMap;

class CGroundBlockingObjectMap
{
	CR_DECLARE_STRUCT(CGroundBlockingObjectMap)

public:
	void Init(unsigned int numSquares) { groundBlockingMap.resize(numSquares); }
	void Kill() {
		// reuse inner vectors when reloading
		// groundBlockingMap.clear();
		for (BlockingMapCell& v: groundBlockingMap) {
			v.clear();
		}
	}

	unsigned int CalcChecksum() const;

	void AddGroundBlockingObject(CSolidObject* object);
	void AddGroundBlockingObject(CSolidObject* object, const YardMapStatus& mask);
	void RemoveGroundBlockingObject(CSolidObject* object);

	void OpenBlockingYard(CSolidObject* object);
	void CloseBlockingYard(CSolidObject* object);
	bool CanOpenYard(CSolidObject* object) const;
	bool CanCloseYard(CSolidObject* object) const;


	// these retrieve either the first object in
	// a given cell, or NULL if the cell is empty
	CSolidObject* GroundBlocked(int x, int z) const;
	CSolidObject* GroundBlocked(const float3& pos) const;

	// same as GroundBlocked(), but does not bounds-check mapSquare
	CSolidObject* GroundBlockedUnsafe(unsigned int mapSquare) const {
		const BlockingMapCell& cell = GetCellUnsafeConst(mapSquare);

		if (cell.empty())
			return nullptr;

		return cell[0];
	}


	bool GroundBlocked(int x, int z, const CSolidObject* ignoreObj) const;
	bool GroundBlocked(const float3& pos, const CSolidObject* ignoreObj) const;

	bool ObjectInCell(unsigned int mapSquare, const CSolidObject* obj) const {
		if (mapSquare >= groundBlockingMap.size())
			return false;

		return (groundBlockingMap[mapSquare].contains(obj));
	}


	const BlockingMapCell& GetCellUnsafeConst(unsigned int mapSquare) const {
		assert(mapSquare < groundBlockingMap.size());
		return groundBlockingMap[mapSquare];
	}

	BlockingMapCell& GetCellUnsafe(unsigned int mapSquare) {
		return (const_cast<BlockingMapCell&>(GetCellUnsafeConst(mapSquare)));
	}

private:
	bool CheckYard(CSolidObject* yardUnit, const YardMapStatus& mask) const;

private:
	BlockingMap groundBlockingMap;
};

extern CGroundBlockingObjectMap groundBlockingObjectMap;

#endif
