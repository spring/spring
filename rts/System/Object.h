/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_H
#define OBJECT_H

#include <atomic>
#include <functional>
#include <vector>

#include "ObjectDependenceTypes.h"
#include "System/creg/creg_cond.h"
#include "System/UnorderedMap.hpp"

class CObject
{
public:
	CR_DECLARE(CObject)

	CObject();
	virtual ~CObject();

	/// Request to not inform this when obj dies
	virtual void DeleteDeathDependence(CObject* obj, DependenceType dep);
	/// Request to inform this when obj dies
	virtual void AddDeathDependence(CObject* obj, DependenceType dep);
	/// Called when an object died, that this is interested in
	/// Any derived implementations should *NOT* modify listening
	/// or listeners because we iterate over both within our dtor
	/// when calling this
	virtual void DependentDied(CObject* obj) {}

/*
	// Possible future replacement for dynamic_cast (10x faster)
	// Identifier bits for classes that have subclasses
	enum {WORLDOBJECT_BIT=8,SOLIDOBJECT_BIT,UNIT_BIT,BUILDING_BIT,MOVETYPE_BIT,AAIRMOVETYPE_BIT,
		COMMANDAI_BIT,EXPGENSPAWNABLE_BIT,PROJECTILE_BIT,SMOKEPROJECTILE_BIT,WEAPON_BIT};
	// Class hierarchy for the relevant classes
	enum {
		OBJECT=0,
			WORLDOBJECT=(1<<WORLDOBJECT_BIT),
				SOLIDOBJECT=(1<<SOLIDOBJECT_BIT)|WORLDOBJECT,
					FEATURE,
					UNIT=(1<<UNIT_BIT)|SOLIDOBJECT,
						BUILDER,TRANSPORTUNIT,
						BUILDING=(1<<BUILDING_BIT)|UNIT,
							FACTORY,EXTRACTORBUILDING,
			MOVETYPE=(1<<MOVETYPE_BIT),
				GROUNDMOVETYPE,
				AAIRMOVETYPE=(1<<AAIRMOVETYPE_BIT)|MOVETYPE,
					AIRMOVETYPE,
					TAAIRMOVETYPE,
			COMMANDAI=(1<<COMMANDAI_BIT),
				FACTORYCAI,MOBILECAI,
			EXPGENSPAWNABLE=(1<<EXPGENSPAWNABLE_BIT),
				PROJECTILE=(1<<PROJECTILE_BIT)|EXPGENSPAWNABLE,
					SHIELDPARTPROJECTILE,
					SMOKEPROJECTILE=(1<<SMOKEPROJECTILE_BIT)|PROJECTILE,
						GEOTHERMSMOKEPROJECTILE,
			WEAPON=(1<<WEAPON_BIT),
				DGUNWEAPON,BEAMLASER
	};
	// Must also set objType in the contstructors of all classes that need to use this feature
	unsigned objType;
#define INSTANCE_OF_SUBCLASS_OF(type,obj) ((obj->objType & kind) == kind) // exact class or any subclass of it
#define INSTANCE_OF(type,obj) (obj->objType == type) // exact class only, saves one instruction yay :)
*/

	std::int64_t GetSyncID() const { return sync_id; }

private:
	// Note, this has nothing to do with the UnitID, FeatureID, ...
	// Its only purpose is to make the sorting in TSyncSafeSet syncsafe
	std::int64_t sync_id;
	static std::atomic<std::int64_t> cur_sync_id;

public:
	typedef std::vector<CObject*> TSyncSafeSet;
	typedef std::vector<TSyncSafeSet> TDependenceMap;
	typedef std::function<bool(const CObject*, int*)> TObjFilterPred;

	bool detached;

protected:
	const TSyncSafeSet& GetListeners(const DependenceType dep) {
		const auto it = listenersDepTbl.find(dep);

		if (it == listenersDepTbl.end()) {
			listeners.emplace_back();
			listenersDepTbl[dep] = listeners.size() - 1;
			return (listeners.back());
		}

		return (listeners[it->second]);
	}
	const TSyncSafeSet& GetListening(const DependenceType dep) {
		const auto it = listeningDepTbl.find(dep);

		if (it == listeningDepTbl.end()) {
			listening.emplace_back();
			listeningDepTbl[dep] = listening.size() - 1;
			return (listening.back());
		}

		return (listening[it->second]);
	}

	const TDependenceMap& GetAllListeners() const { return listeners; }
	const TDependenceMap& GetAllListening() const { return listening; }

	template<size_t N> static void FilterDepObjects(
		const TDependenceMap& depObjects,
		const TObjFilterPred& filterPred,
		std::array<int, N>& objectIDs
	) {
		objectIDs[0] = 0;

		for (const auto& objs: depObjects) {
			for (const CObject* obj: objs) {
				objectIDs[0] += ((objectIDs[0] < (N - 1)) && filterPred(obj, &objectIDs[objectIDs[0] + 1]));
			}
		}
	}

	template<size_t N> void FilterListeners(const TObjFilterPred& fp, std::array<int, N>& ids) const { FilterDepObjects(listeners, fp, ids); }
	template<size_t N> void FilterListening(const TObjFilterPred& fp, std::array<int, N>& ids) const { FilterDepObjects(listening, fp, ids); }

protected:
	spring::unordered_map<int, size_t> listenersDepTbl; // maps dependence-type to index into listeners
	spring::unordered_map<int, size_t> listeningDepTbl; // maps dependence-type to index into listening

	TDependenceMap listeners;
	TDependenceMap listening;
};

#endif /* OBJECT_H */

