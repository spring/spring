/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROFILE_DRAWER
#define PROFILE_DRAWER

#include "System/EventClient.h"

class ProfileDrawer : public CEventClient
{
public:
	// CEventClient interface
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

public:
	static void SetEnabled(bool enable);
	static bool IsEnabled() { return (instance != nullptr); }

	virtual void DrawScreen();
	virtual bool MousePress(int x, int y, int button);
	virtual bool IsAbove(int x, int y);
	virtual void DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end);

private:
	ProfileDrawer();

	static ProfileDrawer* instance;
};

#endif // PROFILE_DRAWER
