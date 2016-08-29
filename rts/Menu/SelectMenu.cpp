/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SelectMenu.h"

#include <SDL_keycode.h>
#include <boost/bind.hpp>
#include <sstream>
#include <stack>
#include <boost/cstdint.hpp>

#include "SelectionWidget.h"
#include "System/AIScriptHandler.h"
#include "Game/ClientSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/PreGame.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/Util.h"
#include "System/Input/InputHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/MsgStrings.h"
#include "System/StartScriptGen.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"
#include "aGui/Window.h"
#include "aGui/Picture.h"
#include "aGui/List.h"
#include "alphanum.hpp"

using std::string;
using agui::Button;
using agui::HorizontalLayout;

CONFIG(std::string, address).defaultValue("").description("Last Ip/hostname used as direct connect in the menu.");
CONFIG(std::string, LastSelectedSetting).defaultValue("").description("Stores the previously selected setting, when editing settings within the Spring main menu.");
CONFIG(std::string, MenuArchive).defaultValue("Spring Bitmaps").description("Archive name for the default Menu.");

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
		address->SetContent(configHandler->GetString("address"));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* connect = new Button("Connect", buttons);
		connect->Clicked.connect(boost::bind(&ConnectWindow::Finish, this, true));
		Button* close = new Button("Close", buttons);
		close->Clicked.connect(boost::bind(&ConnectWindow::Finish, this, false));
		GeometryChange();
	}

	boost::signals2::signal<void (std::string)> Connect;
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
		if (configHandler->IsSet(name))
			value->SetContent(configHandler->GetString(name));
		HorizontalLayout* buttons = new HorizontalLayout(wndLayout);
		Button* ok = new Button("OK", buttons);
		ok->Clicked.connect(boost::bind(&SettingsWindow::Finish, this, true));
		Button* close = new Button("Cancel", buttons);
		close->Clicked.connect(boost::bind(&SettingsWindow::Finish, this, false));
		GeometryChange();
	}

	boost::signals2::signal<void (std::string)> OK;
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

SelectMenu::SelectMenu(boost::shared_ptr<ClientSetup> setup)
: GuiElement(NULL)
, clientSetup(setup)
, conWindow(NULL)
, settingsWindow(NULL)
, curSelect(NULL)
{
	SetPos(0,0);
	SetSize(1,1);
	agui::gui->AddElement(this, true);

	{ // GUI stuff
		agui::Picture* background = new agui::Picture(this);;
		{
			const std::string archiveName = configHandler->GetString("MenuArchive");
			vfsHandler->AddArchive(archiveName, false);
			const std::vector<std::string> files = CFileHandler::FindFiles("bitmaps/ui/background/", "*");
			if (!files.empty()) {
				//TODO: select by resolution / aspect ratio with fallback image
				background->Load(files[gu->RandInt() % files.size()]);
			}
			vfsHandler->RemoveArchive(archiveName);
		}
		selw = new SelectionWidget(this);
		agui::VerticalLayout* menu = new agui::VerticalLayout(this);
		menu->SetPos(0.1, 0.5);
		menu->SetSize(0.4, 0.4);
		menu->SetBorder(1.2f);
		/*agui::TextElement* title = */new agui::TextElement("Spring", menu); // will be deleted in menu
		Button* single = new Button("Test the Game", menu);
		single->Clicked.connect(boost::bind(&SelectMenu::Single, this));

		userSetting = configHandler->GetString("LastSelectedSetting");
		Button* editsettings = new Button("Edit settings", menu);
		editsettings->Clicked.connect(boost::bind(&SelectMenu::ShowSettingsList, this));

		Button* direct = new Button("Direct connect", menu);
		direct->Clicked.connect(boost::bind(&SelectMenu::ShowConnectWindow, this, true));

		Button* quit = new Button("Quit", menu);
		quit->Clicked.connect(boost::bind(&SelectMenu::Quit, this));
		background->GeometryChange();
	}

	if (!clientSetup->isHost) {
		ShowConnectWindow(true);
	}
}

SelectMenu::~SelectMenu()
{
	ShowConnectWindow(false);
	ShowSettingsWindow(false, "");
	CleanWindow();
}

bool SelectMenu::Draw()
{
	spring_msecs(10).sleep();
	ClearScreen();
	agui::gui->Draw();

	return true;
}

void SelectMenu::Single()
{
	if (selw->userMod == SelectionWidget::NoModSelect) {
		selw->ShowModList();
	} else if (selw->userMap == SelectionWidget::NoMapSelect) {
		selw->ShowMapList();
	} else if (selw->userScript == SelectionWidget::NoScriptSelect) {
		selw->ShowScriptList();
	}
	else if (pregame == NULL) {
		// in case of double-click
		if (selw->userScript == SelectionWidget::SandboxAI) {
			selw->userScript.clear();
		}

		pregame = new CPreGame(clientSetup);
		pregame->LoadSetupscript(StartScriptGen::CreateDefaultSetup(selw->userMap, selw->userMod, selw->userScript, clientSetup->myPlayerName));
		return (agui::gui->RmElement(this));
	}
}

void SelectMenu::Quit()
{
	gu->globalQuit = true;
	return (agui::gui->RmElement(this));
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
		size_t p = name.find(" = ");
		if(p != std::string::npos) {
			configHandler->SetString(name.substr(0,p), name.substr(p + 3));
			ShowSettingsList();
		}
		if(curSelect)
			curSelect->list->SetFocus(true);
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
	typedef std::map<std::string, std::string, doj::alphanum_less<std::string> > DataSorted;
	const DataSorted dataSorted(data.begin(), data.end());
	for(DataSorted::const_iterator iter = dataSorted.begin(); iter != dataSorted.end(); ++iter)
		curSelect->list->AddItem(iter->first + " = " + iter->second, "");
	if(data.find(userSetting) != data.end())
		curSelect->list->SetCurrentItem(userSetting + " = " + configHandler->GetString(userSetting));
	curSelect->list->RefreshQuery();
}

void SelectMenu::SelectSetting(std::string setting) {
	size_t p = setting.find(" = ");
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

	clientSetup->hostIP = addr;
	clientSetup->isHost = false;

	pregame = new CPreGame(clientSetup);
	return (agui::gui->RmElement(this));
}

bool SelectMenu::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				LOG("[SelectMenu] user exited");
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
