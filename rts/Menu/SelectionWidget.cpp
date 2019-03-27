/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectionWidget.h"

#ifndef HEADLESS
#include <functional>

#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/DataDirsAccess.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Exceptions.h"
#include "System/Config/ConfigHandler.h"
#include "System/AIScriptHandler.h"
#include "ExternalAI/LuaAIImplHandler.h"
#include "ExternalAI/Interface/SSkirmishAILibrary.h"
#include "System/Info.h"
#include "alphanum.hpp"

const std::string SelectionWidget::NoDemoSelect = "No demo selected";
const std::string SelectionWidget::NoModSelect = "No game selected";
const std::string SelectionWidget::NoMapSelect = "No map selected";
const std::string SelectionWidget::NoScriptSelect = "No script selected";
const std::string SelectionWidget::SandboxAI = "Player Only: Testing Sandbox";

CONFIG(std::string, LastSelectedMod).defaultValue(SelectionWidget::NoModSelect).description("Stores the previously played game.");
CONFIG(std::string, LastSelectedMap).defaultValue(SelectionWidget::NoMapSelect).description("Stores the previously played map.");
CONFIG(std::string, LastSelectedScript).defaultValue(SelectionWidget::NoScriptSelect).description("Stores the previously played AI.");

// returns absolute filename for given archive name, empty if not found
static const std::string GetFileName(const std::string& name){
	if (name.empty())
		return name;
	const std::string& filename = archiveScanner->ArchiveFromName(name);
	if (filename == name)
		return "";
	const std::string& path = archiveScanner->GetArchivePath(filename);
	return path + filename;
}


SelectionWidget::SelectionWidget(agui::GuiElement* parent) : agui::GuiElement(parent)
{
	SetPos(0.5f, 0.2f);
	SetSize(0.4f, 0.2f);
	curSelect = nullptr;

	agui::VerticalLayout* vl = new agui::VerticalLayout(this);
	vl->SetBorder(true);

	agui::HorizontalLayout* modL = new agui::HorizontalLayout(vl);
	mod = new agui::Button("Select", modL);
	mod->Clicked.connect(std::bind(&SelectionWidget::ShowModList, this));
	mod->SetSize(0.1f, 0.00f, true);


	userDemo = NoDemoSelect;
	userMod = configHandler->GetString("LastSelectedMod");
	userMap = configHandler->GetString("LastSelectedMap");
	userScript = configHandler->GetString("LastSelectedScript");

	if (GetFileName(userMod).empty())
		userMod = NoModSelect;
	if (GetFileName(userMap).empty())
		userMap = NoMapSelect;

	agui::HorizontalLayout* mapL = new agui::HorizontalLayout(vl);
	map = new agui::Button("Select", mapL);
	map->Clicked.connect(std::bind(&SelectionWidget::ShowMapList, this));
	map->SetSize(0.1f, 0.00f, true);

	agui::HorizontalLayout* scriptL = new agui::HorizontalLayout(vl);
	script = new agui::Button("Select", scriptL);
	script->Clicked.connect(std::bind(&SelectionWidget::ShowScriptList, this));
	script->SetSize(0.1f, 0.00f, true);

	modT = new agui::TextElement(userMod, modL);
	mapT = new agui::TextElement(userMap, mapL);
	scriptT = new agui::TextElement(userScript, scriptL);

	UpdateAvailableScripts();
}

SelectionWidget::~SelectionWidget()
{
	CleanWindow();
}


void SelectionWidget::ShowDemoList(const std::function<void(const std::string&)>& demoSelectCB)
{
	if (curSelect != nullptr)
		return;

	curSelect = new ListSelectWnd("Select demo");
	curSelect->Selected.connect(std::bind(&SelectionWidget::SelectDemo, this, std::placeholders::_1));
	curSelect->WantClose.connect(std::bind(&SelectionWidget::CleanWindow, this));

	const std::string cwd = std::move(FileSystem::EnsurePathSepAtEnd(FileSystemAbstraction::GetCwd()));
	const std::string dir = std::move(FileSystem::EnsurePathSepAtEnd("demos"));

	const std::vector<std::string> demos(dataDirsAccess.FindFiles(cwd + dir, "*.sdfz", 0));

	// FIXME: names overflow the box
	for (const std::string& demo: demos) {
		curSelect->list->AddItem(demo.substr(demo.find(dir) + 6), "");
	}

	demoSelectedCB = demoSelectCB;
}

