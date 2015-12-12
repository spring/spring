/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WORLD_DRAWER_H
#define _WORLD_DRAWER_H

class CWorldDrawer
{
public:
	CWorldDrawer();
	~CWorldDrawer();

	void LoadPre() const;
	void LoadPost() const;

	void Update(bool newSimFrame);
	void Draw() const;

	void GenerateIBLTextures() const;
	void ResetMVPMatrices() const;

private:
	void DrawOpaqueObjects() const;
	void DrawAlphaObjects() const;
	void DrawMiscObjects() const;
	void DrawBelowWaterOverlay() const;

private:
	unsigned int numUpdates;
};

#endif // _WORLD_DRAWER_H
