/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_H
#define OBJECT_H

#include <map>
#include <set>
#include "System/Platform/Threading.h"
#include "System/creg/creg_cond.h"


class CObject
{
public:
	CR_DECLARE(CObject);

	// The reason to have different dependence types is that an object may simultaneously have more than one kind of dependence to another object.
	// Without dependence types, the dependencies would therefore need to be duplicated (stored as lists or vectors instead of maps or sets).
	// This would also make deleting the death dependence somewhat risky since there in this case must be an exact matching number of
	// AddDeathDependence / DeleteDeathDependence calls in order not to risk a crash. With dependence types this can never happen, i.e.
	// DeleteDeathDependence(object, DEPENDENCE_ATTACKER) can be called a hundred times without any risk of losing some other type of dependence.
	// Dependence types also makes it easy to detect deletion of non-existent dependence types, and output a warning for debugging purposes.
	enum DependenceType {
		DEPENDENCE_ATTACKER,
		DEPENDENCE_BUILD,
		DEPENDENCE_BUILDER,
		DEPENDENCE_CAPTURE,
		DEPENDENCE_COBTHREAD,
		DEPENDENCE_COMMANDQUE,
		DEPENDENCE_DECOYTARGET,
		DEPENDENCE_INCOMING,
		DEPENDENCE_INTERCEPT,
		DEPENDENCE_INTERCEPTABLE,
		DEPENDENCE_INTERCEPTTARGET,
		DEPENDENCE_LANDINGPAD,
		DEPENDENCE_LASTCOLWARN,
		DEPENDENCE_LIGHT,
		DEPENDENCE_ORDERTARGET,
		DEPENDENCE_RECLAIM,
		DEPENDENCE_REPULSE,
		DEPENDENCE_REPULSED,
		DEPENDENCE_RESURRECT,
		DEPENDENCE_SELECTED,
		DEPENDENCE_SOLIDONTOP,
		DEPENDENCE_TARGET,
		DEPENDENCE_TARGETUNIT,
		DEPENDENCE_TERRAFORM,
		DEPENDENCE_TRANSPORTEE,
		DEPENDENCE_TRANSPORTER,
		DEPENDENCE_WAITCMD,
		DEPENDENCE_WEAPON,
		DEPENDENCE_WEAPONTARGET
	};

	CObject();
	virtual ~CObject();
	void Serialize(creg::ISerializer* ser);
	void PostLoad();
	virtual void Detach();

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
				FACTORYCAI,TRANSPORTCAI,MOBILECAI,
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
	typedef std::map<DependenceType, TSyncSafeSet> TDependenceMap;

protected:
	const TSyncSafeSet& GetListeners(const DependenceType dep) { return listeners[dep]; }
	const TDependenceMap& GetAllListeners() const { return listeners; }
	const TSyncSafeSet& GetListening(const DependenceType dep)  { return listening[dep]; }
	const TDependenceMap& GetAllListening() const { return listening; }

protected:
	bool detached;
	TDependenceMap listeners;
	TDependenceMap listening;
};




#endif /* OBJECT_H */