void SelectionWidget::ShowModList()
{
	if (curSelect != nullptr)
		return;

	curSelect = new ListSelectWnd("Select game");
	curSelect->Selected.connect(std::bind(&SelectionWidget::SelectMod, this, std::placeholders::_1));
	curSelect->WantClose.connect(std::bind(&SelectionWidget::CleanWindow, this));

	std::vector<CArchiveScanner::ArchiveData> found = std::move(archiveScanner->GetPrimaryMods());
	std::sort(found.begin(), found.end(), [](const CArchiveScanner::ArchiveData& a, const CArchiveScanner::ArchiveData& b) {
		return (doj::alphanum_less<std::string>()(a.GetNameVersioned(), b.GetNameVersioned()));
	});

	for (const CArchiveScanner::ArchiveData& ad: found) {
		curSelect->list->AddItem(ad.GetNameVersioned(), ad.GetDescription());
	}

	curSelect->list->SetCurrentItem(userMod);
}

void SelectionWidget::ShowMapList()
{
	if (curSelect != nullptr)
		return;

	curSelect = new ListSelectWnd("Select map");
	curSelect->Selected.connect(std::bind(&SelectionWidget::SelectMap, this, std::placeholders::_1));
	curSelect->WantClose.connect(std::bind(&SelectionWidget::CleanWindow, this));

	std::vector<std::string> arFound = std::move(archiveScanner->GetMaps());
	std::sort(arFound.begin(), arFound.end(), doj::alphanum_less<std::string>());

	for (const std::string& arName: arFound) {
		curSelect->list->AddItem(arName, arName);
	}

	curSelect->list->SetCurrentItem(userMap);
}

void SelectionWidget::AddAIScriptsFromArchive()
{
	if (userMod == SelectionWidget::NoModSelect || userMap == SelectionWidget::NoMapSelect )
		return;

	vfsHandler->AddArchive(userMod, true);
	vfsHandler->AddArchive(userMap, true);

	const CLuaAIImplHandler::InfoItemVector& luaAIInfos = luaAIImplHandler.LoadInfoItems();

	for (const auto& luaAIInfo: luaAIInfos) {
		for (const auto& prop: luaAIInfo) {
			if (prop.key == SKIRMISH_AI_PROPERTY_SHORT_NAME)
				availableScripts.push_back(prop.GetValueAsString());
		}
	}

	vfsHandler->RemoveArchive(userMap);
	vfsHandler->RemoveArchive(userMod);
}

void SelectionWidget::UpdateAvailableScripts()
{
	//FIXME: lua ai's should be handled in AIScriptHandler.cpp, too respecting the selected game and map
	// maybe also merge it with StartScriptGen.cpp

	availableScripts.clear();
	availableScripts.reserve(16);
	// load selected archives to get lua ais

	AddAIScriptsFromArchive();

	// add sandbox script to list
	availableScripts.push_back(SandboxAI);

	// add native ai's to the list, too (but second, lua ai's are prefered)
	for (const auto& pair: CAIScriptHandler::Instance().GetScriptMap()) {
		availableScripts.push_back(pair.first);
	}

	if (std::find(availableScripts.begin(), availableScripts.end(), userScript) != availableScripts.end())
		return;

	SelectScript(SelectionWidget::NoScriptSelect);
}

void SelectionWidget::ShowScriptList()
{
	if (curSelect != nullptr)
		return;

	curSelect = new ListSelectWnd("Select script");
	curSelect->Selected.connect(std::bind(&SelectionWidget::SelectScript, this, std::placeholders::_1));
	curSelect->WantClose.connect(std::bind(&SelectionWidget::CleanWindow, this));

	for (const std::string& scriptName: availableScripts) {
		curSelect->list->AddItem(scriptName, "");
	}


	curSelect->list->SetCurrentItem(userScript);
}



void SelectionWidget::SelectDemo(const std::string& demo)
{
	CleanWindow();
	demoSelectedCB("demos/" + demo);
}

void SelectionWidget::SelectMod(const std::string& mod)
{
	if (mod == userMod) {
		CleanWindow();
		return;
	}

	configHandler->SetString("LastSelectedMod", userMod = mod);
	modT->SetText(userMod);

	//SelectScript(SelectionWidget::NoScriptSelect); //reset AI as LuaAI maybe doesn't exist in this game
	UpdateAvailableScripts();
	CleanWindow();
}

void SelectionWidget::SelectScript(const std::string& script)
{
	configHandler->SetString("LastSelectedScript", userScript = script);
	scriptT->SetText(userScript);

	CleanWindow();
}

void SelectionWidget::SelectMap(const std::string& map)
{
	configHandler->SetString("LastSelectedMap", userMap = map);
	mapT->SetText(userMap);

	UpdateAvailableScripts();
	CleanWindow();
}


void SelectionWidget::CleanWindow()
{
	if (curSelect == nullptr)
		return;

	agui::gui->RmElement(curSelect);
	curSelect = nullptr;
}
#endif

