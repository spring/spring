#ifndef GUIHANDLER_H
#define GUIHANDLER_H
// GuiHandler.h: interface for the CGuiHandler class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <vector>
#include <map>
#include "Game/command.h"
#include "Rendering/GL/myGL.h"
#include "KeySet.h"
#include "KeyBindings.h"
#include "InputReceiver.h"

class CUnit;
class CglList;
class CIconLayoutHandler;
struct UnitDef;
struct BuildInfo;

class CGuiHandler : public CInputReceiver {
	public:
		CGuiHandler();
		virtual ~CGuiHandler();

		void Update();

		void Draw();
		void DrawMapStuff(void);
		void DrawCentroidCursor(void);
		
		bool AboveGui(int x,int y);
		bool KeyPressed(unsigned short key);
		bool MousePress(int x, int y, int button);
		void MouseMove(int x, int y, int dx, int dy, int button);
		void MouseRelease(int x, int y, int button);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x, int y);
		std::string GetBuildTooltip() const;

		Command GetOrderPreview(void);
		Command GetCommand(int mousex, int mousey, int buttonHint, bool preview);
		std::vector<BuildInfo> GetBuildPos(const BuildInfo& startInfo,
		                                   const BuildInfo& endInfo);
		                                   // start.def has to be end.def

		bool ReloadConfig(const string& filename);
		
		int GetMaxPage()    const { return maxPage; }
		int GetActivePage() const { return activePage; }
		
		void RunLayoutCommand(const string& command);
		void RunCustomCommands(const vector<string>& cmds, bool rmb);

 		bool GetInvertQueueKey() const { return invertQueueKey; }
 		void SetInvertQueueKey(bool value) { invertQueueKey = value; }
 		bool GetQueueKeystate() const;
 		
 		bool GetGatherMode() const { return gatherMode; }
 		void SetGatherMode(bool value) { gatherMode = value; }
 		
		bool BindNamedTexture(const std::string& texName);
		
		void UnitCreated(CUnit* unit);
		void UnitReady(CUnit* unit, CUnit* builder);
		void UnitDestroyed(CUnit* victim, CUnit* attacker);
		void AddConsoleLine(const std::string& line, int priority);
		
	public:
		vector<CommandDescription> commands;
		int inCommand;
		int buildFacing;
		int buildSpacing;

	private:
		void GiveCommand(const Command& cmd, bool fromUser = true) const;

		void MenuChoice(string s);
		static void MenuSelection(std::string s);
		
		void LayoutIcons(bool useSelectionPage);
		bool LayoutCustomIcons(bool useSelectionPage);
		void ResizeIconArray(unsigned int size);
		void AppendPrevAndNext(vector<CommandDescription>& cmds);
 		void ConvertCommands(vector<CommandDescription>&);

		int  FindInCommandPage();
		void RevertToCmdDesc(const CommandDescription& cmdDesc, bool samePage);

		int  GetDefaultCommand(int x,int y) const;
		void CreateOptions(Command& c,bool rmb);
		void FinishCommand(int button);
		void SetShowingMetal(bool show);
		float GetNumberInput(const CommandDescription& cmdDesc) const;
		
		struct IconInfo;

		void DrawButtons();
		void DrawCustomButton(const IconInfo& icon, bool highlight);
		bool DrawUnitBuildIcon(const IconInfo& icon, int unitDefID);
		bool DrawTexture(const IconInfo& icon, const std::string& texName);
		void DrawName(const IconInfo& icon, const std::string& text,
		              bool offsetForLEDs);
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
		void DrawMiniMapMarker();
		void DrawFront(int button,float maxSize,float sizeDiv);
		void DrawArea(float3 pos, float radius, const float* color);
		void DrawSelectBox(const float3& start, const float3& end);
		void DrawSelectCircle(const float3& pos, float radius,
		                      const float* color);

		void DrawStencilCone(const float3& pos, float radius, float height);
		void DrawStencilRange(const float3& pos, float radius);


		int  IconAtPos(int x,int y);
		void SetCursorIcon() const;

		void LoadDefaults();
		void SanitizeConfig();
		bool LoadConfig(const std::string& filename);
		void ParseFillOrder(const std::string& text);

		bool ProcessLocalActions(const CKeyBindings::Action& action);
		bool ProcessBuildActions(const CKeyBindings::Action& action);
		int  GetIconPosCommand(int slot) const;
		int  ParseIconSlot(const std::string& text) const;
		
	private:
		bool firstLayout;
		bool needShift;
		bool showingMetal;
		bool autoShowMetal;
		bool invertQueueKey;
		bool activeMousePress;
		bool forceLayoutUpdate;
		int maxPage;
		int activePage;
		int defaultCmdMemory;
		int fadein;
		CglList* list;
		
		int actionOffset;
		CKeySet lastKeySet;

		CIconLayoutHandler* layoutHandler;
		bool layoutHandlerClick;		

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
		float frameAlpha;
		float textureAlpha;
		std::vector<int> fillOrder;

		bool gatherMode;
		bool miniMapMarker;
		bool newAttackMode;
		bool attackRect;
		bool invColorSelect;

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
		int iconsSize;
		int iconsCount;
		int activeIcons;
		
		std::map<std::string, unsigned int> textureMap; // filename, glTextureID
		
		static const char* luaLayoutFile;
};


extern CGuiHandler* guihandler;


#endif /* GUIHANDLER_H */
