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
#include "InputReceiver.h"

class CglList;
struct UnitDef;

class CGuiHandler : public CInputReceiver
{
public:
	bool AboveGui(int x,int y);
	int IconAtPos(int x,int y);

	bool MousePress(int x,int y,int button);
	void MouseRelease(int x,int y,int button);

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

	void CreateOptions(Command& c,bool rmb);
	int GetDefaultCommand(int x,int y);
	void DrawMapStuff(void);
	void DrawFront(int button,float maxSize,float sizeDiv);
	bool KeyPressed(unsigned short key);
	void MenuChoice(string s);
	void FinishCommand(int button);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);
	void DrawArea(float3 pos, float radius);

private:
	static const int NUMICOPAGE = 16;
public:
	Command GetOrderPreview(void);
	Command GetCommand(int mousex, int mousey, int buttonHint, bool preview);
	std::vector<float3> GetBuildPos(float3 start, float3 end,UnitDef* unitdef);
};
extern CGuiHandler* guihandler;

#endif /* GUIHANDLER_H */
