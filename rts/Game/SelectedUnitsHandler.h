/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTED_UNITS_H
#define SELECTED_UNITS_H

#include "Sim/Units/UnitSet.h"
#include "Sim/Units/CommandAI/Command.h"
#include "System/float4.h"
#include "System/Object.h"
#include <vector>
#include <string>

class CFeature;
struct SCommandDescription;

class CSelectedUnitsHandler : public CObject
{
public:
	CSelectedUnitsHandler();

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
		std::vector<SCommandDescription> commands;
		int commandPage;
	};
	AvailableCommandsStruct GetAvailableCommands();
	void GiveCommand(Command c, bool fromUser = true);
	void AddUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void ClearSelected();

	/// used by MouseHandler.cpp & MiniMap.cpp
	void HandleUnitBoxSelection(const float4& planeRight, const float4& planeLeft, const float4& planeTop, const float4& planeBottom);
	void HandleSingleUnitClickSelection(CUnit* unit, bool doInViewTest, bool selectType);

	void ToggleBuildIconsFirst();
	bool BuildIconsFirst() const { return buildIconsFirst; }

	void PossibleCommandChange(CUnit* sender);
	void DrawCommands();
	std::string GetTooltip();
	void SetCommandPage(int page);
	void SendCommand(const Command& c);
	void SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands, bool pairwise = false);

	bool IsUnitSelected(const CUnit* unit) const;
	bool IsUnitSelected(const int unitID) const;
	bool AutoAddBuiltUnitsToFactoryGroup() const { return autoAddBuiltUnitsToFactoryGroup; }
	bool AutoAddBuiltUnitsToSelectedGroup() const { return autoAddBuiltUnitsToSelectedGroup; }
	bool IsGroupSelected(int groupID) const { return (selectedGroup == groupID); }
	int GetSelectedGroup() const { return selectedGroup; }

	void SelectUnits(const std::string& line);
	void SelectCycle(const std::string& command);

public:
	CUnitSet selectedUnits;

	bool selectionChanged;
	bool possibleCommandsChanged;

	std::vector< std::vector<int> > netSelected;

private:
	int selectedGroup;
	int soundMultiselID;

	bool autoAddBuiltUnitsToFactoryGroup;
	bool autoAddBuiltUnitsToSelectedGroup;
	bool buildIconsFirst;
};

extern CSelectedUnitsHandler selectedUnitsHandler;

#endif /* SELECTED_UNITS_H */

