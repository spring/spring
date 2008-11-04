#ifndef SELECT_MENU
#define SELECT_MENU

#include "GameController.h"

class LocalSetup;
class CglList;

/**
@brief User prompt for options when no script is given

When no setupscript is given, this will show a menu to select server address (when running in client ("-c")mode).
If in host mode, it will show lists for Map, Mod and Script.
When everything is selected, it will generate a gamesetup-script and start CPreGame
*/
class SelectMenu : public CGameController
{
public:
	SelectMenu(bool server);
	
	bool Draw();
	int KeyPressed(unsigned short k, bool isRepeat);
	bool Update();
	
	void ShowMapList();
	void ShowScriptList();
	void ShowModList();
	
	/// Callback functions for CglList
	void SelectScript(const std::string& s);
	void SelectMap(const std::string& s);
	void SelectMod(const std::string& s);
	
private:
	std::string userScript;
	std::string userMap;
	std::string userMod;
	
	bool addressKnown;
	LocalSetup* mySettings;
	CglList* showList;
};

#endif