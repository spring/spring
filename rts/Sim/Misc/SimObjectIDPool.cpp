/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SimObjectIDPool.h"
#include "GlobalSynced.h"
#include "Sim/Objects/SolidObject.h"
#include "System/creg/STL_Map.h"


CR_BIND(SimObjectIDPool, )
CR_REG_METADATA(SimObjectIDPool, (
	CR_MEMBER(liveIdentToIndexMap),
	CR_MEMBER(liveIndexToIdentMap),
	CR_MEMBER(tempIndexToIdentMap)
))

void SimObjectIDPool::Expand(unsigned int baseID, unsigned int numIDs) {
	// allocate new batch of (randomly shuffled) id's
	std::vector<int> newIDs(numIDs);

	for (unsigned int offsetID = 0; offsetID < numIDs; offsetID++) {
		newIDs[offsetID] = baseID + offsetID;
	}

	// randomize so that Lua widgets can not easily determine counts
	std::random_shuffle(newIDs.begin(), newIDs.end(), gsRNG);
	std::random_shuffle(newIDs.begin(), newIDs.end(), gsRNG);

	// NOTE:
	//   any randomization would be undone by a sorted std::container
	//   instead create a bi-directional mapping from indices to ID's
	//   (where the ID's are a random permutation of the index range)
	//   such that ID's can be assigned and returned to the pool with
	//   their original index
	//
	//     indexToIdentMap = {<0, 13>, < 1, 27>, < 2, 54>, < 3, 1>, ...}
	//     identToIndexMap = {<1,  3>, <13,  0>, <27,  1>, <54, 2>, ...}
	//
	//   (the ID --> index map is never changed at runtime!)
	for (unsigned int offsetID = 0; offsetID < numIDs; offsetID++) {
		liveIndexToIdentMap.insert(IDPair(baseID + offsetID, newIDs[offsetID]));
		liveIdentToIndexMap.insert(IDPair(newIDs[offsetID], baseID + offsetID));
	}
}



void SimObjectIDPool::AssignID(CSolidObject* object) {
	if (object->id < 0) {
		object->id = ExtractID();
	} else {
		ReserveID(object->id);
	}
}

unsigned int SimObjectIDPool::ExtractID() {
	// extract a random ID from the pool
	//
	// should be unreachable, UnitHandler
	// and FeatureHandler have safeguards
	assert(!IsEmpty());

	const IDMap::iterator it = liveIndexToIdentMap.begin();
	const unsigned int id = it->second;

	liveIndexToIdentMap.erase(it);

	if (IsEmpty()) {
		RecycleIDs();
	}

	return id;
}

void SimObjectIDPool::ReserveID(unsigned int id) {
	// reserve a chosen ID from the pool
	assert(HasID(id));
	assert(!IsEmpty());

	const IDMap::iterator it = liveIdentToIndexMap.find(id);
	const unsigned int idx = it->second;

	liveIndexToIdentMap.erase(idx);

	if (IsEmpty()) {
		RecycleIDs();
	}
}

void SimObjectIDPool::FreeID(unsigned int id, bool delayed) {
	// put an ID back into the pool either immediately
	// or after all remaining free ID's run out (which
	// is better iff the object count never gets close
	// to the maximum)
	assert(!HasID(id));

	if (delayed) {
		tempIndexToIdentMap.insert(IDPair(liveIdentToIndexMap[id], id));
	} else {
		liveIndexToIdentMap.insert(IDPair(liveIdentToIndexMap[id], id));
	}
}

void SimObjectIDPool::RecycleIDs() {
	// throw each ID recycled up until now back into the pool
	liveIndexToIdentMap.insert(tempIndexToIdentMap.begin(), tempIndexToIdentMap.end());
	tempIndexToIdentMap.clear();
}

bool SimObjectIDPool::HasID(unsigned int id) const {
	assert(liveIdentToIndexMap.find(id) != liveIdentToIndexMap.end());

	// check if given ID is available in this pool
	const IDMap::const_iterator it = liveIdentToIndexMap.find(id);
	const unsigned int idx = it->second;

	return (liveIndexToIdentMap.find(idx) != liveIndexToIdentMap.end());
}

