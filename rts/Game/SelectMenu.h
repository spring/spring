#ifndef SELECT_MENU
#define SELECT_MENU

#include "aGui/GuiElement.h"
#include "GameController.h"

class ClientSetup;
union SDL_Event;
class SelectionWidget;
namespace agui
{
class Window;
class LineEdit;
class Picture;
}

/**
@brief User prompt for options when no script is given

When no setupscript is given, this will show a menu to select server address (when running in client ("-c")mode).
If in host mode, it will show lists for Map, Mod and Script.
When everything is selected, it will generate a gamesetup-script and start CPreGame
*/
class SelectMenu : public CGameController, public agui::GuiElement
{
public:
	SelectMenu(bool server);
	~SelectMenu();

	bool Draw();
	bool Update();

private:
	void Single();
	void Settings();
	void Multi();
	void Quit();
	void ConnectWindow(bool show);
	void DirectConnect();

	bool HandleEventSelf(const SDL_Event& ev);

	void SelectScript(const std::string& s);
	void SelectMap(const std::string& s);
	void SelectMod(const std::string& s);

	ClientSetup* mySettings;

	agui::LineEdit* address;
	agui::Window* connectWnd;
	agui::Picture* background;
	SelectionWidget* selw;
};

#endif
