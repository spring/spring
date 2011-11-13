/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJECT_H
#define OBJECT_H

#include <list>
#include <map>
#include "System/creg/creg_cond.h"

template<typename T>
bool ListErase(std::list<T>& list, const T& what)
{
	typename std::list<T>::iterator it;
	for (it = list.begin(); it != list.end(); ++it) {
		if (*it == what) {
			list.erase(it);
			return true;
		}
	}
	return false;
}

template<typename T>
bool ListInsert(std::list<T>& list, const T& what)
{
	bool ret = true;
#ifndef NDEBUG
	typename std::list<T>::iterator it;
	for (it = list.begin(); it != list.end(); ++it) {
		if (*it == what) {
			ret = false;
			break;
		}
	}
#endif
	list.insert(list.end(), what);
	return ret;
}

class CObject
{
public:
	CR_DECLARE(CObject);

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
protected:
	bool detached;
	const std::list<CObject*>& GetListeners(const DependenceType dep) { return listeners[dep]; }
	const std::map<DependenceType, std::list<CObject*> >& GetAllListeners() { return listeners; }
	const std::list<CObject*>& GetListening(const DependenceType dep) { return listening[dep]; }
	const std::map<DependenceType, std::list<CObject*> >& GetAllListening() { return listening; }

private:
	std::map<DependenceType, std::list<CObject*> > listeners;
	std::map<DependenceType, std::list<CObject*> > listening;
};

#endif /* OBJECT_H */
