/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"
#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/LightHandler.h"


class CSMFReadMap;
class IMeshDrawer;
struct ISMFRenderState;

enum {
	SMF_MESHDRAWER_LEGACY = 0,
	SMF_MESHDRAWER_ROAM,
	SMF_MESHDRAWER_LAST,
};


/**
 * Map drawer implementation for the CSMFReadMap map system.
 */
class CSMFGroundDrawer : public CBaseGroundDrawer
{
public:
	CSMFGroundDrawer(CSMFReadMap* rm);
	~CSMFGroundDrawer();

	friend class CSMFReadMap;

	void Draw(const DrawPass::e& drawPass);
	void DrawShadowPass();
	void DrawDeferredPass(const DrawPass::e& drawPass);

	void Update();
	void UpdateSunDir();

	void SetupBigSquare(const int bigSquareX, const int bigSquareY);

	// for ARB-only clients
	void SetupBaseDrawPass();
	void SetupReflDrawPass();
	void SetupRefrDrawPass();

	void SetDrawDeferredPass(bool b) {
		if ((drawDeferred = b)) {
			drawDeferred &= UpdateGeometryBuffer(false);
		}
	}

	void IncreaseDetail();
	void DecreaseDetail();
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const;

	const CSMFReadMap* GetReadMap() const { return smfMap; }
	      CSMFReadMap* GetReadMap()       { return smfMap; }
	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return &geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return &geomBuffer; }

	IMeshDrawer* SwitchMeshDrawer(int mode = -1);

private:
	void SelectRenderState(bool shaderPath) {
		smfRenderState = shaderPath? smfRenderStateSSP: smfRenderStateFFP;
	}

	void CreateWaterPlanes(bool camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);
	inline void DrawBorder(const DrawPass::e drawPass);

	bool UpdateGeometryBuffer(bool init);

protected:
	CSMFReadMap* smfMap;
	IMeshDrawer* meshDrawer;

	int groundDetail;

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

	ISMFRenderState* smfRenderStateSSP; // default shader-driven rendering path
	ISMFRenderState* smfRenderStateFFP; // fallback shader-less rendering path
	ISMFRenderState* smfRenderState;

	GL::LightHandler lightHandler;
	GL::GeometryBuffer geomBuffer;
};

#endif // _SMF_GROUND_DRAWER_H_
