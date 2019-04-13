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
	void DrawForReal(bool useNormalizedCoors = true, bool updateTex = false, bool luaCall = false);
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

	int GetPosX()  const { return curPos.x; }
	int GetPosY()  const { return curPos.y; }
	int GetSizeX() const { return curDim.x; }
	int GetSizeY() const { return curDim.y; }
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

	bool RenderCachedTexture(bool useGeom);
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
	int2 curPos;
	int2 curDim;
	int2 tmpPos;
	int2 oldPos;
	int2 oldDim;

	float minimapRefreshRate = 0.0f;

	float unitBaseSize = 0.0f;
	float unitExponent = 0.0f;

	float unitSizeX = 0.0f;
	float unitSizeY = 0.0f;
	float unitSelectRadius = 0.0f;

	bool fullProxy = false;
	bool proxyMode = false;
	bool selecting = false;
	bool maxspect = false;
	bool maximized = false;
	bool minimized = false;
	bool mouseEvents = true; // if false, MousePress is not handled
	bool mouseLook = false;
	bool mouseMove = false;
	bool mouseResize = false;

	bool slaveDrawMode = false;
	bool simpleColors = false;

	bool showButtons = false;
	bool drawProjectiles = false;
	bool useIcons = true;

	bool renderToTexture = true;
	bool multisampledFBO = false;

	struct IntBox {
		bool Inside(int x, int y) const {
			return ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax));
		}
		void DrawBox() const;
		void DrawTextureBox() const;
		int xmin, xmax, ymin, ymax;
		float xminTx, xmaxTx, yminTx, ymaxTx;  // texture coordinates
	};

	int lastWindowSizeX = 0;
	int lastWindowSizeY = 0;

	int buttonSize = 0;

	int drawCommands = 0;
	float cursorScale = 0.0f;

	IntBox mapBox;
	IntBox buttonBox;
	IntBox moveBox;
	IntBox resizeBox;
	IntBox minimizeBox;
	IntBox maximizeBox;

	SColor myColor;
	SColor allyColor;
	SColor enemyColor;

	FBO fbo;
	FBO fboResolve;
	GLuint minimapTex = 0;
	int2 minimapTexSize;

	GLuint buttonsTexture;
	GLuint circleLists; // 8 - 256 divs
	static constexpr int circleListsCount = 6;

	struct Notification {
		float creationTime;
		float3 pos;
		float color[4];
	};
	std::deque<Notification> notes;

	CUnit* lastClicked = nullptr;
};


extern CMiniMap* minimap;


#endif /* MINIMAP_H */
