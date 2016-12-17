/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_IDPOOL_H
#define SIMOBJECT_IDPOOL_H

#include <unordered_map>
#include "System/creg/creg_cond.h"


class CSolidObject;
class SimObjectIDPool {
	CR_DECLARE_STRUCT(SimObjectIDPool)

public:
	void Expand(unsigned int baseID, unsigned int numIDs);

	void AssignID(CSolidObject* object);
	void FreeID(unsigned int id, bool delayed);

	bool HasID(unsigned int id) const;
	bool IsEmpty() const { return (liveIndexToIdentMap.empty()); }

	unsigned int GetSize() const { return (liveIndexToIdentMap.size()); } // number of ID's still unused
	unsigned int MaxSize() const { return (liveIdentToIndexMap.size()); } // number of ID's this pool owns

private:
	unsigned int ExtractID();
	void ReserveID(unsigned int id);
	void RecycleIDs();

private:
	typedef std::pair<unsigned int, unsigned int> IDPair;
	typedef std::unordered_map<unsigned int, unsigned int> IDMap;

	IDMap liveIdentToIndexMap;
	IDMap liveIndexToIdentMap;
	IDMap tempIndexToIdentMap;
};

#endif
