/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DRAWER_H
#define COMMAND_DRAWER_H

#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "System/UnorderedSet.hpp"

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

	void Draw(const CCommandAI*, bool onMiniMap) const;
	void DrawLuaQueuedUnitSetCommands(bool onMiniMap) const;
	void DrawQueuedBuildingSquares(const CBuilderCAI*) const;

	void AddLuaQueuedUnit(const CUnit* unit);

	void SetBuildQueueSquareColor(const float* color) { buildQueueSquareColor = color; }

private:
	void DrawCommands(const CCommandAI*, GL::RenderDataBufferC* rdb) const;
	void DrawAirCAICommands(const CAirCAI*, GL::RenderDataBufferC* rdb) const;
	void DrawBuilderCAICommands(const CBuilderCAI*, GL::RenderDataBufferC* rdb) const;
	void DrawFactoryCAICommands(const CFactoryCAI*, GL::RenderDataBufferC* rdb) const;
	void DrawMobileCAICommands(const CMobileCAI*, GL::RenderDataBufferC* rdb) const;

	void DrawWaitIcon(const Command&) const;
	void DrawDefaultCommand(const Command&, const CUnit*, GL::RenderDataBufferC* rdb) const;

	void DrawQueuedBuildingSquaresAW(const CBuilderCAI* cai) const;
	void DrawQueuedBuildingSquaresUW(const CBuilderCAI* cai) const;

private:
	spring::unordered_set<int> luaQueuedUnitSet;

	// used by DrawQueuedBuildingSquares
	const float* buildQueueSquareColor = nullptr;
};

#define commandDrawer (CommandDrawer::GetInstance())

#endif
