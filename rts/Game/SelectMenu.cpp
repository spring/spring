#include "SelectMenu.h"
#include "Rendering/GL/myGL.h"

#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <boost/bind.hpp>
#include <sstream>
#include <unistd.h>
#include <stack>
#include <boost/cstdint.hpp>

#include "ClientSetup.h"
#include "SelectionWidget.h"
#include "PreGame.h"
#include "Rendering/glFont.h"
#include "LogOutput.h"
#include "Exceptions.h"
#include "TdfParser.h"
#include "Util.h"
#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileHandler.h"
#include "FileSystem/VFSHandler.h"
#include "FileSystem/FileSystem.h"
#include "ConfigHandler.h"
#include "InputHandler.h"
#include "StartScripts/ScriptHandler.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/glList.h"

using std::string;
using agui::Button;
using agui::HorizontalLayout;

extern boost::uint8_t* keys;
extern bool globalQuit;

std::string CreateDefaultSetup(const std::string& map, const std::string& mod, const std::string& script,
			const std::string& playername, int gameMode)
{
	TdfParser::TdfSection setup;
	TdfParser::TdfSection* game = setup.construct_subsection("GAME");
	game->add_name_value("Mapname", map);
	game->add_name_value("Gametype", mod);

	TdfParser::TdfSection* modopts = game->construct_subsection("MODOPTIONS");
	modopts->AddPair("GameMode", gameMode);
	modopts->AddPair("MaxSpeed", 20);
	modopts->add_name_value("Scriptname", script);

	game->AddPair("IsHost", 1);
	game->add_name_value("MyPlayerName", playername);

	game->AddPair("NoHelperAIs", configHandler->Get("NoHelperAIs", 0));

	TdfParser::TdfSection* player0 = game->construct_subsection("PLAYER0");
	player0->add_name_value("Name", playername);
	player0->AddPair("Team", 0);

	TdfParser::TdfSection* player1 = game->construct_subsection("PLAYER1");
	player1->add_name_value("Name", "Enemy");
	player1->AddPair("Team", 1);

	TdfParser::TdfSection* team0 = game->construct_subsection("TEAM0");
	team0->AddPair("TeamLeader", 0);
	team0->AddPair("AllyTeam", 0);

	TdfParser::TdfSection* team1 = game->construct_subsection("TEAM1");
	team1->AddPair("TeamLeader", 1);
	team1->AddPair("AllyTeam", 1);

	TdfParser::TdfSection* ally0 = game->construct_subsection("ALLYTEAM0");
	ally0->AddPair("NumAllies", 0);

	TdfParser::TdfSection* ally1 = game->construct_subsection("ALLYTEAM1");
	ally1->AddPair("NumAllies", 0);

	std::ostringstream str;
	setup.print(str);

	return str.str();
}

SelectMenu::SelectMenu(bool server): menu(NULL)
{
	connectWnd = NULL;
	list = NULL;
	mySettings = new ClientSetup();
	selw = new SelectionWidget();
	mySettings->isHost = server;
	mySettings->myPlayerName = configHandler->GetString("name", "UnnamedPlayer");

	if (mySettings->myPlayerName.empty()) {
		mySettings->myPlayerName = "UnnamedPlayer";
	} else {
		mySettings->myPlayerName = StringReplaceInPlace(mySettings->myPlayerName, ' ', '_');
	}

	{
		menu = new agui::VerticalLayout();
		menu->SetPos(0.1, 0.5);
		menu->SetSize(0.4, 0.4);
		menu->SetBorder(1.2f);
		agui::gui->AddElement(menu);
		agui::TextElement* name = new agui::TextElement("Spring", menu);
		Button* single = new Button("Singleplayer", menu);
		single->ClickHandler(boost::bind(&SelectMenu::Single, this));
		Button* multi = new Button("Online Play", menu);
		multi->ClickHandler(boost::bind(&SelectMenu::Multi, this));
		Button* direct = new Button("Direct connect", menu);
		direct->ClickHandler(boost::bind(&SelectMenu::ConnectWindow, this, true));
		Button* quit = new Button("Quit", menu);
		quit->ClickHandler(boost::bind(&SelectMenu::Quit, this));
	}
	
	if (!mySettings->isHost) {
		ConnectWindow(true);
	}
}

SelectMenu::~SelectMenu()
{
	ConnectWindow(false);
	agui::gui->RmElement(selw);
	if (menu)
	{
		agui::gui->RmElement(menu);
	}
}

int SelectMenu::KeyPressed(unsigned short k,bool isRepeat)
{
	if (k == SDLK_ESCAPE){
		if(keys[SDLK_LSHIFT]){
			logOutput.Print("User exited");
			globalQuit=true;
		} else
			logOutput.Print("Use shift-esc to quit");
	}

	return 0;
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
	return true;
}

void SelectMenu::Single()
{
	pregame = new CPreGame(mySettings);
	pregame->LoadSetupscript(CreateDefaultSetup(selw->userMap, selw->userMod, selw->userScript, mySettings->myPlayerName, 1));
}

void SelectMenu::Multi()
{
#ifdef __unix__
	const std::string defLobby = configHandler->GetString("DefaultLobby", "springlobby");
#else
	const std::string defLobby = configHandler->GetString("DefaultLobby", "springlobby.exe");
#endif
	execlp(defLobby.c_str(), defLobby.c_str(), NULL);
}

void SelectMenu::Quit()
{
	globalQuit = true;
}

void SelectMenu::ConnectWindow(bool show)
{
	if (show && !connectWnd)
	{
		connectWnd = new agui::Window("Connect to server");
		connectWnd->SetPos(0.5, 0.5);
		connectWnd->SetSize(0.4, 0.2);
		agui::gui->AddElement(connectWnd);
		menu = new agui::VerticalLayout(connectWnd);
		HorizontalLayout* input = new HorizontalLayout(menu);
		agui::TextElement* label = new agui::TextElement("Address:", input);
		address = new agui::LineEdit(input);
		address->SetFocus(true);
		address->SetContent(configHandler->GetString("address", ""));
		HorizontalLayout* buttons = new HorizontalLayout(menu);
		Button* close = new Button("Close", buttons);
		close->ClickHandler(boost::bind(&SelectMenu::ConnectWindow, this, false));
		Button* connect = new Button("Connect", buttons);
		connect->ClickHandler(boost::bind(&SelectMenu::DirectConnect, this));
	}
	else if (!show && connectWnd)
	{
		agui::gui->RmElement(connectWnd);
		connectWnd = NULL;
	}
}

void SelectMenu::DirectConnect()
{
	configHandler->SetString("address",address->GetContent());
	mySettings->hostip = address->GetContent();
	mySettings->isHost = false;
	pregame = new CPreGame(mySettings);
	delete this;
}

bool SelectMenu::HandleEvent(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			break;
		}
		case SDL_MOUSEMOTION: {
			break;
		}
	}
	return false;
}
