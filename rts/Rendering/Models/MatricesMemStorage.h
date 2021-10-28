#pragma once

#include <memory>

#include "System/Matrix44f.h"
#include "System/MemPoolTypes.h"

class MatricesMemStorage {
/*
public:
	static MatricesMemStorage& GetInstance() {
		static MatricesMemStorage instance;
		return instance;
	};
*/
public:
	MatricesMemStorage();
public:
	void Reset() {
		//DeallocateDummy();
		spa->Reset();
		//AllocateDummy();
	};
	std::size_t Allocate(std::size_t numElems, bool withMutex = false) { return spa->Allocate(numElems, withMutex); };
	void Free(std::size_t firstElem, size_t numElems) { spa->Free(firstElem, numElems); };
    const std::size_t GetSize() const { return spa->GetSize(); }
    const std::vector<CMatrix44f>& GetData() const { return spa->GetData(); }
	      std::vector<CMatrix44f>& GetData()       { return spa->GetData(); }

	const CMatrix44f& operator[](std::size_t idx) const { return spa->operator[](idx); }
	      CMatrix44f& operator[](std::size_t idx)       { return spa->operator[](idx); }
public:
	static constexpr int INIT_NUM_ELEMS = 1 << 16u;
	static constexpr std::size_t INVALID_INDEX = ~0u;
private:
	void AllocateDummy();
	void DeallocateDummy();
private:
	std::unique_ptr<StablePosAllocator<CMatrix44f>> spa;

	static constexpr size_t DUMMY_ELEMS = 1u;
};

//#define matricesMemStorage MatricesMemStorage::GetInstance()  //benchmark shows it's costly
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

		matricesMemStorage.Free(firstElem, numElems);
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
