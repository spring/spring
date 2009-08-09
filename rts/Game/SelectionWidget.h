#ifndef SELECTIONWIDGET_H 
#define SELECTIONWIDGET_H

#include <string>

#include "aGui/GuiElement.h"

namespace agui
{
class Button;
class TextElement;
}
class ListSelectWnd;

class SelectionWidget : public agui::GuiElement
{
public:
	static const std::string NoModSelect;
	static const std::string NoMapSelect;
	static const std::string NoScriptSelect;

	SelectionWidget(agui::GuiElement* parent);
	~SelectionWidget();
	
	void ShowModList();
	void ShowMapList();
	void ShowScriptList();

	void SelectMod(std::string);
	void SelectScript(std::string);
	void SelectMap(std::string);

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
	ListSelectWnd* curSelect;
};

#endif
