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
#include "PreGame.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
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

SelectMenu::SelectMenu(bool server): showList(NULL), menu(NULL)
{
	connectWnd = NULL;
	mySettings = new ClientSetup();
	mySettings->isHost = server;
	mySettings->myPlayerName = configHandler->GetString("name", "UnnamedPlayer");

	if (mySettings->myPlayerName.empty()) {
		mySettings->myPlayerName = "UnnamedPlayer";
	} else {
		mySettings->myPlayerName = StringReplaceInPlace(mySettings->myPlayerName, ' ', '_');
	}

	{
		menu = new agui::VerticalLayout();
		menu->SetPos(0.3, 0.3);
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

	if (showList) { //are we currently showing a list?
		showList->KeyPressed(k, isRepeat);
		return 0;
	}

	return 0;
}

bool SelectMenu::Draw()
{
	SDL_Delay(10); // milliseconds
	ClearScreen();
	agui::gui->Draw();

	if (showList) {
		showList->Draw();
	}

	return true;
}

bool SelectMenu::Update()
{
	return true;
}

/** Create a CglList for selecting the map. */
void SelectMenu::ShowMapList()
{
	CglList* list = new CglList("Select map", boost::bind(&SelectMenu::SelectMap, this, _1), 2);
	std::vector<std::string> found = filesystem.FindFiles("maps/","{*.sm3,*.smf}");
	std::vector<std::string> arFound = archiveScanner->GetMaps();
	if (found.begin() == found.end() && arFound.begin() == arFound.end()) {
		throw content_error("PreGame couldn't find any map files");
		return;
	}

	std::set<std::string> mapSet; // use a set to sort them
	for (std::vector<std::string>::iterator it = found.begin(); it != found.end(); it++) {
		std::string fn(filesystem.GetFilename(*it));
		mapSet.insert(fn.c_str());
	}
	for (std::vector<std::string>::iterator it = arFound.begin(); it != arFound.end(); it++) {
		mapSet.insert((*it).c_str());
	}

	list->AddItem("Random map", "Random map"); // always first
	for (std::set<std::string>::iterator sit = mapSet.begin(); sit != mapSet.end(); ++sit) {
		list->AddItem(*sit, *sit);
	}
	showList = list;
}


/** Create a CglList for selecting the script. */
void SelectMenu::ShowScriptList()
{
	CglList* list = CScriptHandler::Instance().GenList(boost::bind(&SelectMenu::SelectScript, this, _1));
	showList = list;
}


/** Create a CglList for selecting the mod. */
void SelectMenu::ShowModList()
{
	if (menu)
	{
		agui::gui->RmElement(menu);
		menu = NULL;
	}
	inputCon = input.AddHandler(boost::bind(&SelectMenu::HandleEvent, this, _1));
	CglList* list = new CglList("Select mod", boost::bind(&SelectMenu::SelectMod, this, _1), 3);
	std::vector<CArchiveScanner::ModData> found = archiveScanner->GetPrimaryMods();
	if (found.empty()) {
		throw content_error("PreGame couldn't find any mod files");
		return;
	}

	std::map<std::string, std::string> modMap; // name, desc  (using a map to sort)
	for (std::vector<CArchiveScanner::ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		modMap[it->name] = it->description;
	}

	list->AddItem("Random mod", "Random mod"); // always first
	std::map<std::string, std::string>::iterator mit;
	for (mit = modMap.begin(); mit != modMap.end(); ++mit) {
		list->AddItem(mit->first, mit->second);
	}
	showList = list;
}


void SelectMenu::SelectMap(const std::string& s)
{
	if (s == "Random map") {
		userMap = showList->items[1 + gu->usRandInt() % (showList->items.size() - 1)];
	}
	else
		userMap = s;

	delete showList;
	showList = NULL;

	int gamemode = 3;
	if (userScript.find("GlobalAI test") != std::string::npos)
		gamemode = 0;
	else
		logOutput.Print("Testing mode enabled; game over disabled.\n");

	pregame = new CPreGame(mySettings);
	pregame->LoadSetupscript(CreateDefaultSetup(userMap, userMod, userScript, mySettings->myPlayerName, gamemode));
	delete this;
}


void SelectMenu::SelectScript(const std::string& s)
{
	userScript = s;

	delete showList;
	showList = NULL;

	ShowMapList();
}


void SelectMenu::SelectMod(const std::string& s)
{
	if (s == "Random mod") {
		const int index = 1 + (gu->usRandInt() % (showList->items.size() - 1));
		userMod = showList->items[index];
	}
	else
		userMod = s;

	delete showList;
	showList = NULL;

	ShowScriptList();
}

void SelectMenu::Single()
{
	ShowModList();
}

void SelectMenu::Multi()
{
	const std::string defLobby = configHandler->GetString("DefaultLobby", "springlobby");
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
			if (showList)
				showList->MousePress(ev.button.x, ev.button.y, ev.button.button);
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			if (showList)
				showList->MouseRelease(ev.button.x, ev.button.y, ev.button.button);
			break;
		}
		case SDL_MOUSEMOTION: {
			if (showList)
				showList->MouseMove(ev.motion.x, ev.motion.y, ev.motion.xrel, ev.motion.yrel, ev.motion.state);
			break;
		}
	}
	return false;
}
