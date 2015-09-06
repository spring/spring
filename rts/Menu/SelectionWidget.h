/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SELECTIONWIDGET_H
#define SELECTIONWIDGET_H

#include <string>
#include <vector>
#include <boost/bind.hpp>

#include "aGui/GuiElement.h"
#include "aGui/Window.h"
#include "aGui/List.h"
#include "aGui/Gui.h"
#include "aGui/VerticalLayout.h"
#include "aGui/HorizontalLayout.h"
#include "aGui/Button.h"
#include "aGui/LineEdit.h"
#include "aGui/TextElement.h"

namespace agui
{
class Button;
class TextElement;
}

class ListSelectWnd : public agui::Window
{
public:
	ListSelectWnd(const std::string& title) : agui::Window(title)
	{
		agui::gui->AddElement(this);
		SetPos(0.5, 0.2);
		SetSize(0.4, 0.7);

		agui::VerticalLayout* modWindowLayout = new agui::VerticalLayout(this);
		list = new agui::List(modWindowLayout);
		list->FinishSelection.connect(boost::bind(&ListSelectWnd::SelectButton, this));
		agui::HorizontalLayout* buttons = new agui::HorizontalLayout(modWindowLayout);
		buttons->SetSize(0.0f, 0.04f, true);
		agui::Button* select = new agui::Button("Select", buttons);
		select->Clicked.connect(boost::bind(&ListSelectWnd::SelectButton, this));
		agui::Button* cancel = new agui::Button("Close", buttons);
		cancel->Clicked.connect(boost::bind(&ListSelectWnd::CancelButton, this));
		GeometryChange();
	}

	boost::signals2::signal<void (std::string)> Selected;
	agui::List* list;

private:
	void SelectButton()
	{
		list->SetFocus(false);
		Selected(list->GetCurrentItem());
	}
	void CancelButton()
	{
		WantClose();
	}
};

class SelectionWidget : public agui::GuiElement
{
public:
	static const std::string NoModSelect;
	static const std::string NoMapSelect;
	static const std::string NoScriptSelect;
	static const std::string SandboxAI;

	SelectionWidget(agui::GuiElement* parent);
	~SelectionWidget();

	void ShowModList();
	void ShowMapList();
	void ShowScriptList();

	void SelectMod(const std::string&);
	void SelectScript(const std::string&);
	void SelectMap(const std::string&);

	std::string userScript;
	std::string userMap;
	std::string userMod;

private:
	void CleanWindow();
	void UpdateAvailableScripts();
	
	agui::Button* mod;
	agui::TextElement* modT;
	agui::Button* map;
	agui::TextElement* mapT;
	agui::Button* script;
	agui::TextElement* scriptT;
	ListSelectWnd* curSelect;
	std::vector<std::string> availableScripts;
};

#endif
