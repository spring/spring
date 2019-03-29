/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_HANDLER_H
#define GUI_HANDLER_H

#include <vector>

#include "KeySet.h"
#include "InputReceiver.h"
#include "MouseHandler.h"
#include "Game/Camera.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "Rendering/GL/WideLineAdapterFwd.hpp"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/CommandAI/Command.h"

#define DEFAULT_GUI_CONFIG "ctrlpanel.txt"

class CUnit;
struct UnitDef;

class Action;
struct SCommandDescription;

/**
 * The C and part of the V in MVC (Model-View-Controller).
 */
class CGuiHandler : public CInputReceiver {
public:
	CGuiHandler();

	void Update();

	void Draw();
	void DrawMapStuff(bool onMiniMap);
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
		MouseRelease(x, y, button, camera->GetPos(), mouse->dir);
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
		return GetCommand(mouseX, mouseY, buttonHint, preview, camera->GetPos(), mouse->dir);
	}
	Command GetCommand(int mouseX, int mouseY, int buttonHint, bool preview, const float3& cameraPos, const float3& mouseDir);

	/// startInfo.def has to be endInfo.def
	size_t GetBuildPositions(const BuildInfo& startInfo, const BuildInfo& endInfo, const float3& cameraPos, const float3& mouseDir);

	bool EnableLuaUI(bool enableCommand);
	bool DisableLuaUI(bool layoutIcons = true);

	bool LoadConfig(const std::string& cfg);
	bool LoadDefaultConfig() { return (LoadConfig(DEFAULT_GUI_CONFIG)); }
	bool ReloadConfigFromFile(const std::string& fileName);
	bool ReloadConfigFromString(const std::string& cfg);

	void ForceLayoutUpdate() { forceLayoutUpdate = true; }

	int GetMaxPage()    const { return maxPage; }
	int GetActivePage() const { return activePage; }

	void RunCustomCommands(const std::vector<std::string>& cmds, bool rightMouseButton);

	bool GetInvertQueueKey() const { return invertQueueKey; }
	void SetInvertQueueKey(bool value) { invertQueueKey = value; }
	bool GetQueueKeystate() const;

	bool GetGatherMode() const { return gatherMode; }
	void SetGatherMode(bool value) { gatherMode = value; }

	bool GetOutlineFonts() const { return outlineFonts; }

	int  GetDefaultCommand(int x, int y) const { return GetDefaultCommand(x, y, camera->GetPos(), ::mouse->dir); }
	int  GetDefaultCommand(int x, int y, const float3& cameraPos, const float3& mouseDir) const;

	bool SetActiveCommand(int cmdIndex, bool rightMouseButton);
	bool SetActiveCommand(int cmdIndex, int button, bool leftMouseButton, bool rightMouseButton, bool alt, bool ctrl, bool meta, bool shift);
	bool SetActiveCommand(const Action& action, const CKeySet& ks, int actionIndex);

	void SetDrawSelectionInfo(bool dsi) { drawSelectionInfo = dsi; }
	bool GetDrawSelectionInfo() const { return drawSelectionInfo; }

	void SetBuildFacing(unsigned int facing) { buildFacing = facing % NUM_FACINGS; }
	void SetBuildSpacing(int spacing) { buildSpacing = std::max(spacing, 0); }

	void LayoutIcons(bool useSelectionPage);

