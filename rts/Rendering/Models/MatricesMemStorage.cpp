#include "MatricesMemStorage.h"

MatricesMemStorage matricesMemStorage;

MatricesMemStorage::MatricesMemStorage()
{
	spa = std::make_unique<StablePosAllocator<CMatrix44f>>(INIT_NUM_ELEMS);
	//AllocateDummy();
}

void MatricesMemStorage::AllocateDummy()
{
	size_t pos = spa->Allocate(DUMMY_ELEMS, false); //allocate dummy matrix at 0th position
	assert(pos == 0u);

	std::memset(&spa->GetData()[pos], 0u, DUMMY_ELEMS * sizeof(CMatrix44f));
}

void MatricesMemStorage::DeallocateDummy()
{
	spa->Free(0u, DUMMY_ELEMS); //deallocate dummy matrix at 0th position
}