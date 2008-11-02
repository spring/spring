#include "SelectMenu.h"
#include "Rendering/GL/myGL.h"

#include <SDL_keysym.h>
#include <SDL_timer.h>
#include <SDL_types.h>
#include <boost/bind.hpp>
#include <sstream>
#include <stack>

#include "GameSetup.h"
#include "PreGame.h"
#include "Rendering/glFont.h"
#include "Rendering/GL/glList.h"
#include "System/LogOutput.h"
#include "System/Exceptions.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Platform/FileSystem.h"
#include "System/Platform/ConfigHandler.h"
#include "StartScripts/ScriptHandler.h"

using std::string;

extern Uint8* keys;
extern bool globalQuit;


class TdfWriter
{
public:
	void PushSection(const std::string& section)
	{
		Indent();
		str << "["<<section<<"]\n{\n";
		sections.push(section);
	};
	void PopSection()
	{
		assert(!sections.empty());
		Indent();
		str << "} //"<< sections.top() << "\n";
		sections.pop();
	};
	
	template<typename T>
	void AddPair(const std::string& key, const T& value)
	{
		Indent();
		str << key << "=" << value << ";\n";
	};
	
	std::string Get() const
	{
		return str.str();
	};
	
private:
	void Indent()
	{
		for (unsigned i = 0; i < sections.size(); ++i)
		str << "    ";
	};

	std::ostringstream str;
	std::stack<std::string> sections;
};

std::string CreateDefaultSetup(const std::string& map, const std::string& mod, const std::string& script, const std::string& playername)
{
	TdfWriter setup;
	setup.PushSection("GAME");
	setup.AddPair("Mapname", map);
	setup.AddPair("Gametype", mod);
	setup.AddPair("Scriptname", script);
	
	setup.AddPair("IsHost", 1);
	setup.AddPair("MyPlayerName", playername);
	
	setup.AddPair("NoHelperAIs", configHandler.GetInt("NoHelperAIs", 0));
	
	setup.PushSection("PLAYER0");
	setup.AddPair("Name", playername);
	setup.AddPair("Team", 0);
	setup.PopSection();
	
	setup.PushSection("PLAYER1");
	setup.AddPair("Name", "Enemy");
	setup.AddPair("Team", 1);
	setup.PopSection();
	
	setup.PushSection("TEAM0");
	setup.AddPair("Leader", 0);
	setup.AddPair("AllyTeam", 0);
	setup.PopSection();
	
	setup.PushSection("TEAM1");
	setup.AddPair("AllyTeam", 1);
	setup.PopSection();
	
	setup.PushSection("ALLYTEAM0");
	setup.AddPair("NumAllies", 0);
	setup.PopSection();
	
	setup.PushSection("ALLYTEAM1");
	setup.AddPair("NumAllies", 0);
	setup.PopSection();
	setup.PopSection();
	
	return setup.Get();
}

SelectMenu::SelectMenu(bool server) :
		showList(NULL)
{
	mySettings = new LocalSetup();
	mySettings->isHost = server;
	mySettings->myPlayerName = configHandler.GetString("name", "unnamed");
	if (!mySettings->isHost)
	{
		userInput=configHandler.GetString("address","");
		writingPos = userInput.length();
		userPrompt = "Enter server address: ";
		userWriting = true;
	}
	else
	{
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
	PrintLoadMsg("", false); // just clear screen and set up matrices etc.

	if (userWriting) {
		const std::string tempstring = userPrompt + userInput;

		const float xStart = 0.10f;
		const float yStart = 0.75f;

		const float fontScale = 1.0f;

		// draw the caret
		const int caretPos = userPrompt.length() + writingPos;
		const string caretStr = tempstring.substr(0, caretPos);
		const float caretWidth = font->CalcTextWidth(caretStr.c_str());
		char c = userInput[writingPos];
		if (c == 0) { c = ' '; }
		const float cw = fontScale * font->CalcCharWidth(c);
		const float csx = xStart + (fontScale * caretWidth);
		glDisable(GL_TEXTURE_2D);
		const float f = 0.5f * (1.0f + fastmath::sin((float)SDL_GetTicks() * 0.015f));
		glColor4f(f, f, f, 0.75f);
		glRectf(csx, yStart, csx + cw, yStart + fontScale * font->GetHeight());
		glEnable(GL_TEXTURE_2D);

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		font->glPrintAt(xStart, yStart, fontScale, tempstring.c_str());
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
			configHandler.SetString("address",userInput);
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
		list->AddItem(sit->c_str(), sit->c_str());
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
		list->AddItem(mit->first.c_str(), mit->second.c_str());
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
	
	pregame = new CPreGame(mySettings);
	pregame->LoadSetupscript(CreateDefaultSetup(userMap, userMod, userScript, mySettings->myPlayerName));
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

