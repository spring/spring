#if !defined(GUIBUILDMENU_H)
#define GUIBUILDMENU_H

#include "GUIframe.h"
#include "Functor.h"

#include <string>
#include <vector>
class GUIlabel;
class GUIbutton;
struct UnitDef;
struct CommandDescription;

class GUIbuildMenu : public GUIframe
{
public:
	GUIbuildMenu(int x, int y, int w, int h, Functor1<UnitDef*> select);
	virtual ~GUIbuildMenu();
	
	static void SetBuildMenuOptions(const std::vector<CommandDescription*>& newOpts);

	void Move(GUIbutton* button);
	
	void SetBuildPicSize(int size) { buildPicSize=size; }
protected:
	void PrivateDraw();
	bool MouseUpAction(int x, int y, int button);
	bool MouseDownAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int button);
	bool EventAction(const std::string& event);

	bool isPressed;
	int hiliteNum;
	
	int buildPicSize;
	
	GLuint emptyTex;
	
	int PicForPos(int x, int y);
	
	int menuOffset;
	
	GUIbutton *left;
	GUIbutton *right;
	
	string Tooltip();

	Functor1<UnitDef*> clicked;
};

#endif // !defined(GUIBUILDMENU_H)
