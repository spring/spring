#ifndef GUIHANDLER_H
#define GUIHANDLER_H
// GuiHandler.h: interface for the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <vector>
#include "Game/command.h"
#include "Rendering/GL/myGL.h"
#include <map>
#include "KeySet.h"
#include "KeyBindings.h"
#include "InputReceiver.h"

class CglList;
struct UnitDef;
struct BuildInfo;

class CGuiHandler : public CInputReceiver
{
public:
	bool AboveGui(int x,int y);
	int IconAtPos(int x,int y);

	bool MousePress(int x,int y,int button);
	void MouseRelease(int x,int y,int button);

	void Update();
	void Draw();
	void DrawButtons();

	CGuiHandler();
	virtual ~CGuiHandler();

	
	bool LoadCMDBitmap(int id, char* filename);
	void LayoutIcons();

	vector<CommandDescription> commands;

	int inCommand;
	bool needShift;
	bool showingMetal;
	bool autoShowMetal;
	bool activeMousePress;
	int activePage;
	int maxPages;
	int defaultCmdMemory;
	CglList* list;
	int buildSpacing;

	int buildFacing; // which side the built buildings should face?

	void CreateOptions(Command& c,bool rmb);
	int GetDefaultCommand(int x,int y);
	void DrawMapStuff(void);
	void DrawFront(int button,float maxSize,float sizeDiv);
	bool KeyPressed(unsigned short key);
	void MenuChoice(string s);
	void FinishCommand(int button);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);
	std::string GetBuildTooltip() const;
	void DrawArea(float3 pos, float radius);

private:
	void SetCursorIcon();
	void SetShowingMetal(bool show);
	bool ProcessLocalActions(const CKeyBindings::Action& action);
	bool ProcessBuildActions(const CKeyBindings::Action& action);
	
private:
	int actionOffset;
	CKeySet lastKeySet;
	
	static const int NUMICOPAGE = 16;
	
public:
	Command GetOrderPreview(void);
	Command GetCommand(int mousex, int mousey, int buttonHint, bool preview);
	std::vector<BuildInfo> GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo); // start.def has to be end.def
};
extern CGuiHandler* guihandler;

#endif /* GUIHANDLER_H */
