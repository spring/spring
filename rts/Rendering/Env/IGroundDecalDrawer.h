/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IGROUND_DECAL_DRAWER_H
#define IGROUND_DECAL_DRAWER_H

#include "System/float3.h"

//class CUnit;
class CSolidObject;
//struct CExplosionEvent;
class SolidObjectGroundDecal;
struct S3DModel;

struct GhostSolidObject {
	SolidObjectGroundDecal* decal; //FIXME defined in legacy decal handler with a lot legacy stuff
	S3DModel* model;

	float3 pos;
	float3 dir;

	int facing; //FIXME replaced with dir-vector just legacy decal drawer uses this
	int team;
};


class IGroundDecalDrawer
{
public:
	static bool GetDrawDecals() { return drawDecals; }
	static void SetDrawDecals(bool v) { if (decalLevel > 0) { drawDecals = v; } } //FIXME

	static IGroundDecalDrawer* GetInstance();
	static void FreeInstance();

public:
	virtual void Draw() = 0;
	virtual void Update() = 0;

	virtual void ForceRemoveSolidObject(CSolidObject* object) = 0;
	virtual void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb) = 0;

	//FIXME move to eventhandler?
	virtual void GhostDestroyed(GhostSolidObject* gb) = 0;
	virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb) = 0;

public:
	IGroundDecalDrawer();
	virtual ~IGroundDecalDrawer(){}

protected:
	static bool drawDecals;
	static unsigned int decalLevel;
};

#define groundDecals IGroundDecalDrawer::GetInstance()

#endif // IGROUND_DECAL_DRAWER_H
