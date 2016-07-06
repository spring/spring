/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_H
#define OBJECT_H

#include <map>
#include <set>
#include <array>
#include "ObjectDependenceTypes.h"
#include "System/Platform/Threading.h"
#include "System/creg/creg_cond.h"

class CObject
{
public:
	CR_DECLARE(CObject)

	CObject();
	virtual ~CObject();

	void Serialize(creg::ISerializer* ser);
	void PostLoad();

	/// Request to not inform this when obj dies
	virtual void DeleteDeathDependence(CObject* obj, DependenceType dep);
	/// Request to inform this when obj dies
	virtual void AddDeathDependence(CObject* obj, DependenceType dep);
	/// Called when an object died, that this is interested in
	virtual void DependentDied(CObject* obj);

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

private:
	// Note, this has nothing to do with the UnitID, FeatureID, ...
	// It's only purpose is to make the sorting in TSyncSafeSet syncsafe
	boost::int64_t sync_id;
	static Threading::AtomicCounterInt64 cur_sync_id;

	// makes std::set<T*> syncsafe (else iteration order depends on the pointer's address, which is not syncsafe)
	struct syncsafe_compare
	{
		bool operator() (const CObject* const& a1, const CObject* const& a2) const
		{
			// I don't think the default less-op is sync-safe
			//return a1 < a2;
			return a1->sync_id < a2->sync_id;
		}
	};

public:
	typedef std::set<CObject*, syncsafe_compare> TSyncSafeSet;
	typedef std::array<TSyncSafeSet*, DEPENDENCE_COUNT> TDependenceMap;
	bool detached;

protected:
	const TSyncSafeSet& GetListeners(const DependenceType dep) { return listeners[dep] ? *listeners[dep] : *(listeners[dep] = new TSyncSafeSet()); }
	const TDependenceMap& GetAllListeners() const { return listeners; }
	const TSyncSafeSet& GetListening(const DependenceType dep)  { return listening[dep] ? *listening[dep] : *(listening[dep] = new TSyncSafeSet()); }
	const TDependenceMap& GetAllListening() const { return listening; }

protected:
	TDependenceMap listeners;
	TDependenceMap listening;
};




#endif /* OBJECT_H */
