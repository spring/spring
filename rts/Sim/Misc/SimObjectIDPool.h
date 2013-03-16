/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SIMOBJECT_IDPOOL_H
#define SIMOBJECT_IDPOOL_H

// #include <map>
#include <vector>

#include "System/creg/STL_Map.h"

class SimObjectIDPool {
	CR_DECLARE_STRUCT(SimObjectIDPool)

public:
	unsigned int ExtractID();
	void ReserveID(unsigned int id);
	void FreeID(unsigned int id, bool delayed);
	void RecycleIDs();
	bool HasID(unsigned int id) const;
	bool IsEmpty() const { return (liveIndexIdentMap.empty()); }

	unsigned int GetSize() const { return (liveIndexIdentMap.size()); } // number of ID's still unused
	unsigned int MaxSize() const { return (liveIdentIndexMap.size()); } // number of ID's this pool owns

	void Expand(unsigned int baseID, unsigned int numIDs);

private:
	typedef std::pair<unsigned int, unsigned int> IDPair;
	typedef std::map<unsigned int, unsigned int> IDMap;

	IDMap liveIdentIndexMap;
	IDMap liveIndexIdentMap;
	IDMap tempIndexIdentMap;
};

#endif
