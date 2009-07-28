#ifndef SELECT_MENU
#define SELECT_MENU

#include <boost/signals/connection.hpp>

#include "GameController.h"

class ClientSetup;
class CglList;
union SDL_Event;
class VerticalLayout;
class LineEdit;

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
	~SelectMenu();

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
	void Single();
	void Multi();
	void Quit();
	void DirectConnect();

	bool HandleEvent(const SDL_Event& ev);
	std::string userScript;
	std::string userMap;
	std::string userMod;

	bool addressKnown;
	ClientSetup* mySettings;
	CglList* showList;

	boost::signals::scoped_connection inputCon;
	VerticalLayout* menu;
	LineEdit* address;
	bool wantConnect;
};

#endif
