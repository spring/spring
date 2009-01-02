#ifndef SELECTEDUNITS_H
#define SELECTEDUNITS_H
// SelectedUnits.h: interface for the CSelectedUnits class.
//
//////////////////////////////////////////////////////////////////////

#include "Object.h"
#include <vector>
#include <set>
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitSet.h"
class CFeature;

class CSelectedUnits : public CObject
{
public:
	void Init();
	void SelectGroup(int num);
	void AiOrder(int unitid, const Command& c, int playerID);
	int GetDefaultCmd(CUnit* unit,CFeature* feature);
	bool CommandsChanged();
	void NetOrder(Command& c,int player);
	void NetSelect(vector<int>& s,int player);
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
