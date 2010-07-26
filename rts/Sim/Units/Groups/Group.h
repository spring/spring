/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUP_H
#define GROUP_H

#include <vector>

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitSet.h"

class CUnit;
class CFeature;
class CGroupHandler;

class CGroup
{
	CR_DECLARE(CGroup);

public:
	CGroup(int id, CGroupHandler* groupHandler);
	virtual ~CGroup();
	void Serialize(creg::ISerializer *s);
	void PostLoad();

	void Update();
	void DrawCommands();

	void CommandFinished(int unitId, int commandTopicId);
	void RemoveUnit(CUnit* unit); ///< call unit.SetGroup(NULL) instead of calling this directly
	bool AddUnit(CUnit* unit);    ///< dont call this directly call unit.SetGroup and let that call this
	const std::vector<CommandDescription>& GetPossibleCommands();
	int GetDefaultCmd(const CUnit* unit, const CFeature* feature) const;
	void GiveCommand(Command c);
	void ClearUnits(void);

	int id;

	CUnitSet units;

	std::vector<CommandDescription> myCommands;

	int lastCommandPage;

	CGroupHandler* handler;
};

#endif // GROUP_H
