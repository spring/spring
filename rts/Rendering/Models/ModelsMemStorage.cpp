#include "ModelsMemStorage.h"
#include "Sim/Objects/WorldObject.h"

MatricesMemStorage matricesMemStorage;
ModelsUniformsStorage modelsUniformsStorage;

ModelsUniformsStorage::ModelsUniformsStorage()
{
	storage[0] = dummy;
	objectsMap.emplace(nullptr, 0);
}

size_t ModelsUniformsStorage::AddObjects(const CWorldObject* o)
{
	const size_t idx = storage.Add(ModelUniformData());
	objectsMap[const_cast<CWorldObject*>(o)] = idx;
	return idx;
}

void ModelsUniformsStorage::DelObjects(const CWorldObject* o)
{
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));
	assert(it != objectsMap.end());

	storage.Del(it->second);
	objectsMap.erase(it);
}

size_t ModelsUniformsStorage::GetObjOffset(const CWorldObject* o)
{
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));
	if (it != objectsMap.end())
		return it->second;

	size_t idx = AddObjects(o);
	return idx;
}

ModelUniformData& ModelsUniformsStorage::GetObjUniformsArray(const CWorldObject* o)
{
	size_t offset = GetObjOffset(o);
	return storage[offset];
}
/*
void MatricesMemStorage::SetDirty(bool d)
{
	std::fill(dirtyMap.begin(), dirtyMap.end(), d);
}
*/
