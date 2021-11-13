#include "ModelsMemStorage.h"
#include "Sim/Objects/WorldObject.h"

MatricesMemStorage matricesMemStorage;
ModelsUniformsStorage modelsUniformsStorage;

ModelsUniformsStorage::ModelsUniformsStorage()
{
	storage.emplace_back(dummy);
	objectsMap.emplace(nullptr, 0);
}

size_t ModelsUniformsStorage::AddObjects(const CWorldObject* o)
{
	if (!emptyIndices.empty()) {
		size_t idx = emptyIndices.back();
		emptyIndices.pop_back();

		//storage[idx] = ModelUniformData(); done in DelObjects
		objectsMap[const_cast<CWorldObject*>(o)] = idx;
		return idx;
	}

	storage.emplace_back(ModelUniformData());
	size_t idx = storage.size() - 1;

	objectsMap[const_cast<CWorldObject*>(o)] = idx;
	return idx;
}

void ModelsUniformsStorage::DelObjects(const CWorldObject* o)
{
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));
	assert(it != objectsMap.end());

	storage[it->second] = ModelUniformData(); //nullify
	emptyIndices.emplace_back(it->second);
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
