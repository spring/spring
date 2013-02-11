/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include "System/EventHandler.h"
#include <vector>


class CBenchmark : public CEventClient
{
public:
	static bool enabled;
	static int startFrame;
	static int endFrame;

public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "GameFrame") || (eventName == "DrawWorld");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void GameFrame(int gameFrame);
	void DrawWorld();

public:
	CBenchmark();
	~CBenchmark();
};

#endif // _ROAM_MESH_DRAWER_H_
