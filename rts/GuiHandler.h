// GuiHandler.h: interface for the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIHANDLER_H__E513A229_42ED_4319_85F2_898CECF847A5__INCLUDED_)
#define AFX_GUIHANDLER_H__E513A229_42ED_4319_85F2_898CECF847A5__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "command.h"
#include "mygl.h"
#include <map>
#include "inputreceiver.h"

using namespace std;
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
	bool activeMousePress;
	int activePage;
	int maxPages;
	int defaultCmdMemory;
	CglList* list;


	void CreateOptions(Command& c,bool rmb);
	int GetDefaultCommand(int x,int y);
	void DrawMapStuff(void);
	void DrawFront(int button,float maxSize,float sizeDiv);
	bool KeyPressed(unsigned char key);
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
	void MakeBuildPos(float3& pos,UnitDef* unitdef);
};
extern CGuiHandler* guihandler;

#endif // !defined(AFX_GUIHANDLER_H__E513A229_42ED_4319_85F2_898CECF847A5__INCLUDED_)

