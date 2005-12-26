#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "Sim/MoveTypes/Mobility.h"
#include "GlobalStuff.h"

class CUnit;
struct DamageArray;


class CSolidObject : public CWorldObject {
public:
	CR_DECLARE(CSolidObject)

	enum PhysicalState {
		Ghost,
		OnGround,
		Floating,
		Hovering,
		Flying,
		Submarine
	};

	CSolidObject();
	virtual ~CSolidObject(void);

	virtual bool AddBuildPower(float amount, CUnit* builder);

	virtual void DoDamage(const DamageArray& damages,CUnit* attacker,const float3& impulse){};
	virtual void Kill(float3& impulse){};
	void Draw();

	void Block();
	void UnBlock();


	//Static properties.
	float mass;									//The physical mass of this object.
	bool blocking;								//If this object is blocking/collidable. (NOTE: Some objects could be flat => not collidable.)
	bool floatOnWater;							//If the object will float or not in water.	(TODO: "float dencity;" would give more dynamic.)
	bool isUnderWater;
	bool immobile;								//Immobile objects could not be moved. (Except perhaps in y, to make them stay on gound.)
	bool blockHeightChanges;			//map height cant change under this object
	int xsize;									//The x-size of this object, according to it's footprint.
	int ysize;									//The z-size of this object, according to it's footprint. NOTE: This one should have been called zsize!
	float height;								//The height of this object.
	short heading;								//Contains the same information as frontdir, but in a short signed integer.

	//Positional properties.
	PhysicalState physicalState;				//The current state of the object within the gameworld. I.e Flying or OnGround.
	float3 midPos;								//This is the calculated center position of the model (pos is usually at the very bottom of the model). Used as mass center.

	//Current dynamic properties.
	bool isMoving;								//= velocity.length() > 0.0
	
	//Accelerated dynamic properties.
//	float3 newVelocity;							//Resulting velocity at end of frame. Use this one to apply acceleration.
//	float3 newMomentum;							//Resulting momentum at end of frame. Use this one to apply acceleration.
	float3 residualImpulse;						//Used to sum up external impulses.

	//Mobility
	CMobility *mobility;						//Holding information about the mobility of this object. 0 means that object could not move by it's own.

	//Map
	int2 mapPos;								//Current position on GroundBlockingMap.
	unsigned char* yardMap;						//Current active yardmap of this object. 0 means no active yardmap => all blocked.
	bool isMarkedOnBlockingMap;					//Tells if this object are marked on the GroundBlockingMap.

	//Old stuff. Used for back-compability. NOTE: Don't use whose!
	float3 speed;
	int2 GetMapPos();
};

#endif
