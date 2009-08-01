#include "SelectionWidget.h"

#include <set>
#include <boost/bind.hpp>

#include "FileSystem/ArchiveScanner.h"
#include "FileSystem/FileSystem.h"
#include "Exceptions.h"
#include "Game/StartScripts/ScriptHandler.h"
#include "ConfigHandler.h"

#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/glList.h"


SelectionWidget::SelectionWidget()
{
	SetPos(0.5f, 0.2f);
	SetSize(0.4f, 0.2f);
	curSelect = NULL;
	curSelectList = NULL;
	
	agui::gui->AddElement(this);
	agui::VerticalLayout* vl = new agui::VerticalLayout(this);
	vl->SetBorder(1.2f);
	agui::HorizontalLayout* modL = new agui::HorizontalLayout(vl);
	mod = new agui::Button("Select", modL);
	mod->ClickHandler(boost::bind(&SelectionWidget::ShowModList, this));
	userMod = configHandler->GetString("LastSelectedMod", "No mod selected");
	modT = new agui::TextElement(userMod, modL);
	agui::HorizontalLayout* mapL = new agui::HorizontalLayout(vl);
	map = new agui::Button("Select", mapL);
	map->ClickHandler(boost::bind(&SelectionWidget::ShowMapList, this));
	userMap = configHandler->GetString("LastSelectedMap", "No map selected");
	mapT = new agui::TextElement(userMap, mapL);
	userMap = configHandler->GetString("LastSelectedMap", "No map selected");
	agui::HorizontalLayout* scriptL = new agui::HorizontalLayout(vl);
	script = new agui::Button("Select", scriptL);
	script->ClickHandler(boost::bind(&SelectionWidget::ShowScriptList, this));
	userScript = configHandler->GetString("LastSelectedScript", "No script selected");
	scriptT = new agui::TextElement(userScript, scriptL);
	
}

SelectionWidget::~SelectionWidget()
{
	agui::gui->RmElement(this);
}

void SelectionWidget::ShowModList()
{
	if (curSelect)
		return;
	curSelect = new agui::Window("Select mod");
	agui::gui->AddElement(curSelect);
	curSelect->SetPos(0.5, 0.2);
	curSelect->SetSize(0.4, 0.7);
	
	agui::VerticalLayout* modWindowLayout = new agui::VerticalLayout(curSelect);
	curSelectList = new agui::CglList(modWindowLayout);
	agui::Button* select = new agui::Button("Select", modWindowLayout);
	select->ClickHandler(boost::bind(&SelectionWidget::SelectMod, this));
	select->SetSize(0.0f, 0.04f, true);
	curSelect->GeometryChange();
	
	std::vector<CArchiveScanner::ModData> found = archiveScanner->GetPrimaryMods();
	if (found.empty()) {
		throw content_error("PreGame couldn't find any mod files");
		return;
	}

	std::map<std::string, std::string> modMap; // name, desc  (using a map to sort)
	for (std::vector<CArchiveScanner::ModData>::iterator it = found.begin(); it != found.end(); ++it) {
		modMap[it->name] = it->description;
	}

	curSelectList->AddItem("Random mod", "Random mod"); // always first
	std::map<std::string, std::string>::iterator mit;
	for (mit = modMap.begin(); mit != modMap.end(); ++mit) {
		curSelectList->AddItem(mit->first, mit->second);
	}
}

void SelectionWidget::ShowMapList()
{
	if (curSelect)
		return;
	curSelect = new agui::Window("Select map");
	agui::gui->AddElement(curSelect);
	curSelect->SetPos(0.5, 0.2);
	curSelect->SetSize(0.4, 0.7);
	
	agui::VerticalLayout* mapWindowLayout = new agui::VerticalLayout(curSelect);
	curSelectList = new agui::CglList(mapWindowLayout);
	agui::Button* select = new agui::Button("Select", mapWindowLayout);
	select->ClickHandler(boost::bind(&SelectionWidget::SelectMap, this));
	select->SetSize(0.0f, 0.04f, true);
	curSelect->GeometryChange();

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

	curSelectList->AddItem("Random map", "Random map"); // always first
	for (std::set<std::string>::iterator sit = mapSet.begin(); sit != mapSet.end(); ++sit) {
		curSelectList->AddItem(*sit, *sit);
	}
}

void SelectionWidget::ShowScriptList()
{
	if (curSelect)
		return;
	curSelect = new agui::Window("Select script");
	agui::gui->AddElement(curSelect);
	curSelect->SetPos(0.5, 0.2);
	curSelect->SetSize(0.4, 0.7);
	
	agui::VerticalLayout* scriptWindowLayout = new agui::VerticalLayout(curSelect);
	curSelectList = new agui::CglList(scriptWindowLayout);
	agui::Button* select = new agui::Button("Select", scriptWindowLayout);
	select->ClickHandler(boost::bind(&SelectionWidget::SelectScript, this));
	select->SetSize(0.0f, 0.04f, true);
	curSelect->GeometryChange();
	
	std::list<std::string> scriptList = CScriptHandler::Instance().ScriptList();
	for (std::list<std::string>::iterator it = scriptList.begin(); it != scriptList.end(); ++it)
	{
		curSelectList->AddItem(*it, "");
	}
}

void SelectionWidget::SelectMod()
{
	if (curSelectList)
	{
		userMod = curSelectList->GetCurrentItem();
		configHandler->SetString("LastSelectedMod", userMod);
		modT->SetText(userMod);
	}
	CleanWindow();
}

void SelectionWidget::SelectScript()
{
	if (curSelectList)
	{
		userScript = curSelectList->GetCurrentItem();
		configHandler->SetString("LastSelectedScript", userScript);
		scriptT->SetText(userScript);
	}
	CleanWindow();
}

void SelectionWidget::SelectMap()
{
	if (curSelectList)
	{
		userMap = curSelectList->GetCurrentItem();
		configHandler->SetString("LastSelectedMap", userMap);
		mapT->SetText(userMap);
	}
	CleanWindow();
}

void SelectionWidget::CleanWindow()
{
	if (curSelect)
	{
		agui::gui->RmElement(curSelect);
		curSelect = NULL;
		curSelectList = NULL;
	}
}
