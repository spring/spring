#pragma once

#include <memory>
#include <unordered_map>

#include "ModelsMemStorageDefs.h"
#include "System/Matrix44f.h"
#include "System/MemPoolTypes.h"
#include "System/FreeListMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Objects/SolidObjectDef.h"

class MatricesMemStorage : public StablePosAllocator<CMatrix44f> {
public:
	MatricesMemStorage()
		: StablePosAllocator<CMatrix44f>(INIT_NUM_ELEMS)
	{}
private:
	static constexpr int INIT_NUM_ELEMS = 1 << 16u;
};

extern MatricesMemStorage matricesMemStorage;


////////////////////////////////////////////////////////////////////

class ScopedMatricesMemAlloc {
public:
	ScopedMatricesMemAlloc() : ScopedMatricesMemAlloc(0u) {};
	ScopedMatricesMemAlloc(std::size_t numElems_, bool withMutex = false) : numElems{numElems_} {
		firstElem = matricesMemStorage.Allocate(numElems, withMutex);
	}

	ScopedMatricesMemAlloc(const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc(ScopedMatricesMemAlloc&& smma) noexcept { *this = std::move(smma); }


	~ScopedMatricesMemAlloc() {
		if (firstElem == MatricesMemStorage::INVALID_INDEX)
			return;

		matricesMemStorage.Free(firstElem, numElems, &CMatrix44f::Zero());
	}

	bool Valid() const { return firstElem != MatricesMemStorage::INVALID_INDEX;	}
	std::size_t GetOffset(bool assertInvalid = true) const {
		if (assertInvalid)
			assert(Valid());

		return firstElem;
	}

	ScopedMatricesMemAlloc& operator= (const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc& operator= (ScopedMatricesMemAlloc&& smma) noexcept {
		//swap to prevent dealloc on dying object, yet enable destructor to do its thing on valid object
		std::swap(firstElem, smma.firstElem);
		std::swap(numElems , smma.numElems );

		return *this;
	}

	const CMatrix44f& operator[](std::size_t offset) const {
		assert(firstElem != MatricesMemStorage::INVALID_INDEX);
		assert(offset >= 0 && offset < numElems);
		return matricesMemStorage[firstElem + offset];
	}
	CMatrix44f& operator[](std::size_t offset) {
		assert(firstElem != MatricesMemStorage::INVALID_INDEX);
		assert(offset >= 0 && offset < numElems);
		return matricesMemStorage[firstElem + offset];
	}
public:
	static const ScopedMatricesMemAlloc& Dummy() {
		static ScopedMatricesMemAlloc dummy;
		return dummy;
	};
private:
	std::size_t firstElem = MatricesMemStorage::INVALID_INDEX;
	std::size_t numElems  = 0u;
};

////////////////////////////////////////////////////////////////////

class CWorldObject;
class ModelsUniformsStorage {
public:
	ModelsUniformsStorage();
public:
	size_t AddObjects(const CWorldObject* o);
	void   DelObjects(const CWorldObject* o);
	size_t GetObjOffset(const CWorldObject* o);
	ModelUniformData& GetObjUniformsArray(const CWorldObject* o);

	size_t AddObjects(const SolidObjectDef* o) { return INVALID_INDEX; }
	void   DelObjects(const SolidObjectDef* o) {}
	size_t GetObjOffset(const SolidObjectDef* o) { return INVALID_INDEX; }
	ModelUniformData& GetObjUniformsArray(const SolidObjectDef* o) { return dummy; }

	size_t AddObjects(const S3DModel* o) { return INVALID_INDEX; }
	void   DelObjects(const S3DModel* o) {}
	size_t GetObjOffset(const S3DModel* o) { return INVALID_INDEX; }
	ModelUniformData& GetObjUniformsArray(const S3DModel* o) { return dummy; }

	size_t Size() const { return storage.GetData().size(); }
	const std::vector<ModelUniformData>& GetData() const { return storage.GetData(); }
public:
	static constexpr size_t INVALID_INDEX = 0;
private:
	inline static ModelUniformData dummy = {0};

	std::unordered_map<CWorldObject*, size_t> objectsMap;
	spring::FreeListMap<ModelUniformData> storage;
};

extern ModelsUniformsStorage modelsUniformsStorage;