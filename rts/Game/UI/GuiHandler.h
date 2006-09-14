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

class CglList;
struct UnitDef;
struct BuildInfo;

class CGuiHandler : public CInputReceiver {
	public:
		CGuiHandler();
		virtual ~CGuiHandler();

		void Update();

		void Draw();
		void DrawMapStuff(void);
		
		bool AboveGui(int x,int y);
		bool KeyPressed(unsigned short key);
		bool MousePress(int x,int y,int button);
		void MouseRelease(int x,int y,int button);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x, int y);
		std::string GetBuildTooltip() const;

		Command GetOrderPreview(void);
		Command GetCommand(int mousex, int mousey, int buttonHint, bool preview);
		std::vector<BuildInfo> GetBuildPos(const BuildInfo& startInfo,
		                                   const BuildInfo& endInfo);
		                                   // start.def has to be end.def
		bool ReloadConfig();
		
	public:
		vector<CommandDescription> commands;
		int inCommand;
		int buildFacing;
		int buildSpacing;

	private:
		void MenuChoice(string s);
		static void MenuSelection(std::string s);
		
		void LayoutIcons();
		bool LoadCMDBitmap(int id, char* filename);

		int  GetDefaultCommand(int x,int y) const;
		void CreateOptions(Command& c,bool rmb);
		void FinishCommand(int button);
		void SetShowingMetal(bool show);

		void DrawButtons();
		void DrawFront(int button,float maxSize,float sizeDiv);
		void DrawArea(float3 pos, float radius);

		int  IconAtPos(int x,int y);
		void SetCursorIcon() const;

		void LoadDefaults();
		void SanitizeConfig();
		bool LoadConfig(const std::string& filename);

		void ResetInCommand(const CommandDescription& cmdDesc);
		bool ProcessLocalActions(const CKeyBindings::Action& action);
		bool ProcessBuildActions(const CKeyBindings::Action& action);
		int  GetIconPosCommand(int slot) const;
		int  ParseIconPosSlot(const std::string& text) const;
		
	private:
		bool needShift;
		bool showingMetal;
		bool autoShowMetal;
		bool activeMousePress;
		int maxPage;
		int activePage;
		int defaultCmdMemory;
		int fadein;
		CglList* list;
		
		int actionOffset;
		CKeySet lastKeySet;

		int xIcons, yIcons;
		float xPos, yPos;
		float iconBorder;
		float frameBorder;
		float xIconSize, yIconSize;
		float xIconStep, yIconStep;
		float xSelectionPos, ySelectionPos;
		int deadIconSlot;
		int prevPageSlot;
		int nextPageSlot;
		bool noSelectGaps;
		std::vector<int> fillOrder;

		int iconsPerPage;
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
		
		struct GuiIconData {
			unsigned int x;
			unsigned int y;
			GLfloat width;
			GLfloat height;
			bool has_bitmap;
			unsigned int texture;
			char* c_str;
		};
		std::map<int, GuiIconData> iconMap;
};


extern CGuiHandler* guihandler;


#endif /* GUIHANDLER_H */
