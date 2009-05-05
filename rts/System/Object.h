#ifndef OBJECT_H
#define OBJECT_H
// Object.h: interface for the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include <list>
#include "creg/creg_cond.h"

template<typename T>
void ListErase(std::list<T>& list, const T& what)
{
	typename std::list<T>::iterator it;
	for (it = list.begin(); it != list.end(); ++it) {
		if (*it == what) {
			list.erase(it);
			break;
		}
	}
}

class CObject
{
public:
	CR_DECLARE(CObject);

	CObject();
	virtual ~CObject();
	void Serialize(creg::ISerializer *s);
	void PostLoad();
	
	void DeleteDeathDependence(CObject* o);
	void AddDeathDependence(CObject* o);
	virtual void DependentDied(CObject* o);
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
	std::list<CObject*> listeners, listening;
};

#endif /* OBJECT_H */
