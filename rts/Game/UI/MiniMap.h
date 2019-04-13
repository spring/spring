/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MINIMAP_H
#define MINIMAP_H

#include <string>
#include <deque>

#include "InputReceiver.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/GL/WideLineAdapterFwd.hpp"
#include "System/Color.h"
#include "System/float4.h"
#include "System/type2.h"
#include "System/Matrix44f.h"


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
	void DrawForReal();
	void Update();

	void RenderCachedTextureRaw();
	void RenderCachedTextureNormalized(bool luaCall);
	void RenderCachedTextureImpl(const CMatrix44f& viewMat, const CMatrix44f& projMat);

	void RenderCameraFrustumLinesAndSelectionBox(GL::WideLineAdapterC* wla);
	void RenderMarkerNotificationRectangles(GL::RenderDataBufferC* buffer);


	void ConfigCommand(const std::string& command);

	CMatrix44f CalcNormalizedCoorMatrix(bool dualScreen, bool luaCall) const;
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

	const CMatrix44f& GetViewMat(unsigned int idx) const { return viewMats[idx]; }
	const CMatrix44f& GetProjMat(unsigned int idx) const { return projMats[idx]; }

protected:
	void LoadBuffer();
	void ParseGeometry(const std::string& geostr);
	void ToggleMaximized(bool maxspect);
	void SetMaximizedGeometry();

	void SelectUnits(int x, int y);
	void ProxyMousePress(int x, int y, int button);
	void ProxyMouseRelease(int x, int y, int button);

	void DrawBackground();
	void DrawUnitIcons() const;
	void DrawUnitRanges() const;
	void DrawWorldStuff() const;

	void DrawFrame(GL::RenderDataBufferC* buffer);
	bool DrawButtonQuads(GL::RenderDataBufferTC* buffer);
	bool DrawButtonLoops(GL::RenderDataBufferC* buffer);
	bool DrawButtons(GL::RenderDataBufferTC* tcbuffer, GL::RenderDataBufferC* cbuffer) {
		return (DrawButtonQuads(tcbuffer) && DrawButtonLoops(cbuffer));
	}

	void DrawMinimizedButtonQuad(GL::RenderDataBufferTC* buffer);
	void DrawMinimizedButtonLoop(GL::RenderDataBufferC* buffer);

	void DrawUnitHighlight(const CUnit* unit);
	void DrawCircle(GL::RenderDataBufferC* buffer, const float4& pos, const float4& color) const;
	void DrawCircleW(GL::WideLineAdapterC* buffer, const float4& pos, const float4& color) const;

	const icon::CIconData* GetUnitIcon(const CUnit* unit, float& scale) const;

	void UpdateTextureCache();
	void ResizeTextureCache();

protected:
	static void DrawSurfaceCircleFunc(GL::RenderDataBufferC* buffer, const float4& pos, const float4& color, unsigned int);
	static void DrawSurfaceCircleWFunc(GL::WideLineAdapterC* wla, const float4& pos, const float4& color, unsigned int);

protected:
	int2 curPos;
	int2 curDim;
	int2 tmpPos;
	int2 oldPos;
	int2 oldDim;

	int2 minimapTexSize;


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

	bool multisampledFBO = false;


	struct IntBox {
		bool Inside(int x, int y) const {
			return ((x >= xmin) && (x <= xmax) && (y >= ymin) && (y <= ymax));
		}

		void DrawBox(GL::RenderDataBufferC* buffer) const;
		void DrawTextureBox(GL::RenderDataBufferTC* buffer) const;

		int xmin, xmax;
		int ymin, ymax;

		// texture coordinates
		float xminTx, xmaxTx;
		float yminTx, ymaxTx;

		SColor color;
	};

	IntBox mapBox;
	IntBox buttonBox;
	IntBox moveBox;
	IntBox resizeBox;
	IntBox minimizeBox;
	IntBox maximizeBox;

	int lastWindowSizeX = 0;
	int lastWindowSizeY = 0;

	int buttonSize = 0;

	int drawCommands = 0;
	float cursorScale = 0.0f;

	SColor myColor = {0.2f, 0.9f, 0.2f, 1.0f};
	SColor allyColor = {0.3f, 0.3f, 0.9f, 1.0f};
	SColor enemyColor = {0.9f, 0.2f, 0.2f, 1.0f};

	// transforms for [0] := Draw, [1] := DrawInMiniMap, [2] := Lua DrawInMiniMap
	CMatrix44f viewMats[3];
	CMatrix44f projMats[3];

	FBO fbo;
	FBO fboResolve;

	GL::RenderDataBuffer miniMap;


	GLuint minimapTextureID = 0;
	GLuint buttonsTextureID = 0;

	struct MiniMapVertType {
		float2 xy;
		float2 tc;
	};

	struct Notification {
		float creationTime;
		float3 pos;
		float4 color;
	};
	std::deque<Notification> notes;

	CUnit* lastClicked = nullptr;
};


extern CMiniMap* minimap;


#endif /* MINIMAP_H */
