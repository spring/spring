#if !defined(GUIGAME_H)
#define GUIGAME_H

#include "GUIpane.h"
#include "Game/command.h"
#include "float3.h"

#define numButtons 6

class GUIcontroller;

class CUnit;
class CFeature;
struct UnitDef;

class Selector
{
public:
	Selector() {isDragging=false;}
	virtual ~Selector();

	bool BeginSelection(const float3& dir);
	bool UpdateSelection(const float3& dir);
	void GetSelection(std::vector<CUnit*>&selected);
	void EndSelection();

	// assumes game space projection
	void DrawSelection();
	void DrawArea();
	void DrawFront(float maxSize, float sizeDiv);

	// returns true if BeginSelection was called and EndSelection wasn't
	bool IsDragging() { return isDragging; }
	
	string& DragCommand() { return dragCommand; };
	void SetDragCommand(const string& command) { dragCommand = command; };
	
	float3& Start() { return begin; };
	float3& End() { return end; };

protected:
	bool isDragging;

	float3 begin;
	float3 end;	
	
	string dragCommand;
};

class GUIgame : public GUIpane
{
public:
	GUIgame(GUIcontroller *controller);
	GUIgame(int x, int y, int w, int h, GUIcontroller *controller);
	virtual ~GUIgame();
	
	void StartCommand(CommandDescription &cmd);
	void StopCommand();
	
	void SelectCursor();
	
	bool DirectCommand(const float3& position, int button);
protected:
	bool MouseDownAction(int x, int y, int button);
	bool MouseUpAction(int x, int y, int button);
	bool MouseMoveAction(int x, int y, int xrel, int yrel, int buttons);
	bool EventAction(const std::string& command);

	void PrivateDraw();
	
	Command GetCommand();
	
	bool MouseEvent(const std::string& event, int x, int y, int button);

	CommandDescription *currentCommand;
	
	bool IsDragCommand(CommandDescription *cmd);

	struct mu
	{
		int x, y;
		float3 dir;
	} down[numButtons];

	Selector selector[numButtons];

	int buttonAction[numButtons];

	int commandButton;	// clicks of command button select or give a command
	
	bool mouseLook;
	
	bool showingMetalMap;

	GUIcontroller *controller;
	
	void InitCommand(Command& c);
	void DispatchCommand(Command c);
	
	
	std::vector<float3> GetBuildPos(float3 start, float3 end,UnitDef* unitdef);
	void MakeBuildPos(float3& pos,UnitDef* unitdef);
	
	friend class GUIcontroller;
	
	CUnit *unit;
	CFeature *feature;
	float3 position;
	
	string Tooltip();
	
	std::string tooltip;
		
	bool temporaryCommand;
	bool drawTooltip;
};

extern GUIgame *guiGameControl;

#endif // !defined(GUIGAME_H)
