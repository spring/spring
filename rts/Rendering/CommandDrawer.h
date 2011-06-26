/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_DRAWER_H
#define COMMAND_DRAWER_H

struct Command;
class CCommandAI;
class CAirCAI;
class CBuilderCAI;
class CFactoryCAI;
class CMobileCAI;
class CTransportCAI;
class CUnit;

struct CommandDrawer {
public:
	static CommandDrawer* GetInstance();

	void Draw(const CCommandAI*) const;
	void DrawQuedBuildingSquares(const CBuilderCAI*) const;

private:
	void DrawCommands(const CCommandAI*) const;
	void DrawAirCAICommands(const CAirCAI*) const;
	void DrawBuilderCAICommands(const CBuilderCAI*) const;
	void DrawFactoryCAICommands(const CFactoryCAI*) const;
	void DrawMobileCAICommands(const CMobileCAI*) const;
	void DrawTransportCAICommands(const CTransportCAI*) const;

	void DrawWaitIcon(const Command&) const;
	void DrawDefaultCommand(const Command&, const CUnit*) const;
};

#define commandDrawer (CommandDrawer::GetInstance())

#endif
