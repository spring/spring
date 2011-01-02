/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "SelectMenu.h"

#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <boost/bind.hpp>
#include <sstream>
#ifndef _WIN32
	#include <unistd.h>
	#define EXECLP execlp
#else
	#include <process.h>
	#define EXECLP _execlp
#endif
#include <stack>
#include <boost/cstdint.hpp>

#include "LobbyConnection.h"
#include "Game/ClientSetup.h"
#include "SelectionWidget.h"
#include "Game/PreGame.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/myGL.h"
#include "System/ConfigHandler.h"
#include "System/GlobalUnsynced.h"
#include "System/Exceptions.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h"
#include "System/Util.h"
#include "System/Input/InputHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "ScriptHandler.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/Picture.h"
#include "aGui/List.h"

using std::string;
using agui::Button;
using agui::HorizontalLayout;

class ConnectWindow : public agui::Window
{
public:
	ConnectWindow() : agui::Window("Connect to server")
	{
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* label = */new agui::TextElement("Address:", input); // will be deleted in input
		address = new agui::LineEdit(input);
		address->DefaultAction.connect(boost::bind(&ConnectWindow::Finish, this, true));
		address->SetFocus(true);
		address->SetContent(configHandler->GetString("address", ""));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* connect = new Button("Connect", buttons);
		connect->Clicked.connect(boost::bind(&ConnectWindow::Finish, this, true));
		Button* close = new Button("Close", buttons);
		close->Clicked.connect(boost::bind(&ConnectWindow::Finish, this, false));
		GeometryChange();
	}

	boost::signal<void (std::string)> Connect;
	agui::LineEdit* address;

private:
	void Finish(bool connect)
	{
		if (connect)
			Connect(address->GetContent());
		else
			WantClose();
	};
};

class SettingsWindow : public agui::Window
{
public:
	SettingsWindow(std::string &name) : agui::Window(name)
	{
		agui::gui->AddElement(this);
		SetPos(0.5, 0.5);
		SetSize(0.4, 0.2);

		agui::VerticalLayout* wndLayout = new agui::VerticalLayout(this);
		HorizontalLayout* input = new HorizontalLayout(wndLayout);
		/*agui::TextElement* value_label = */new agui::TextElement("Value:", input); // will be deleted in input
		value = new agui::LineEdit(input);
		value->DefaultAction.connect(boost::bind(&SettingsWindow::Finish, this, true));
		value->SetFocus(true);
		value->SetContent(configHandler->GetString(name, ""));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* ok = new Button("OK", buttons);
		ok->Clicked.connect(boost::bind(&SettingsWindow::Finish, this, true));
		Button* close = new Button("Cancel", buttons);
		close->Clicked.connect(boost::bind(&SettingsWindow::Finish, this, false));
		GeometryChange();
	}

	boost::signal<void (std::string)> OK;
	agui::LineEdit* value;

private:
	void Finish(bool set)
	{
		if (set)
			OK(title + " = " + value->GetContent());
		else
			WantClose();
	};
};

std::string CreateDefaultSetup(const std::string& map, const std::string& mod, const std::string& script,
			const std::string& playername)
{
	TdfParser::TdfSection setup;
	TdfParser::TdfSection* game = setup.construct_subsection("GAME");
	game->add_name_value("Mapname", map);
	game->add_name_value("Gametype", mod);

	TdfParser::TdfSection* modopts = game->construct_subsection("MODOPTIONS");
	modopts->AddPair("MaxSpeed", 20);

	game->AddPair("IsHost", 1);
	game->AddPair("OnlyLocal", 1);
	game->add_name_value("MyPlayerName", playername);

	game->AddPair("NoHelperAIs", configHandler->Get("NoHelperAIs", 0));

	TdfParser::TdfSection* player0 = game->construct_subsection("PLAYER0");
	player0->add_name_value("Name", playername);
	player0->AddPair("Team", 0);

	const bool isSkirmishAITestScript = CScriptHandler::Instance().IsSkirmishAITestScript(script);
	if (isSkirmishAITestScript) {
		SkirmishAIData aiData = CScriptHandler::Instance().GetSkirmishAIData(script);
		TdfParser::TdfSection* ai = game->construct_subsection("AI0");
		ai->add_name_value("Name", "Enemy");
		ai->add_name_value("ShortName", aiData.shortName);
		ai->add_name_value("Version", aiData.version);
		ai->AddPair("Host", 0);
		ai->AddPair("Team", 1);
	} else {
		TdfParser::TdfSection* player1 = game->construct_subsection("PLAYER1");
		player1->add_name_value("Name", "Enemy");
		player1->AddPair("Team", 1);
	}

	TdfParser::TdfSection* team0 = game->construct_subsection("TEAM0");
	team0->AddPair("TeamLeader", 0);
	team0->AddPair("AllyTeam", 0);

	TdfParser::TdfSection* team1 = game->construct_subsection("TEAM1");
	if (isSkirmishAITestScript) {
		team1->AddPair("TeamLeader", 0);
	} else {
		team1->AddPair("TeamLeader", 1);
	}
	team1->AddPair("AllyTeam", 1);

	TdfParser::TdfSection* ally0 = game->construct_subsection("ALLYTEAM0");
	ally0->AddPair("NumAllies", 0);

	TdfParser::TdfSection* ally1 = game->construct_subsection("ALLYTEAM1");
	ally1->AddPair("NumAllies", 0);

	std::ostringstream str;
	setup.print(str);

	return str.str();
}

