/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTED_UNITS_H
#define SELECTED_UNITS_H

#include "Object.h"
#include <vector>
#include <string>
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitSet.h"

class CFeature;

class CSelectedUnits : public CObject
{
public:
	CSelectedUnits();
	virtual ~CSelectedUnits();

	void Init(unsigned numPlayers);
	void SelectGroup(int num);
	void AiOrder(int unitid, const Command& c, int playerID);
	int GetDefaultCmd(const CUnit* unit, const CFeature* feature);
	bool CommandsChanged();
	void NetOrder(Command& c, int playerId);
	void NetSelect(std::vector<int>& s, int playerId);
	void ClearNetSelect(int playerId);
	void DependentDied(CObject* o);
	void Draw();

	struct AvailableCommandsStruct {
		std::vector<CommandDescription> commands;
		int commandPage;
	};
	AvailableCommandsStruct GetAvailableCommands();
	void GiveCommand(Command c, bool fromUser = true);
	void AddUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void ClearSelected();

	void ToggleBuildIconsFirst();
	bool BuildIconsFirst() const { return buildIconsFirst; }

	void PossibleCommandChange(CUnit* sender);
	void DrawCommands();
	std::string GetTooltip();
	void SetCommandPage(int page);
	void SendSelection();
	void SendCommand(const Command& c);
	void SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands);


	CUnitSet selectedUnits;

	bool selectionChanged;
	bool possibleCommandsChanged;

	std::vector< std::vector<int> > netSelected;

	bool buildIconsFirst;
	int selectedGroup;
};

extern CSelectedUnits selectedUnits;

#endif /* SELECTED_UNITS_H */
