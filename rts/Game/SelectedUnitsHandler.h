/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTED_UNITS_H
#define SELECTED_UNITS_H

#include <vector>
#include <string>

#include "Sim/Units/CommandAI/Command.h"
#include "System/float4.h"
#include "System/Object.h"
#include "System/UnorderedSet.hpp"

class CUnit;
class CFeature;
struct SCommandDescription;

class CSelectedUnitsHandler : public CObject
{
public:
	void Init(unsigned numPlayers);
	void SelectGroup(int num);
	void AINetOrder(int unitID, int aiTeamID, int playerID, const Command& c);
	int GetDefaultCmd(const CUnit* unit, const CFeature* feature);

	void NetOrder(Command& c, int playerId);
	void NetSelect(std::vector<int>& s, int playerId);
	void ClearNetSelect(int playerId);
	void DependentDied(CObject* o) override;
	void Draw();

	struct AvailableCommandsStruct {
		std::vector<SCommandDescription> commands;
		int commandPage;
	};
	AvailableCommandsStruct GetAvailableCommands();
	void GiveCommand(const Command& c, bool fromUser = true);
	void AddUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void ClearSelected();

	/// used by MouseHandler.cpp & MiniMap.cpp
	void HandleUnitBoxSelection(const float4& planeRight, const float4& planeLeft, const float4& planeTop, const float4& planeBottom);
	void HandleSingleUnitClickSelection(CUnit* unit, bool doInViewTest, bool selectType);

	void ToggleBuildIconsFirst();
	bool BuildIconsFirst() const { return buildIconsFirst; }

	void PossibleCommandChange(CUnit* sender);
	void DrawCommands(bool onMiniMap);
	std::string GetTooltip();
	void SetCommandPage(int page);
	void SendCommand(const Command& c);
	void SendCommandsToUnits(const std::vector<int>& unitIDs, const std::vector<Command>& commands, bool pairwise = false);

	bool CommandsChanged() const { return possibleCommandsChanged; }
	bool IsUnitSelected(const CUnit* unit) const;
	bool IsUnitSelected(const int unitID) const;
	bool AutoAddBuiltUnitsToFactoryGroup() const { return autoAddBuiltUnitsToFactoryGroup; }
	bool AutoAddBuiltUnitsToSelectedGroup() const { return autoAddBuiltUnitsToSelectedGroup; }
	bool IsGroupSelected(int groupID) const { return (selectedGroup == groupID); }
	int GetSelectedGroup() const { return selectedGroup; }

	void SelectUnits(const std::string& line);
	void SelectCycle(const std::string& command);

private:
	int selectedGroup = -1;
	int soundMultiselID = 0;

	bool autoAddBuiltUnitsToFactoryGroup = false;
	bool autoAddBuiltUnitsToSelectedGroup = false;
	bool buildIconsFirst = false;

public:
	bool selectionChanged = false;
	bool possibleCommandsChanged = true;

	spring::unordered_set<int> selectedUnits;
	std::vector< std::vector<int> > netSelected;

private:
	// buffer for SendCommand unordered_set->vector conversion
	std::vector<int16_t> selectedUnitIDs;
};

extern CSelectedUnitsHandler selectedUnitsHandler;

#endif /* SELECTED_UNITS_H */

