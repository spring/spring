/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_HANDLER_H
#define GUI_HANDLER_H

#include <vector>
#include <map>
#include <set>
#include "KeySet.h"
#include "InputReceiver.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Sim/Units/CommandAI/Command.h"

#define DEFAULT_GUI_CONFIG "ctrlpanel.txt"

class CUnit;
struct UnitDef;
struct BuildInfo;
class Action;
struct SCommandDescription;

/**
 * The C and part of the V in MVC (Model-View-Controller).
 */
class CGuiHandler : public CInputReceiver {
public:
	CGuiHandler();
	virtual ~CGuiHandler();

	void Update();

	void Draw();
	void DrawMapStuff(bool onMinimap);
	void DrawCentroidCursor();

	bool AboveGui(int x, int y);
	bool KeyPressed(int key, bool isRepeat);
	bool KeyReleased(int key);
	bool MousePress(int x, int y, int button);
	void MouseRelease(int x, int y, int button)
	{
		// We can not use default params for this,
		// because they get initialized at compile-time,
		// where camera and mouse are still undefined.
		MouseRelease(x, y, button, camera->GetPos(), ::mouse->dir);
	}
	void MouseRelease(int x, int y, int button, const float3& cameraPos, const float3& mouseDir);
	bool IsAbove(int x, int y);
	std::string GetTooltip(int x, int y);
	std::string GetBuildTooltip() const;

	Command GetOrderPreview();
	Command GetCommand(int mouseX, int mouseY, int buttonHint, bool preview)
	{
		// We can not use default params for this,
		// because they get initialized at compile-time,
		// where camera and mouse are still undefined.
		return GetCommand(mouseX, mouseY, buttonHint, preview, camera->GetPos(), ::mouse->dir);
	}
	Command GetCommand(int mouseX, int mouseY, int buttonHint, bool preview, const float3& cameraPos, const float3& mouseDir);
	/// startInfo.def has to be endInfo.def
	std::vector<BuildInfo> GetBuildPos(const BuildInfo& startInfo, const BuildInfo& endInfo, const float3& cameraPos, const float3& mouseDir);

	bool EnableLuaUI(bool);
	bool DisableLuaUI();

	bool LoadConfig(const std::string& cfg);
	bool LoadDefaultConfig() { return (LoadConfig(DEFAULT_GUI_CONFIG)); }
	bool ReloadConfigFromFile(const std::string& fileName);
	bool ReloadConfigFromString(const std::string& cfg);

	void ForceLayoutUpdate() { forceLayoutUpdate = true; }

	int GetMaxPage()    const { return maxPage; }
	int GetActivePage() const { return activePage; }

	void RunLayoutCommand(const std::string& command);
	void RunCustomCommands(const std::vector<std::string>& cmds, bool rightMouseButton);

	bool GetInvertQueueKey() const { return invertQueueKey; }
	void SetInvertQueueKey(bool value) { invertQueueKey = value; }
	bool GetQueueKeystate() const;

	bool GetGatherMode() const { return gatherMode; }
	void SetGatherMode(bool value) { gatherMode = value; }

	bool GetOutlineFonts() const { return outlineFonts; }

	int  GetDefaultCommand(int x, int y) const
	{
		// We can not use default params for this,
		// because they get initialized at compile-time,
		// where camera and mouse are still undefined.
		return GetDefaultCommand(x, y, camera->GetPos(), ::mouse->dir);
	}
	int  GetDefaultCommand(int x, int y, const float3& cameraPos, const float3& mouseDir) const;

	bool SetActiveCommand(int cmdIndex, bool rightMouseButton);
	bool SetActiveCommand(int cmdIndex, int button, bool leftMouseButton, bool rightMouseButton, bool alt, bool ctrl, bool meta, bool shift);
	bool SetActiveCommand(const Action& action, const CKeySet& ks, int actionIndex);

	void SetDrawSelectionInfo(bool dsi) { drawSelectionInfo = dsi; }
	bool GetDrawSelectionInfo() const { return drawSelectionInfo; }

	void SetBuildFacing(unsigned int facing);
	void SetBuildSpacing(int spacing);

	void LayoutIcons(bool useSelectionPage);

public:
	std::vector<SCommandDescription> commands;
	int inCommand;
	int buildFacing;
	int buildSpacing;

private:
	void GiveCommand(Command& cmd, bool fromUser = true);
	void GiveCommandsNow();
	bool LayoutCustomIcons(bool useSelectionPage);
	void ResizeIconArray(unsigned int size);
	void AppendPrevAndNext(std::vector<SCommandDescription>& cmds);
	void ConvertCommands(std::vector<SCommandDescription>& cmds);

	int  FindInCommandPage();
	void RevertToCmdDesc(const SCommandDescription& cmdDesc, bool defaultCommand, bool samePage);

	unsigned char CreateOptions(bool rightMouseButton);
	unsigned char CreateOptions(int button);
	void FinishCommand(int button);
	void SetShowingMetal(bool show);
	float GetNumberInput(const SCommandDescription& cmdDesc) const;

	void ProcessFrontPositions(float3& pos0, const float3& pos1);

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
	void DrawMiniMapMarker(const float3& cameraPos);
	void DrawFront(int button, float maxSize, float sizeDiv, bool onMinimap, const float3& cameraPos, const float3& mouseDir);
	void DrawArea(float3 pos, float radius, const float* color);
	void DrawSelectBox(const float3& start, const float3& end, const float3& cameraPos);
	void DrawSelectCircle(const float3& pos, float radius, const float* color);

	void DrawStencilCone(const float3& pos, float radius, float height);
	void DrawStencilRange(const float3& pos, float radius);


	int  IconAtPos(int x, int y);
	void SetCursorIcon() const;
	bool TryTarget(const SCommandDescription& cmdDesc) const;

	void LoadDefaults();
	void SanitizeConfig();
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
	std::vector<IconInfo> icons;
	unsigned int iconsSize;
	int iconsCount;

	std::map<std::string, unsigned int> textureMap; // fileName, glTextureID

	int failedSound;

	std::vector<std::string> layoutCommands;
	std::vector< std::pair<Command, bool> > commandsToGive;
};

extern CGuiHandler* guihandler;

#endif /* GUI_HANDLER_H */
