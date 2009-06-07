#include "SelectMenu.h"
#include "Rendering/GL/myGL.h"

#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <boost/bind.hpp>
#include <sstream>
#include <stack>

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
#include "StartScripts/ScriptHandler.h"
#include <boost/cstdint.hpp>

using std::string;

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

SelectMenu::SelectMenu(bool server): showList(NULL)
{
	mySettings = new ClientSetup();
	mySettings->isHost = server;
	mySettings->myPlayerName = configHandler->GetString("name", "UnnamedPlayer");

	if (mySettings->myPlayerName.empty()) {
		mySettings->myPlayerName = "UnnamedPlayer";
	} else {
		mySettings->myPlayerName = StringReplaceInPlace(mySettings->myPlayerName, ' ', '_');
	}

	if (!mySettings->isHost) {
		userInput = configHandler->GetString("address", "");
		writingPos = userInput.length();
		userPrompt = "Enter server address: ";
		userWriting = true;
	} else {
		ShowModList();
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

	if (userWriting) {
		keys[k] = true;
		if (k == SDLK_v && keys[SDLK_LCTRL]){
			PasteClipboard();
			return 0;
		}
		if(k == SDLK_BACKSPACE){
			if (!userInput.empty() && (writingPos > 0)) {
				userInput.erase(writingPos - 1, 1);
				writingPos--;
			}
			return 0;
		}
		if (k == SDLK_DELETE) {
			if (!userInput.empty() && (writingPos < (int)userInput.size())) {
				userInput.erase(writingPos, 1);
			}
			return 0;
		}
		else if (k == SDLK_LEFT) {
			writingPos = std::max(0, std::min((int)userInput.length(), writingPos - 1));
		}
		else if (k == SDLK_RIGHT) {
			writingPos = std::max(0, std::min((int)userInput.length(), writingPos + 1));
		}
		else if (k == SDLK_HOME) {
			writingPos = 0;
		}
		else if (k == SDLK_END) {
			writingPos = (int)userInput.length();
		}
		if (k == SDLK_RETURN){
			userWriting = false;
			return 0;
		}
		return 0;
	}

	return 0;
}

bool SelectMenu::Draw()
{
	SDL_Delay(10); // milliseconds
	ClearScreen();

	if (userWriting) {
		const std::string tempstring = userPrompt + userInput;

		const float xStart = 0.10f;
		const float yStart = 0.75f;

		const float fontScale = 1.0f;
		const float fontSize  = fontScale * font->GetSize();

		// draw the caret
		const int caretPos = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPos);
		const float caretWidth = fontSize * font->GetTextWidth(caretStr) * gu->pixelX;
		char c = userInput[writingPos];
		if (c == 0) { c = ' '; }
		const float cw = fontSize * font->GetCharacterWidth(c) * gu->pixelX;
		const float csx = xStart + caretWidth;
		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + fastmath::sin((float)SDL_GetTicks() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, yStart, csx + cw, yStart + fontSize * font->GetLineHeight() * gu->pixelY);
		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		font->glPrint(xStart, yStart, fontSize, FONT_NORM, tempstring);
	}

	if (showList) {
		showList->Draw();
	}

	return true;
}

bool SelectMenu::Update()
{
	if (!mySettings->isHost)
	{
		// we are a client, wait for user to type address
		if (!userWriting)
		{
			configHandler->SetString("address",userInput);
			mySettings->hostip = userInput;
			pregame = new CPreGame(mySettings);
			delete this;
		}
	}

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