private:
	void GiveCommand(const Command& cmd, bool fromUser = true);
	void GiveCommandsNow();
	bool LayoutCustomIcons(bool useSelectionPage);
	void ResizeIconArray(size_t size);
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
	struct Box;

	void DrawMenu();
	bool DrawMenuUnitBuildIcon(const Box& iconBox, GL::RenderDataBufferTC* rdBuffer, int unitDefID) const;
	bool DrawMenuIconCustomTexture(const Box& iconBox, const SCommandDescription& cmdDesc, GL::RenderDataBufferTC* rdBuffer) const;
	bool DrawMenuIconTexture(const Box& iconBox, const std::string& texName, GL::RenderDataBufferTC* rdBuffer) const;
	void DrawMenuIconName(const Box& iconBox, const std::string& text, bool offsetForLEDs) const;

	void DrawMenuIconNWtext(const Box& iconBox, const std::string& text) const;
	void DrawMenuIconSWtext(const Box& iconBox, const std::string& text) const;
	void DrawMenuIconNEtext(const Box& iconBox, const std::string& text) const;
	void DrawMenuIconSEtext(const Box& iconBox, const std::string& text) const;

	void DrawMenuPrevArrow(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer) const;
	void DrawMenuNextArrow(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer) const;

	void DrawMenuIconFrame(const Box& iconBox, const SColor& color, GL::RenderDataBufferC* rdBuffer, float fudge = 0.001f) const;
	void DrawMenuIconOptionLEDs(const Box& iconBox, const SCommandDescription& cmdDesc, GL::RenderDataBufferC* rdBuffer, bool outline) const;
	void DrawMenuName(GL::RenderDataBufferC* rdBuffer) const;

	void DrawMenuSelectionInfo(GL::RenderDataBufferC* rdBuffer) const;
	void DrawMenuNumberInput(GL::RenderDataBufferC* rdBuffer) const;


	void DrawMiniMapMarker(const float3& cameraPos);
	void DrawFormationFrontOrder(
		GL::RenderDataBufferC* buffer,
		GL::WideLineAdapterC* wla,
		Shader::IProgramObject* shader,
		const float3& cameraPos,
		const float3& mouseDir,
		int button,
		float maxSize,
		float sizeDiv,
		bool onMiniMap
	);
	void DrawSelectBox(GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject* ipo, const float3& start, const float3& end);
	void DrawSelectCircle(GL::RenderDataBufferC* rdb, GL::WideLineAdapterC* wla, Shader::IProgramObject* ipo, const float4& pos, const float* color);


	int  IconAtPos(int x, int y);
	void SetCursorIcon() const;
	bool TryTarget(const SCommandDescription& cmdDesc) const;

	void LoadDefaults() {}
	void SanitizeConfig();
	void ParseFillOrder(const std::string& text);

	bool ProcessLocalActions(const Action& action);
	bool ProcessBuildActions(const Action& action);
	int  GetIconPosCommand(int slot) const;
	int  ParseIconSlot(const std::string& text) const;


public:
	int inCommand = -1;
	int buildFacing = FACING_SOUTH;
	int buildSpacing = 0;

private:
	int maxPage = 0;
	int activePage = 0;
	int defaultCmdMemory = -1;
	int explicitCommand = -1;
	int curIconCommand = -1;

	int actionOffset = 0;

	int deadIconSlot = -1;
	int prevPageSlot = -1;
	int nextPageSlot = -1;

	int xIcons = 2;
	int yIcons = 8;
	// number of slots taken up in <icons>
	int iconsCount = 0;
	int iconsPerPage = 0;

	int failedSound = -1;


	float xPos = 0.000f;
	float yPos = 0.175f;
	float textBorder = 0.003f;
	float iconBorder = 0.003f;
	float frameBorder = 0.003f;
	float xIconSize = 0.060f;
	float yIconSize = 0.060f;
	float xSelectionPos = 0.018f;
	float ySelectionPos = 0.127f;

	float xIconStep = 0.0f;
	float yIconStep = 0.0f;
	float xBpos = 0.0f;
	float yBpos = 0.0f; // center of the buildIconsFirst indicator

	float frameAlpha = -1.0f;
	float textureAlpha = 0.8f;


	bool needShift = false;
	bool showingMetal = false;
	bool invertQueueKey = false;
	bool activeMousePress = false;
	bool forceLayoutUpdate = false;

	bool dropShadows = true;
	bool useOptionLEDs = true;
	bool selectGaps = true;
	bool selectThrough = false;
	bool outlineFonts = false;
	bool drawSelectionInfo = true;

	bool gatherMode = false;
	bool miniMapMarker = true;
	bool newAttackMode = true;
	bool attackRect = false;
	bool invColorSelect = true;
	bool frontByEnds = false;

	struct Box {
		float x1 = 0.0f;
		float y1 = 0.0f;
		float x2 = 0.0f;
		float y2 = 0.0f;
	};

	struct IconInfo {
		int commandsID; // index into commands list (or -1)
		Box visual;
		Box selection;
	};

	Box buttonBox;
	CKeySet lastKeySet;

	std::string menuName;

	std::vector<int> iconFillOrder;
	std::vector<IconInfo> icons;

	std::vector<std::string> layoutCommands;
	std::vector< std::pair<Command, bool> > commandsToGive;

	// DrawMapStuff caches
	std::vector<BuildInfo> buildInfoQueue;
	std::vector<Command> buildCommands;
	std::vector<float4> buildColors;

	// DrawMenuButtons caches
	std::vector<uint8_t> menuIconDrawFlags;
	std::vector<int32_t> menuIconIndices;

public:
	std::vector<SCommandDescription> commands;
};

extern CGuiHandler* guihandler;

#endif /* GUI_HANDLER_H */
