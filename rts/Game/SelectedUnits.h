/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTEDUNITS_H
#define SELECTEDUNITS_H

#include "Object.h"
#include <vector>
#include <string>
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitSet.h"

using std::vector;

class CFeature;

class CSelectedUnits : public CObject
{
public:
	void Init(unsigned numPlayers);
	void SelectGroup(int num);
	void AiOrder(int unitid, const Command& c, int playerID);
	int GetDefaultCmd(const CUnit* unit, const CFeature* feature);
	bool CommandsChanged();
	void NetOrder(Command& c,int player);
	void NetSelect(vector<int>& s,int player);
	void ClearNetSelect(int player);
	void DependentDied(CObject* o);
	void Draw();
	CSelectedUnits();
	virtual ~CSelectedUnits();

	struct AvailableCommandsStruct{
		vector<CommandDescription> commands;
		int commandPage;
	};
	AvailableCommandsStruct GetAvailableCommands();
	void GiveCommand(Command c,bool fromUser=true);
	void AddUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void ClearSelected();

	void ToggleBuildIconsFirst();
	bool BuildIconsFirst() const { return buildIconsFirst; }

	CUnitSet selectedUnits;

	bool selectionChanged;
	bool possibleCommandsChanged;

	std::vector< vector<int> > netSelected;

	bool buildIconsFirst;
	int selectedGroup;
	void PossibleCommandChange(CUnit* sender);
	void DrawCommands();
	std::string GetTooltip(void);
	void SetCommandPage(int page);
	void SendSelection(void);
	void SendCommand(Command& c);
	void SendCommandsToUnits(const vector<int>& unitIDs, const vector<Command>& commands);
};

extern CSelectedUnits selectedUnits;

#endif /* SELECTEDUNITS_H */
