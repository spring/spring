/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SMF_GROUND_DRAWER_H_
#define _SMF_GROUND_DRAWER_H_

#include "Map/BaseGroundDrawer.h"
#include "Rendering/GL/FBO.h"


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

	void Update();
	void UpdateSunDir();
	bool UpdateGeometryBuffer(bool init);
	void SetupBigSquare(const int bigSquareX, const int bigSquareY);

	// for ARB-only clients
	void SetupBaseDrawPass();
	void SetupReflDrawPass();
	void SetupRefrDrawPass();

	void SetDeferredDrawMode(bool b) {
		if ((drawDeferred = b)) {
			UpdateGeometryBuffer(false);
		}
	}

	void IncreaseDetail();
	void DecreaseDetail();
	int GetGroundDetail(const DrawPass::e& drawPass = DrawPass::Normal) const;

	const CSMFReadMap* GetReadMap() const { return smfMap; }
	      CSMFReadMap* GetReadMap()       { return smfMap; }
	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	GLuint GetGeomBufferTexture(unsigned int idx) { return geomBufferTextureIDs[idx]; }

	IMeshDrawer* SwitchMeshDrawer(int mode = -1);

private:
	void SelectRenderState(bool shaderPath) {
		smfRenderState = shaderPath? smfRenderStateSSP: smfRenderStateFFP;
	}
	void DrawDeferredPass(const DrawPass::e& drawPass);

	void CreateWaterPlanes(bool camOufOfMap);
	inline void DrawWaterPlane(bool drawWaterReflection);
	inline void DrawBorder(const DrawPass::e drawPass);

protected:
	CSMFReadMap* smfMap;
	IMeshDrawer* meshDrawer;

	int groundDetail;

	GLuint waterPlaneCamOutDispList;
	GLuint waterPlaneCamInDispList;

	GLuint geomBufferTextureIDs[GBUFFER_ATTACHMENT_COUNT];
	GLenum geomBufferAttachments[GBUFFER_ATTACHMENT_COUNT];

	ISMFRenderState* smfRenderStateSSP; // default shader-driven rendering path
	ISMFRenderState* smfRenderStateFFP; // fallback shader-less rendering path
	ISMFRenderState* smfRenderState;

	GL::LightHandler lightHandler;

	FBO geomBuffer;
};

#endif // _SMF_GROUND_DRAWER_H_
