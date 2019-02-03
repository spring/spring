/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDBLOCKINGOBJECTMAP_H
#define GROUNDBLOCKINGOBJECTMAP_H

#include <array>
#include <vector>

#include "Sim/Objects/SolidObject.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"

class CGroundBlockingObjectMap
{
	CR_DECLARE_STRUCT(CGroundBlockingObjectMap)

private:
	template<typename T, uint32_t S = 8> struct ArrayCell {
	public:
		CR_DECLARE_STRUCT(ArrayCell)

		void Clear() {
			arr.fill(nullptr);

			numObjs = 0;
			vecIndx = 0; // point to dummy
		}

		bool InsertUnique(T* o) { return (!Contains(o) && Insert(o)); }
		bool Insert(T* o) {
			if (Full())
				return false;

			arr[numObjs++] = o;
			return true;
		}

		bool Erase(T* o) {
			const auto ae = arr.begin() + numObjs;
			const auto it = std::find(arr.begin(), ae, o);

			if (it == ae)
				return false;

			*it = arr[--numObjs];
			arr[numObjs] = nullptr;
			return true;
		}

		bool Empty() const { return (numObjs == 0); }
		bool Full() const { return (numObjs == S); }
		bool Contains(const T* o) const {
			const auto ab = arr.begin();
			const auto ae = arr.begin() + numObjs;

			return (std::find(ab, ae, o) != ae);
		}

		T* operator [] (size_t i) const { return arr[i]; }

		uint32_t GetNumObjs() const { return numObjs; }
		uint32_t GetVecIndx() const { return vecIndx; }
		uint32_t SetVecIndx(uint32_t i) { return (vecIndx = i); }
	
	private:
		uint32_t numObjs = 0;
		uint32_t vecIndx = 0;

		std::array<T*, S> arr;
	};

	typedef ArrayCell<CSolidObject> ArrCell;
	typedef std::vector<CSolidObject*> VecCell;

public:
	struct BlockingMapCell {
	public:
		BlockingMapCell() = delete;
		BlockingMapCell(const ArrCell& ac, const VecCell* vc): arrCell(ac), vecCell(vc) {
		}

		VecCell::value_type operator [] (size_t i) const {
			assert(i < size());
			assert(i < arrCell.GetNumObjs() || (i - arrCell.GetNumObjs()) < vecCell[ arrCell.GetVecIndx() ].size());

			return ((i < arrCell.GetNumObjs())? arrCell[i]: vecCell[ arrCell.GetVecIndx() ][ i - arrCell.GetNumObjs() ]);
		}

		size_t size() const { return (arrCell.GetNumObjs() + vsize()); }
		size_t vsize() const { return ((arrCell.Full())? vecCell[ arrCell.GetVecIndx() ].size(): 0); }

		bool empty() const { return (arrCell.Empty()); }

	private:
		const ArrCell& arrCell;
		const VecCell* vecCell;
	};


	void Init(unsigned int numSquares) {
		arrCells.resize(numSquares);
		vecCells.reserve(32);
		vecIndcs.reserve(32);

		// add dummy
		if (vecCells.empty())
			vecCells.emplace_back();
	}
	void Kill() {
		// reuse inner vectors when reloading
		// vecCells.clear();
		for (auto& v: arrCells) {
			v.Clear();
		}
		for (auto& v: vecCells) {
			v.clear();
		}

		vecIndcs.clear();
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
		if (mapSquare >= arrCells.size())
			return false;

		const ArrCell& ac = GetArrCell(mapSquare);
		const VecCell* vc = nullptr;

		if (ac.Contains(obj))
			return true;

		return (((vc = &GetVecCell(mapSquare)) != &vecCells[0]) && (std::find(vc->begin(), vc->end(), obj) != vc->end()));
	}


	BlockingMapCell GetCellUnsafeConst(unsigned int mapSquare) const {
		assert(mapSquare < arrCells.size());
		// avoid vec-cell lookup unless needed
		return {GetArrCell(mapSquare), vecCells.data()};
	}

private:
	bool CheckYard(CSolidObject* yardUnit, const YardMapStatus& mask) const;

	const ArrCell& GetArrCell(unsigned int mapSquare) const { return           arrCells[mapSquare]               ; }
	      ArrCell& GetArrCell(unsigned int mapSquare)       { return           arrCells[mapSquare]               ; }
	const VecCell& GetVecCell(unsigned int mapSquare) const { return vecCells[ arrCells[mapSquare].GetVecIndx() ]; }
	      VecCell& GetVecCell(unsigned int mapSquare)       { return vecCells[ arrCells[mapSquare].GetVecIndx() ]; }

	bool CellInsertUnique(unsigned int sqr, CSolidObject* o);
	bool CellErase(unsigned int sqr, CSolidObject* o);

private:
	std::vector<ArrCell> arrCells;
	std::vector<VecCell> vecCells;
	std::vector<uint32_t> vecIndcs;
};

extern CGroundBlockingObjectMap groundBlockingObjectMap;

#endif
