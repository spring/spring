/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IGROUND_DECAL_DRAWER_H
#define IGROUND_DECAL_DRAWER_H

class CSolidObject;
struct GhostSolidObject;


class IGroundDecalDrawer
{
public:
	static bool GetDrawDecals() { return drawDecals; }
	static void SetDrawDecals(bool v) { if (decalLevel > 0) { drawDecals = v; } } //FIXME

	static IGroundDecalDrawer* GetInstance();
	static void FreeInstance();

public:
	virtual void Draw() = 0;

	virtual void ForceRemoveSolidObject(CSolidObject* object) = 0;

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
