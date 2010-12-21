/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUIHANDLER_H
#define GUIHANDLER_H

#include <vector>
#include <map>
#include <set>
#include "KeySet.h"
#include "InputReceiver.h"
#include "MouseHandler.h"
#include "Game/Camera.h"

class CUnit;
struct UnitDef;
struct BuildInfo;
class Action;
struct Command;
struct CommandDescription;

class CGuiHandler : public CInputReceiver {
public:
	CGuiHandler();
	virtual ~CGuiHandler();

	void Update();

	void Draw();
	void DrawMapStuff(int minimapLevel);
	void DrawCentroidCursor();

	bool AboveGui(int x,int y);
	bool KeyPressed(unsigned short key, bool isRepeat);
	bool KeyReleased(unsigned short key);
	bool MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button) {MouseRelease(x,y,button, ::camera->pos, ::mouse->dir);}
	void MouseRelease(int x, int y, int button, float3& camerapos, float3& mousedir);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);
	std::string GetBuildTooltip() const;

	Command GetOrderPreview();
	Command GetCommand(int mousex, int mousey, int buttonHint, bool preview, float3& camerapos=::camera->pos, float3& mousedir=::mouse->dir);
	std::vector<BuildInfo> GetBuildPos(const BuildInfo& startInfo,
		const BuildInfo& endInfo, float3& camerapos, float3& mousedir); // startInfo.def has to be endInfo.def

	bool ReloadConfig(const std::string& filename);

	void ForceLayoutUpdate() { forceLayoutUpdate = true; }

	int GetMaxPage()    const { return maxPage; }
	int GetActivePage() const { return activePage; }

	void RunLayoutCommand(const std::string& command);
	void RunCustomCommands(const std::vector<std::string>& cmds, bool rmb);

	bool GetInvertQueueKey() const { return invertQueueKey; }
	void SetInvertQueueKey(bool value) { invertQueueKey = value; }
	bool GetQueueKeystate() const;

	bool GetGatherMode() const { return gatherMode; }
	void SetGatherMode(bool value) { gatherMode = value; }

	bool GetOutlineFonts() const { return outlineFonts; }

	int  GetDefaultCommand(int x, int y, float3& camerapos=::camera->pos, float3& mousedir=::mouse->dir) const;

	bool SetActiveCommand(int cmdIndex, bool rmb);
	bool SetActiveCommand(int cmdIndex,
		int button, bool lmb, bool rmb,
		bool alt, bool ctrl, bool meta, bool shift);
	bool SetActiveCommand(const Action& action, const CKeySet& ks, int actionIndex);

	void SetDrawSelectionInfo(bool dsi) { drawSelectionInfo = dsi; }
	bool GetDrawSelectionInfo() const { return drawSelectionInfo; }

	void SetBuildFacing(int facing);
	void SetBuildSpacing(int spacing);

	void PushLayoutCommand(const std::string&, bool luacmd = true);
	void RunLayoutCommands();

public:
	std::vector<CommandDescription> commands;
	int inCommand;
	int buildFacing;
	int buildSpacing;

private:
	void GiveCommand(const Command& cmd, bool fromUser = true) const;
	void LayoutIcons(bool useSelectionPage);
	bool LayoutCustomIcons(bool useSelectionPage);
	void ResizeIconArray(unsigned int size);
	void AppendPrevAndNext(std::vector<CommandDescription>& cmds);
	void ConvertCommands(std::vector<CommandDescription>&);

	int  FindInCommandPage();
	void RevertToCmdDesc(const CommandDescription& cmdDesc, bool defaultCommand, bool samePage);

	void CreateOptions(Command& c,bool rmb);
	void FinishCommand(int button);
	void SetShowingMetal(bool show);
	float GetNumberInput(const CommandDescription& cmdDesc) const;

	void ProcessFrontPositions(float3& pos0, float3& pos1);

	struct IconInfo;

	void DrawButtons();
	void DrawCustomButton(const IconInfo& icon, bool highlight);
	bool DrawUnitBuildIcon(const IconInfo& icon, int unitDefID);
	bool DrawTexture(const IconInfo& icon, const std::string& texName);
	void DrawName(const IconInfo& icon, const std::string& text, bool offsetForLEDs);
	void DrawNWtext(const IconInfo& icon, const std::string& text);
	void DrawSWtext(const IconInfo& icon, const std::string& text);
	void DrawNEtext(const IconInfo& icon, const std::string& text);
	void DrawSEtext(const IconInfo& icon, const std::string& text);
	void DrawPrevArrow(const IconInfo& icon);
	void DrawNextArrow(const IconInfo& icon);
	void DrawHilightQuad(const IconInfo& icon);
	void DrawIconFrame(const IconInfo& icon);
	void DrawOptionLEDs(const IconInfo& icon);
	void DrawMenuName();
	void DrawSelectionInfo();
	void DrawNumberInput();
	void DrawMiniMapMarker(float3& camerapos);
	void DrawFront(int button, float maxSize, float sizeDiv, bool onMinimap, float3& camerapos, float3& mousedir);
	void DrawArea(float3 pos, float radius, const float* color);
	void DrawSelectBox(const float3& start, const float3& end, float3& camerapos);
	void DrawSelectCircle(const float3& pos, float radius, const float* color);

	void DrawStencilCone(const float3& pos, float radius, float height);
	void DrawStencilRange(const float3& pos, float radius);


	int  IconAtPos(int x,int y);
	void SetCursorIcon() const;

	void LoadDefaults();
	void SanitizeConfig();
	bool LoadConfig(const std::string& filename);
	void ParseFillOrder(const std::string& text);

	bool ProcessLocalActions(const Action& action);
	bool ProcessBuildActions(const Action& action);
	int  GetIconPosCommand(int slot) const;
	int  ParseIconSlot(const std::string& text) const;

private:
	bool needShift;
	bool showingMetal;
	bool autoShowMetal;
	bool invertQueueKey;
	bool activeMousePress;
	bool forceLayoutUpdate;
	int maxPage;
	int activePage;
	int defaultCmdMemory;
	int explicitCommand;
	int curIconCommand;

	int actionOffset;
	CKeySet lastKeySet;

	std::string menuName;
	int xIcons, yIcons;
	float xPos, yPos;
	float textBorder;
	float iconBorder;
	float frameBorder;
	float xIconSize, yIconSize;
	float xSelectionPos, ySelectionPos;
	int deadIconSlot;
	int prevPageSlot;
	int nextPageSlot;
	bool dropShadows;
	bool useOptionLEDs;
	bool selectGaps;
	bool selectThrough;
	bool outlineFonts;
	bool drawSelectionInfo;

	float frameAlpha;
	float textureAlpha;
	std::vector<int> fillOrder;

	bool gatherMode;
	bool miniMapMarker;
	bool newAttackMode;
	bool attackRect;
	bool invColorSelect;
	bool frontByEnds;

	bool useStencil;

	int iconsPerPage;
	float xIconStep, yIconStep;
	float xBpos, yBpos; // center of the buildIconsFirst indicator

	struct Box {
		GLfloat x1;
		GLfloat y1;
		GLfloat x2;
		GLfloat y2;
	};
	Box buttonBox;

	struct IconInfo {
		int commandsID; // index into commands list (or -1)
		Box visual;
		Box selection;
	};
	IconInfo* icons;
	unsigned int iconsSize;
	int iconsCount;

	std::map<std::string, unsigned int> textureMap; // filename, glTextureID

	int failedSound;

	std::vector<std::string> layoutCommands;
	bool hasLuaUILayoutCommands;
};


extern CGuiHandler* guihandler;


#endif /* GUIHANDLER_H */
