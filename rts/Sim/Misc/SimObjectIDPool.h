/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_IDPOOL_H
#define SIMOBJECT_IDPOOL_H

#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"

class CSolidObject;
class SimObjectIDPool {
	CR_DECLARE_STRUCT(SimObjectIDPool)

public:
	SimObjectIDPool() {} // FIXME: creg, needs PostLoad
	SimObjectIDPool(unsigned int maxObjects) {
		// pools are reused as part of object handlers, internal table sizes must be
		// constant at runtime to prevent desyncs between fresh and reloaded clients
		// (both must execute Expand since it touches the RNG)
		poolIDs.reserve(maxObjects);
		freeIDs.reserve(maxObjects);
		tempIDs.reserve(maxObjects);
	}

	void Expand(unsigned int baseID, unsigned int numIDs);
	void Clear() {
		freeIDs.clear();
		poolIDs.clear();
		tempIDs.clear();
	}

	void AssignID(CSolidObject* object);
	void FreeID(unsigned int uid, bool delayed);

	bool RecycleID(unsigned int uid);
	bool HasID(unsigned int uid) const;
	bool IsEmpty() const { return (freeIDs.empty()); }

	unsigned int GetSize() const { return (freeIDs.size()); } // number of ID's still unused
	unsigned int MaxSize() const { return (poolIDs.size()); } // number of ID's this pool owns

private:
	unsigned int ExtractID();

	void ReserveID(unsigned int uid);
	void RecycleIDs();

private:
	spring::unordered_map<unsigned int, unsigned int> poolIDs; // uid to idx
	spring::unordered_map<unsigned int, unsigned int> freeIDs; // idx to uid
	spring::unordered_map<unsigned int, unsigned int> tempIDs; // idx to uid
};

#endif

