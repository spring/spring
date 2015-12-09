/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _WORLD_DRAWER_H
#define _WORLD_DRAWER_H

class CWorldDrawer
{
public:
	CWorldDrawer();
	~CWorldDrawer();

	void Update();
	void Draw();
	void GenerateIBL();

private:
	unsigned int numUpdates;
};

#endif // _WORLD_DRAWER_H
