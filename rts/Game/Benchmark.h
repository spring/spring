/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include <map>

#include "System/EventHandler.h"


class CBenchmark : public CEventClient
{
public:
	static bool enabled;
	static int startFrame;
	static int endFrame;

public:
	CBenchmark();
	~CBenchmark();

	void ResetState() {
		realFPS.clear();
		drawFPS.clear();
		simFPS.clear();
		units.clear();
		features.clear();
		gameSpeed.clear();
		luaUsage.clear();
	}

	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "GameFrame") || (eventName == "DrawWorld");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void GameFrame(int gameFrame);
	void DrawWorld();

private:
	std::map<float, float> realFPS;
	std::map<float, float> drawFPS;
	std::map<int, float>   simFPS;
	std::map<int, size_t>  units;
	std::map<int, size_t>  features;
	std::map<int, float>   gameSpeed;
	std::map<int, float>   luaUsage;
};

#endif // _ROAM_MESH_DRAWER_H_
