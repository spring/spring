/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROFILE_DRAWER
#define PROFILE_DRAWER

#include "InputReceiver.h"

class ProfileDrawer : public CInputReceiver
{
public:
	static void SetEnabled(bool enable);
	static bool IsEnabled();

	virtual void Draw();
	virtual bool MousePress(int x, int y, int button);
	virtual bool IsAbove(int x, int y);	

private:
	ProfileDrawer();
	~ProfileDrawer();

	static ProfileDrawer* instance;
};

#endif // PROFILE_DRAWER
