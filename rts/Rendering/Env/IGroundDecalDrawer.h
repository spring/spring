/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IGROUND_DECAL_DRAWER_H
#define IGROUND_DECAL_DRAWER_H

class CSolidObject;
struct GhostSolidObject;


class IGroundDecalDrawer
{
public:
	static bool GetDrawDecals() { return (decalLevel > 0); }
	static void SetDrawDecals(bool v);

	static void Init();
	static void FreeInstance();
	static IGroundDecalDrawer* singleton;

public:
	virtual void Draw() = 0;

	virtual void AddSolidObject(CSolidObject* object) = 0;
	virtual void ForceRemoveSolidObject(CSolidObject* object) = 0;

	//FIXME move to eventhandler?
	virtual void GhostDestroyed(GhostSolidObject* gb) = 0;
	virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb) = 0;

public:
	virtual ~IGroundDecalDrawer() {}

protected:
	virtual void OnDecalLevelChanged() = 0;

protected:
	static int decalLevel;
};



class NullGroundDecalDrawer: public IGroundDecalDrawer
{
public:
	void Draw() override {}

	void AddSolidObject(CSolidObject* object) override {}
	void ForceRemoveSolidObject(CSolidObject* object) override {}

	void GhostDestroyed(GhostSolidObject* gb) override {}
	void GhostCreated(CSolidObject* object, GhostSolidObject* gb) override {}

	void OnDecalLevelChanged() override {}
};


#define groundDecals IGroundDecalDrawer::singleton

#endif // IGROUND_DECAL_DRAWER_H
