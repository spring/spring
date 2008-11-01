#ifndef SELECT_MENU
#define SELECT_MENU

#include "GameController.h"

class LocalSetup;
class CglList;

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
	CglList* showList;
	
	/// Choose the script we will tell the server to start with
	void SelectScript(const std::string& s);
	void SelectMap(const std::string& s);
	void SelectMod(const std::string& s);
	
private:
	std::string userScript;
	std::string userMap;
	std::string userMod;
	
	bool addressKnown;
	LocalSetup* mySettings;
};

#endif