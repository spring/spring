/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectionWidget.h"

#include <set>

#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Exceptions.h"
#include "System/Config/ConfigHandler.h"
#include "System/AIScriptHandler.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Info.h"

const std::string SelectionWidget::NoModSelect = "No game selected";
const std::string SelectionWidget::NoMapSelect = "No map selected";
const std::string SelectionWidget::NoScriptSelect = "No script selected";
const std::string SelectionWidget::SandboxAI = "Player Only: Testing Sandbox";

CONFIG(std::string, LastSelectedMod).defaultValue(SelectionWidget::NoModSelect).description("Stores the previously played game.");
CONFIG(std::string, LastSelectedMap).defaultValue(SelectionWidget::NoMapSelect).description("Stores the previously played map.");
CONFIG(std::string, LastSelectedScript).defaultValue(SelectionWidget::NoScriptSelect).description("Stores the previously played AI.");

SelectionWidget::SelectionWidget(agui::GuiElement* parent) : agui::GuiElement(parent)
{
	SetPos(0.5f, 0.2f);
	SetSize(0.4f, 0.2f);
	curSelect = NULL;

	agui::VerticalLayout* vl = new agui::VerticalLayout(this);
	vl->SetBorder(1.2f);
	agui::HorizontalLayout* modL = new agui::HorizontalLayout(vl);
	mod = new agui::Button("Select", modL);
	mod->Clicked.connect(boost::bind(&SelectionWidget::ShowModList, this));
	mod->SetSize(0.1f, 0.00f, true);
	userMod = configHandler->GetString("LastSelectedMod");
	if (archiveScanner->GetSingleArchiveChecksum(archiveScanner->ArchiveFromName(userMod)) == 0)
		userMod = NoModSelect;
	modT = new agui::TextElement(userMod, modL);
	agui::HorizontalLayout* mapL = new agui::HorizontalLayout(vl);
	map = new agui::Button("Select", mapL);
	map->Clicked.connect(boost::bind(&SelectionWidget::ShowMapList, this));
	map->SetSize(0.1f, 0.00f, true);
	userMap = configHandler->GetString("LastSelectedMap");
	if (archiveScanner->GetSingleArchiveChecksum(archiveScanner->ArchiveFromName(userMap)) == 0)
		userMap = NoMapSelect;
	mapT = new agui::TextElement(userMap, mapL);
	agui::HorizontalLayout* scriptL = new agui::HorizontalLayout(vl);
	script = new agui::Button("Select", scriptL);
	script->Clicked.connect(boost::bind(&SelectionWidget::ShowScriptList, this));
	script->SetSize(0.1f, 0.00f, true);
	userScript = configHandler->GetString("LastSelectedScript");
	scriptT = new agui::TextElement(userScript, scriptL);
}

SelectionWidget::~SelectionWidget()
{
	CleanWindow();
}

void SelectionWidget::ShowModList()
{
	if (curSelect)
		return;
	curSelect = new ListSelectWnd("Select game");
	curSelect->Selected.connect(boost::bind(&SelectionWidget::SelectMod, this, _1));
	curSelect->WantClose.connect(boost::bind(&SelectionWidget::CleanWindow, this));

	const std::vector<CArchiveScanner::ArchiveData> &found = archiveScanner->GetPrimaryMods();

	std::map<std::string, std::string> modMap; // name, desc  (using a map to sort)
	for (std::vector<CArchiveScanner::ArchiveData>::const_iterator it = found.begin(); it != found.end(); ++it) {
		modMap[it->GetNameVersioned()] = it->GetDescription();
	}

	std::map<std::string, std::string>::iterator mit;
	for (mit = modMap.begin(); mit != modMap.end(); ++mit) {
		curSelect->list->AddItem(mit->first, mit->second);
	}
	curSelect->list->SetCurrentItem(userMod);
}

void SelectionWidget::ShowMapList()
{
	if (curSelect)
		return;
	curSelect = new ListSelectWnd("Select map");
	curSelect->Selected.connect(boost::bind(&SelectionWidget::SelectMap, this, _1));
	curSelect->WantClose.connect(boost::bind(&SelectionWidget::CleanWindow, this));

	const std::vector<std::string> &arFound = archiveScanner->GetMaps();

	std::set<std::string> mapSet; // use a set to sort them
	for (std::vector<std::string>::const_iterator it = arFound.begin(); it != arFound.end(); ++it) {
		mapSet.insert((*it).c_str());
	}

	for (std::set<std::string>::iterator sit = mapSet.begin(); sit != mapSet.end(); ++sit) {
		curSelect->list->AddItem(*sit, *sit);
	}
	curSelect->list->SetCurrentItem(userMap);
}


static const std::string GetFileName(const std::string& name){
	if (name.empty())
		return name;
	const std::string filename = archiveScanner->ArchiveFromName(name);
	const std::string path = archiveScanner->GetArchivePath(filename);
	return path + filename;
}

static void AddArchive(const std::string& name) {
	if (!name.empty()) {
		vfsHandler->AddArchive(GetFileName(name), true);
	}
}

static void RemoveArchive(const std::string& name) {
	if (!name.empty()) {
		vfsHandler->RemoveArchive(GetFileName(name));
	}
}

void SelectionWidget::ShowScriptList()
{
	if (curSelect)
		return;
	curSelect = new ListSelectWnd("Select script");
	curSelect->Selected.connect(boost::bind(&SelectionWidget::SelectScript, this, _1));
	curSelect->WantClose.connect(boost::bind(&SelectionWidget::CleanWindow, this));

	//FIXME: lua ai's should be handled in AIScriptHandler.cpp, too respecting the selected game and map
	// maybe also merge it with StartScriptGen.cpp

	// load selected archives to get lua ais
	AddArchive(userMod);
	AddArchive(userMap);

	std::vector< std::vector<InfoItem> > luaAIInfos = luaAIImplHandler.LoadInfos();
	for(int i=0; i<luaAIInfos.size(); i++) {
		for (int j=0; j<luaAIInfos[i].size(); j++) {
			if (luaAIInfos[i][j].key==SKIRMISH_AI_PROPERTY_SHORT_NAME)
				curSelect->list->AddItem(info_getValueAsString(&luaAIInfos[i][j]), "");
		}
	}

	// close archives
	RemoveArchive(userMap);
	RemoveArchive(userMod);

	// add sandbox script to list
	curSelect->list->AddItem(SandboxAI, "");

	// add native ai's to the list, too (but second, lua ai's are prefered)
	CAIScriptHandler::ScriptList scriptList = CAIScriptHandler::Instance().GetScriptList();
	for (CAIScriptHandler::ScriptList::iterator it = scriptList.begin(); it != scriptList.end(); ++it) {
		curSelect->list->AddItem(*it, "");
	}

	curSelect->list->SetCurrentItem(userScript);
}

void SelectionWidget::SelectMod(std::string mod)
{
	userMod = mod;
	configHandler->SetString("LastSelectedMod", userMod);
	modT->SetText(userMod);

	CleanWindow();
}

void SelectionWidget::SelectScript(std::string map)
{
	userScript = map;
	configHandler->SetString("LastSelectedScript", userScript);
	scriptT->SetText(userScript);

	CleanWindow();
}

void SelectionWidget::SelectMap(std::string script)
{
	userMap = script;
	configHandler->SetString("LastSelectedMap", userMap);
	mapT->SetText(userMap);

	CleanWindow();
}

void SelectionWidget::CleanWindow()
{
	if (curSelect)
	{
		agui::gui->RmElement(curSelect);
		curSelect = NULL;
	}
}
