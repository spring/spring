/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROFILE_DRAWER
#define PROFILE_DRAWER

#include "System/EventClient.h"

class ProfileDrawer : public CEventClient
{
public:
	// CEventClient interface
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }

public:
	static void SetEnabled(bool enable);
	static bool IsEnabled() { return (instance != nullptr); }

	virtual void DrawScreen() override;
	virtual bool MousePress(int x, int y, int button) override;
	virtual bool IsAbove(int x, int y) override;
	virtual void DbgTimingInfo(DbgTimingInfoType type, const spring_time start, const spring_time end) override;

private:
	ProfileDrawer();

	static ProfileDrawer* instance;
};

#endif // PROFILE_DRAWER
