#ifndef MINIMAP_H
#define MINIMAP_H
// MiniMap.h: interface for the CMiniMap class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>
#include <string>
#include "InputReceiver.h"
#include "Rendering/GL/myGL.h"

class CUnit;
class CIconData;


class CMiniMap : public CInputReceiver {
	public:
		CMiniMap();
		virtual ~CMiniMap();

		bool MousePress(int x, int y, int button);
		void MouseMove(int x, int y, int dx,int dy, int button);
		void MouseRelease(int x, int y, int button);
		void MoveView(int x, int y);
		bool IsAbove(int x, int y);
		std::string GetTooltip(int x, int y);
		void Draw();
		void DrawForReal();
		
		void ConfigCommand(const std::string& command);

		float3 GetMapPosition(int x, int y) const;
		CUnit* GetSelectUnit(const float3& pos) const;
		
		void UpdateGeometry();
		void SetGeometry(int px, int py, int sx, int sy);

		void AddNotification(float3 pos, float3 color, float alpha);

		bool  FullProxy()   const { return fullProxy; }
		bool  ProxyMode()   const { return proxyMode; }
		float CursorScale() const { return cursorScale; }

		void SetMinimized(bool state) { minimized = state; }
		bool GetMinimized() const { return minimized; }

		bool GetMaximized() const { return maximized; }

		inline int GetPosX()  const { return xpos; }
		inline int GetPosY()  const { return ypos; }
		inline int GetSizeX() const { return width; }
		inline int GetSizeY() const { return height; }

		void SetSlaveMode(bool value);
		bool GetSlaveMode() const { return slaveDrawMode; }
		
	protected:
		void ParseGeometry(const std::string& geostr);
		void ToggleMaximized(bool maxspect);
		void SetMaximizedGeometry();
		
		void SelectUnits(int x, int y) const;
		void ProxyMousePress(int x, int y, int button);
		void ProxyMouseRelease(int x, int y, int button);
		
		void DrawNotes(void);
		void DrawButtons();
		void DrawMinimizedButton();
		void DrawUnit(CUnit* unit);
		void DrawUnitHighlight(CUnit* unit);
		void DrawCircle(const float3& pos, float radius);
		void DrawSquare(const float3& pos, float xsize, float zsize);
		const CIconData* GetUnitIcon(CUnit* unit, float& scale) const;
		void GetFrustumSide(float3& side);
		
	protected:
		static void DrawSurfaceCircle(const float3& pos, float radius,
		                              unsigned int resolution);
		static void DrawSurfaceSquare(const float3& pos, float xsize, float zsize);
	protected:
		int xpos, ypos;
		int height, width;
		int oldxpos, oldypos;
		int oldheight, oldwidth;

		float unitBaseSize;
		float unitExponent;

		float unitSizeX;
		float unitSizeY;
		float unitSelectRadius;
		
		bool fullProxy;

		bool proxyMode;
		bool selecting;
		bool maxspect;
		bool maximized;
		bool minimized;
		bool mouseLook;
		bool mouseMove;
		bool mouseResize;

		bool slaveDrawMode;

		struct IntBox {
			bool Inside(int x, int y) const {
				return ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax));
			}
			void DrawBox() const;
			void DrawTextureBox() const;
			int xmin, xmax, ymin, ymax;
			float xminTx, xmaxTx, yminTx, ymaxTx;  // texture coordinates
		};

		int buttonSize;
		bool showButtons;
		IntBox mapBox;
		IntBox buttonBox;
		IntBox moveBox;
		IntBox resizeBox;
		IntBox minimizeBox;
		IntBox maximizeBox;
		int lastWindowSizeX;
		int lastWindowSizeY;
		
		bool drawProjectiles;
		bool useIcons;
		int drawCommands;
		float cursorScale;
		
		bool simpleColors;
		unsigned char myColor[4];
		unsigned char allyColor[4];
		unsigned char enemyColor[4];

		GLuint buttonsTexture;
		unsigned int circleLists; // 8 - 256 divs
		static const int circleListsCount = 6;
				
		struct fline {
			float base;
			float dir;
			int left;
			float minz;
			float maxz;
		};
		std::vector<fline> left;

		struct Notification {
			float creationTime;
			float3 pos;
			float color[4];
		};
		std::list<Notification> notes;
};


extern CMiniMap* minimap;


#endif /* MINIMAP_H */
