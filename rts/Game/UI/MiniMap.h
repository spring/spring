/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <deque>
#include "InputReceiver.h"
#include "Rendering/GL/FBO.h"
#include "System/Color.h"
#include "System/float3.h"
#include "System/type2.h"


class CUnit;
namespace icon {
	class CIconData;
}


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
		void DrawForReal(bool use_geo = true, bool updateTex = false);
		void Update();

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

		int GetPosX()  const { return xpos; }
		int GetPosY()  const { return ypos; }
		int GetSizeX() const { return width; }
		int GetSizeY() const { return height; }
		float GetUnitSizeX() const { return unitSizeX; }
		float GetUnitSizeY() const { return unitSizeY; }

		void SetSlaveMode(bool value);
		bool GetSlaveMode() const { return slaveDrawMode; }

		bool UseUnitIcons() const { return useIcons; }
		bool UseSimpleColors() const { return simpleColors; }

		const unsigned char* GetMyTeamIconColor() const { return &myColor[0]; }
		const unsigned char* GetAllyTeamIconColor() const { return &allyColor[0]; }
		const unsigned char* GetEnemyTeamIconColor() const { return &enemyColor[0]; }

		void ApplyConstraintsMatrix() const;

	protected:
		void ParseGeometry(const std::string& geostr);
		void ToggleMaximized(bool maxspect);
		void SetMaximizedGeometry();

		void SelectUnits(int x, int y);
		void ProxyMousePress(int x, int y, int button);
		void ProxyMouseRelease(int x, int y, int button);

		bool RenderCachedTexture(const bool use_geo);
		void DrawBackground() const;
		void DrawUnitIcons() const;
		void DrawUnitRanges() const;
		void DrawWorldStuff() const;
		void DrawCameraFrustumAndMouseSelection();
		void SetClipPlanes(const bool lua) const;

		void DrawFrame();
		void DrawNotes();
		void DrawButtons();
		void DrawMinimizedButton();

		void DrawUnitHighlight(const CUnit* unit);
		void DrawCircle(const float3& pos, float radius) const;
		void DrawSquare(const float3& pos, float xsize, float zsize) const;
		const icon::CIconData* GetUnitIcon(const CUnit* unit, float& scale) const;

		void UpdateTextureCache();
		void ResizeTextureCache();

	protected:
		static void DrawSurfaceCircle(const float3& pos, float radius, unsigned int resolution);
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
		SColor myColor;
		SColor allyColor;
		SColor enemyColor;

		bool renderToTexture;
		FBO fbo;
		FBO fboResolve;
		bool multisampledFBO;
		GLuint minimapTex;
		int2 minimapTexSize;
		float minimapRefreshRate;

		GLuint buttonsTexture;
		GLuint circleLists; // 8 - 256 divs
		static const int circleListsCount = 6;

		struct Notification {
			float creationTime;
			float3 pos;
			float color[4];
		};
		std::deque<Notification> notes;
		
		CUnit* lastClicked;
};


extern CMiniMap* minimap;


#endif /* MINIMAP_H */
