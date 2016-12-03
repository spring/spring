/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef BATTERY_H
#define BATTERY_H

#include "System/EventClient.h"
#include "System/Misc/SpringTime.h"


class CBattery: public CEventClient
{
public:
	CBattery();
	virtual ~CBattery();

public:
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "Update");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void Update();

	bool OnBattery() const { return onBattery; }

private:
	static bool HasBattery();

private:
	spring_time next_check;
	bool onBattery;
};

extern CBattery* battery;

#endif // BATTERY_H
