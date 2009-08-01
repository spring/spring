#ifndef SELECTIONWIDGET_H 
#define SELECTIONWIDGET_H

#include <string>

#include "aGui/GuiElement.h"

namespace agui
{
class Button;
class CglList;
class Window;
class TextElement;
}

class SelectionWidget : public agui::GuiElement
{
public:
	SelectionWidget();
	~SelectionWidget();
	
	void ShowModList();
	void ShowMapList();
	void ShowScriptList();

	void SelectMod();
	void SelectScript();
	void SelectMap();

	std::string userScript;
	std::string userMap;
	std::string userMod;

private:
	void CleanWindow();

	agui::Button* mod;
	agui::TextElement* modT;
	agui::Button* map;
	agui::TextElement* mapT;
	agui::Button* script;
	agui::TextElement* scriptT;
	agui::Window* curSelect;
	agui::CglList* curSelectList;
};

#endif
