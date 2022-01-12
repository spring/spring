/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WORLD_DRAWER_H
#define _WORLD_DRAWER_H

class CWorldDrawer
{
public:
	void InitPre() const;
	void InitPost() const;
	void Kill();

	void Update(bool newSimFrame);
	void Draw() const;

	void GenerateIBLTextures() const;
	void SetupScreenState() const;

private:
	void DrawOpaqueObjects() const;
	void DrawAlphaObjects() const;
	void DrawMiscObjects() const;
	void DrawBelowWaterOverlay() const;

private:
	unsigned int numUpdates = 0;
};

#endif // _WORLD_DRAWER_H
