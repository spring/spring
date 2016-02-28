/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DRAWER_H
#define COMMAND_DRAWER_H

#include <set>

struct Command;
class CCommandAI;
class CAirCAI;
class CBuilderCAI;
class CFactoryCAI;
class CMobileCAI;
class CUnit;

struct CommandDrawer {
public:
	static CommandDrawer* GetInstance();

	// clear the set after WorldDrawer and MiniMap have both used it
	void Update() { luaQueuedUnitSet.clear(); }

	void Draw(const CCommandAI*) const;
	void DrawLuaQueuedUnitSetCommands() const;
	void DrawQuedBuildingSquares(const CBuilderCAI*) const;

	void AddLuaQueuedUnit(const CUnit* unit);

private:
	void DrawCommands(const CCommandAI*) const;
	void DrawAirCAICommands(const CAirCAI*) const;
	void DrawBuilderCAICommands(const CBuilderCAI*) const;
	void DrawFactoryCAICommands(const CFactoryCAI*) const;
	void DrawMobileCAICommands(const CMobileCAI*) const;

	void DrawWaitIcon(const Command&) const;
	void DrawDefaultCommand(const Command&, const CUnit*) const;

private:
	std::set<int> luaQueuedUnitSet;
};

#define commandDrawer (CommandDrawer::GetInstance())

#endif