SelectMenu::SelectMenu(bool server) : GuiElement(NULL), conWindow(NULL), updWindow(NULL), settingsWindow(NULL), curSelect(NULL)
{
	SetPos(0,0);
	SetSize(1,1);
	agui::gui->AddElement(this, true);
	mySettings = new ClientSetup();

	mySettings->isHost = server;
	mySettings->myPlayerName = configHandler->GetString("name", "UnnamedPlayer");
	if (mySettings->myPlayerName.empty()) {
		mySettings->myPlayerName = "UnnamedPlayer";
	} else {
		mySettings->myPlayerName = StringReplaceInPlace(mySettings->myPlayerName, ' ', '_');
	}

	{ // GUI stuff
		agui::Picture* background = new agui::Picture(this);;
		{
			std::string archive = archiveScanner->ArchiveFromName("Spring Bitmaps");
			std::string archivePath = archiveScanner->GetArchivePath(archive)+archive;
			vfsHandler->AddArchive(archivePath, false);
			background->Load("bitmaps/ui/background.jpg");
			vfsHandler->RemoveArchive(archivePath);
		}
		selw = new SelectionWidget(this);
		agui::VerticalLayout* menu = new agui::VerticalLayout(this);
		menu->SetPos(0.1, 0.5);
		menu->SetSize(0.4, 0.4);
		menu->SetBorder(1.2f);
		/*agui::TextElement* title = */new agui::TextElement("Spring", menu); // will be deleted in menu
		Button* single = new Button("Test the Game", menu);
		single->Clicked.connect(boost::bind(&SelectMenu::Single, this));
		Button* multi = new Button("Start the Lobby", menu);
		multi->Clicked.connect(boost::bind(&SelectMenu::Multi, this));
		Button* update = new Button("Lobby connect (WIP)", menu);
		update->Clicked.connect(boost::bind(&SelectMenu::ShowUpdateWindow, this, true));

		userSetting = configHandler->GetString("LastSelectedSetting", "");
		Button* editsettings = new Button("Edit settings", menu);
		editsettings->Clicked.connect(boost::bind(&SelectMenu::ShowSettingsList, this));

		Button* settings = new Button("Start SpringSettings", menu);
		settings->Clicked.connect(boost::bind(&SelectMenu::Settings, this));

		Button* direct = new Button("Direct connect", menu);
		direct->Clicked.connect(boost::bind(&SelectMenu::ShowConnectWindow, this, true));

		Button* quit = new Button("Quit", menu);
		quit->Clicked.connect(boost::bind(&SelectMenu::Quit, this));
		background->GeometryChange();
	}

	if (!mySettings->isHost) {
		ShowConnectWindow(true);
	}
}

SelectMenu::~SelectMenu()
{
	ShowConnectWindow(false);
	CleanWindow();
	delete updWindow;
	delete mySettings;
}

bool SelectMenu::Draw()
{
	SDL_Delay(10); // milliseconds
	ClearScreen();
	agui::gui->Draw();

	return true;
}

bool SelectMenu::Update()
{
	if (updWindow)
	{
		updWindow->Poll();
		if (updWindow->WantClose())
		{
			delete updWindow;
			updWindow = NULL;
		}
	}

	return true;
}

void SelectMenu::Single()
{
	static bool once = false;
	if (selw->userMod == SelectionWidget::NoModSelect)
	{
		selw->ShowModList();
	}
	else if (selw->userMap == SelectionWidget::NoMapSelect)
	{
		selw->ShowMapList();
	}
	else if (selw->userScript == SelectionWidget::NoScriptSelect)
	{
		selw->ShowScriptList();
	}
	else if (!once) // in case of double-click
	{
		once = true;
		mySettings->isHost = true;
		pregame = new CPreGame(mySettings);
		pregame->LoadSetupscript(CreateDefaultSetup(selw->userMap, selw->userMod, selw->userScript, mySettings->myPlayerName));
		agui::gui->RmElement(this);
	}
}

