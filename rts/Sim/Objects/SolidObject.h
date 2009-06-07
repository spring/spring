#ifndef SOLID_OBJECT_H
#define SOLID_OBJECT_H

#include "WorldObject.h"
#include "Sim/MoveTypes/MoveInfo.h"
#include "Vec2.h"
#include "Sync/SyncedFloat3.h"

class CUnit;
struct DamageArray;


class CSolidObject: public CWorldObject {
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
	virtual ~CSolidObject();

	virtual bool AddBuildPower(float amount, CUnit* builder);

	virtual void DoDamage(const DamageArray& damages, CUnit* attacker, const float3& impulse) {};
	virtual void Kill(float3& impulse) {};
	virtual int GetBlockingMapID() const { return -1; }

	void Block();
	void UnBlock();

	// Static properties.
	float mass;									// the physical mass of this object
	bool blocking;								// if this object can be collided with at all (NOTE: Some objects could be flat => not collidable.)
	bool floatOnWater;							// if the object will float on water (TODO: "float density;" would be more dynamic.)
	bool isUnderWater;
	bool immobile;								// Immobile objects can not be moved. (Except perhaps along y-axis, to make them stay on ground.)
	bool blockHeightChanges;					// map height cannot change under this object
	int xsize;									// The x-size of this object, according to its footprint.
	int zsize;									// The z-size of this object, according to its footprint.
	float height;								// The height of this object.
	SyncedSshort heading;								// Contains the same information as frontdir, but in a short signed integer.
	
	// Positional properties.
	PhysicalState physicalState;				// The current state of the object within the gameworld. I.e Flying or OnGround.
	SyncedFloat3 midPos;						// This is the calculated center position of the model (pos is usually at the very bottom of the model). Used as mass center.

	// Current dynamic properties.
	bool isMoving;								// = velocity.length() > 0.0

	// Accelerated dynamic properties.
	float3 residualImpulse;						// Used to sum up external impulses.

	// Mobility
	MoveData* mobility;							// holds information about the mobility and movedata of this object (0 means object can not move on its own)

	// Map
	int2 mapPos;								// Current position on GroundBlockingMap.
	unsigned char* yardMap;						// Current active yardmap of this object. 0 means no active yardmap => all blocked.
	int buildFacing;							// Orientation of footprint, 4 different states
	bool isMarkedOnBlockingMap;					// true if this object is currently marked on the GroundBlockingMap

	// Old stuff. Used for back-compability. NOTE: Don't use these!
	float3 speed;
	int2 GetMapPos();
	int2 GetMapPos(const float3 &position);
};

#endif
