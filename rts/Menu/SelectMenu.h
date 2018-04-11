/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECT_MENU
#define SELECT_MENU

#include "aGui/GuiElement.h"
#include "Game/GameController.h"
#include <memory>

class SelectionWidget;
class ConnectWindow;
class SettingsWindow;
class ListSelectWnd;
class ClientSetup;


/**
@brief User prompt for options when no script is given

When no setupscript is given, this will show a menu to select server address (when running in client ("-c")mode).
If in host mode, it will show lists for Map, Game and Script.
When everything is selected, it will generate a gamesetup-script and start CPreGame
*/
class SelectMenu : public CGameController, public agui::GuiElement
{
public:
	SelectMenu(std::shared_ptr<ClientSetup> setup);
	~SelectMenu();

	bool Draw();

private:
	void Demo();
	void Single();
	void Settings();
	void Multi();
	void Quit();
	void ShowConnectWindow(bool show);
	void DirectConnect(const std::string& addr);

	bool HandleEventSelf(const SDL_Event& ev);

	void SelectScript(const std::string& s);
	void SelectMap(const std::string& s);
	void SelectMod(const std::string& s);

	void ShowSettingsWindow(bool show, std::string name);
	void ShowSettingsList();
	void SelectSetting(std::string);
	void CleanWindow();

private:
	std::shared_ptr<ClientSetup> clientSetup;

	ConnectWindow* conWindow;
	SelectionWidget* selw;

	SettingsWindow* settingsWindow;
	ListSelectWnd* curSelect;
	std::string userSetting;
};

#endif