void SelectMenu::Settings()
{
#ifdef __unix__
	const std::string settingsProgram = "springsettings";
#else
	const std::string settingsProgram = "springsettings.exe";
#endif
	EXECLP(settingsProgram.c_str(), Quote(settingsProgram).c_str(), NULL);
}

void SelectMenu::Multi()
{
#ifdef __unix__
	const std::string defLobby = configHandler->GetString("DefaultLobby", "springlobby");
#else
	const std::string defLobby = configHandler->GetString("DefaultLobby", "springlobby.exe");
#endif
	EXECLP(defLobby.c_str(), Quote(defLobby).c_str(), NULL);
}

void SelectMenu::Quit()
{
	gu->globalQuit = true;
}

void SelectMenu::ShowConnectWindow(bool show)
{
	if (show && !conWindow)
	{
		conWindow = new ConnectWindow();
		conWindow->Connect.connect(boost::bind(&SelectMenu::DirectConnect, this, _1));
		conWindow->WantClose.connect(boost::bind(&SelectMenu::ShowConnectWindow, this, false));
	}
	else if (!show && conWindow)
	{
		agui::gui->RmElement(conWindow);
		conWindow = NULL;
	}
}

void SelectMenu::ShowSettingsWindow(bool show, std::string name)
{
	if (show)
	{
		if(settingsWindow) {
			agui::gui->RmElement(settingsWindow);
			settingsWindow = NULL;
		}
		settingsWindow = new SettingsWindow(name);
		settingsWindow->OK.connect(boost::bind(&SelectMenu::ShowSettingsWindow, this, false, _1));
		settingsWindow->WantClose.connect(boost::bind(&SelectMenu::ShowSettingsWindow, this, false, ""));
	}
	else if (!show && settingsWindow)
	{
		agui::gui->RmElement(settingsWindow);
		settingsWindow = NULL;
		int p = name.find(" = ");
		if(p != std::string::npos) {
			configHandler->SetString(name.substr(0,p), name.substr(p + 3));
			ShowSettingsList();
		}
		if(curSelect)
			curSelect->list->SetFocus(true);
	}
}

void SelectMenu::ShowUpdateWindow(bool show)
{
	if (show)
	{
		if (!updWindow)
			updWindow = new LobbyConnection();
		updWindow->ConnectDialog(true);
	}
	else if (!show && updWindow)
	{
		delete updWindow;
		updWindow = NULL;
	}
}

void SelectMenu::ShowSettingsList()
{
	if (!curSelect) {
		curSelect = new ListSelectWnd("Select setting");
		curSelect->Selected.connect(boost::bind(&SelectMenu::SelectSetting, this, _1));
		curSelect->WantClose.connect(boost::bind(&SelectMenu::CleanWindow, this));
	}
	curSelect->list->RemoveAllItems();
	const std::map<std::string, std::string> &data = configHandler->GetData();
	for(std::map<std::string,std::string>::const_iterator iter = data.begin(); iter != data.end(); ++iter)
		curSelect->list->AddItem(iter->first + " = " + iter->second, "");
	if(data.find(userSetting) != data.end())
		curSelect->list->SetCurrentItem(userSetting + " = " + configHandler->GetString(userSetting, ""));
	curSelect->list->RefreshQuery();
}

void SelectMenu::SelectSetting(std::string setting) {
	int p = setting.find(" = ");
	if(p != std::string::npos)
		setting = setting.substr(0, p);
	userSetting = setting;
	configHandler->SetString("LastSelectedSetting", userSetting);
	ShowSettingsWindow(true, userSetting);
}

void SelectMenu::CleanWindow() {
	if (curSelect) {
		ShowSettingsWindow(false, "");
		agui::gui->RmElement(curSelect);
		curSelect = NULL;
	}
}

void SelectMenu::DirectConnect(const std::string& addr)
{
	configHandler->SetString("address", addr);
	mySettings->hostIP = addr;
	mySettings->isHost = false;
	pregame = new CPreGame(mySettings);
	agui::gui->RmElement(this);
}

bool SelectMenu::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				logOutput.Print("User exited");
				Quit();
			} else if (ev.key.keysym.sym == SDLK_RETURN) {
				Single();
				return true;
			}
			break;
		}
	}
	return false;
}
