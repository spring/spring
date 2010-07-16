/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "SelectionWidget.h"

#include <set>

#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileSystem.h"
#include "Exceptions.h"
#include "ConfigHandler.h"
#include "ScriptHandler.h"


const std::string SelectionWidget::NoModSelect = "No mod selected";
const std::string SelectionWidget::NoMapSelect = "No map selected";
const std::string SelectionWidget::NoScriptSelect = "No script selected";

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
	userMod = configHandler->GetString("LastSelectedMod", NoModSelect);
	if (archiveScanner->GetSingleArchiveChecksum(archiveScanner->ArchiveFromName(userMod)) == 0)
		userMod = NoModSelect;
	modT = new agui::TextElement(userMod, modL);
	agui::HorizontalLayout* mapL = new agui::HorizontalLayout(vl);
	map = new agui::Button("Select", mapL);
	map->Clicked.connect(boost::bind(&SelectionWidget::ShowMapList, this));
	map->SetSize(0.1f, 0.00f, true);
	userMap = configHandler->GetString("LastSelectedMap", NoMapSelect);
	if (archiveScanner->GetSingleArchiveChecksum(archiveScanner->ArchiveFromName(userMap)) == 0)
		userMap = NoMapSelect;
	mapT = new agui::TextElement(userMap, mapL);
	agui::HorizontalLayout* scriptL = new agui::HorizontalLayout(vl);
	script = new agui::Button("Select", scriptL);
	script->Clicked.connect(boost::bind(&SelectionWidget::ShowScriptList, this));
	script->SetSize(0.1f, 0.00f, true);
	userScript = configHandler->GetString("LastSelectedScript", NoScriptSelect);
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
	curSelect = new ListSelectWnd("Select mod");
	curSelect->Selected.connect(boost::bind(&SelectionWidget::SelectMod, this, _1));
	curSelect->WantClose.connect(boost::bind(&SelectionWidget::CleanWindow, this));

	std::vector<CArchiveScanner::ArchiveData> found = archiveScanner->GetPrimaryMods();

	std::map<std::string, std::string> modMap; // name, desc  (using a map to sort)
	for (std::vector<CArchiveScanner::ArchiveData>::iterator it = found.begin(); it != found.end(); ++it) {
		modMap[it->name] = it->description;
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

	std::vector<std::string> arFound = archiveScanner->GetMaps();

	std::set<std::string> mapSet; // use a set to sort them
	for (std::vector<std::string>::iterator it = arFound.begin(); it != arFound.end(); it++) {
		mapSet.insert((*it).c_str());
	}

	for (std::set<std::string>::iterator sit = mapSet.begin(); sit != mapSet.end(); ++sit) {
		curSelect->list->AddItem(*sit, *sit);
	}
	curSelect->list->SetCurrentItem(userMap);
}

void SelectionWidget::ShowScriptList()
{
	if (curSelect)
		return;
	curSelect = new ListSelectWnd("Select script");
	curSelect->Selected.connect(boost::bind(&SelectionWidget::SelectScript, this, _1));
	curSelect->WantClose.connect(boost::bind(&SelectionWidget::CleanWindow, this));

	CScriptHandler::ScriptList scriptList = CScriptHandler::Instance().GetScriptList();
	for (CScriptHandler::ScriptList::iterator it = scriptList.begin(); it != scriptList.end(); ++it) {
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
