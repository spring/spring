#ifndef SELECTEDUNITS_H
#define SELECTEDUNITS_H
// SelectedUnits.h: interface for the CSelectedUnits class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include "Object.h"
#include <vector>
#include <set>
#include "command.h"
class CUnit;
class CFeature;

using namespace std;

class CSelectedUnits : public CObject
{
public:
	void SelectGroup(int num);
	void AiOrder(int unitid,Command& c);
	void SendOrdersToUnits(const vector<int>& unitIDs,
	                       const vector<Command>& commands);
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

	set<CUnit*> selectedUnits;

	bool selectionChanged;
	bool possibleCommandsChanged;

	vector<int> netSelected[MAX_PLAYERS];

	bool buildIconsFirst;
	int selectedGroup;
	void PossibleCommandChange(CUnit* sender);
	void DrawCommands();
	std::string GetTooltip(void);
	void SetCommandPage(int page);
	void SendSelection(void);
	void SendCommand(Command& c);
	void SendCommandsToUnits(const vector<int>& unitIDs,
	                         const vector<Command>& commands);
};

extern CSelectedUnits selectedUnits;

#endif /* SELECTEDUNITS_H */
