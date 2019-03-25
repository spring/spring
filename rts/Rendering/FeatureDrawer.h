/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <vector>
#include <array>

#include "Game/Camera.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "System/creg/creg_cond.h"
#include "System/EventClient.h"

class CFeature;

namespace GL {
	struct GeometryBuffer;
}

class CFeatureDrawer: public CEventClient
{
public:
	CFeatureDrawer(): CEventClient("[CFeatureDrawer]", 313373, false) {}

	static void InitStatic();
	static void KillStatic(bool reload);

	void Init();
	void Kill();

	void ConfigNotify(const std::string& key, const std::string& value);

	void UpdateDrawQuad(CFeature* feature);
	void Update();

	void Draw();
	void DrawOpaquePass(bool deferredPass);
	void DrawShadowPass();
	void DrawAlphaPass(bool aboveWater);

	static void SetFeatureLuaTrans(const CFeature* feature, bool lodCall);
	static void SetFeatureDefTrans(const CFeature* feature, bool lodCall);

	static void DrawFeatureLuaTrans(const CFeature* feature, bool lodCall, bool noLuaCall);
	static void DrawFeatureDefTrans(const CFeature*, bool lodCall, bool noLuaCall);

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void PushIndividualState(const CFeature* feature, bool deferredPass);
	void PopIndividualState(const CFeature* feature, bool deferredPass);
	void DrawIndividualDefTrans(const CFeature* feature, bool noLuaCall);
	void DrawIndividualLuaTrans(const CFeature* feature, bool noLuaCall);

	void SetDrawForwardPass(bool b) { drawForward = b; }
	void SetDrawDeferredPass(bool b) { drawDeferred = b; }

public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "RenderFeatureCreated" || eventName == "RenderFeatureDestroyed" || eventName == "FeatureMoved");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
	void FeatureMoved(const CFeature* feature, const float3& oldpos);

public:
	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }

	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

private:
	static void UpdateDrawPos(CFeature* f);

	void DrawOpaqueFeatures(int modelType);
	void DrawAlphaFeatures(int modelType);
	void DrawAlphaFeature(CFeature* f, bool ffpMat);
	void DrawFarFeatures();

	bool CanDrawFeature(const CFeature*) const;

	static void DrawFeatureModel(const CFeature* feature, bool noLuaCall);

	void FlagVisibleFeatures(
		const CCamera*,
		bool drawShadowPass,
		bool drawReflection,
		bool drawRefraction,
		bool drawFar
	);
	void GetVisibleFeatures(CCamera*, int, bool drawFar);

private:
	int drawQuadsX;
	int drawQuadsY;

	float farDist;
	float featureDrawDistance;
	float featureFadeDistance;

	bool drawForward;
	bool drawDeferred;

	// we need these because alpha- and shadow-pass both
	// reuse Draw{Opaque}Feature{s} which sets team color
	bool inAlphaPass;
	bool inShadowPass;

private:
	friend class CFeatureQuadDrawer;
	struct RdrContProxy {
		const ModelRenderContainer<CFeature>& GetRenderer(unsigned int i) const { return rendererTypes[i]; }
		      ModelRenderContainer<CFeature>& GetRenderer(unsigned int i)       { return rendererTypes[i]; }

		unsigned int GetLastDrawFrame() const { return lastDrawFrame; }
		void SetLastDrawFrame(unsigned int f) { lastDrawFrame = f; }

	private:
		std::array<ModelRenderContainer<CFeature>, MODELTYPE_OTHER> rendererTypes;

		// frame on which this proxy's owner quad last
		// received a DrawQuad call (i.e. was in view)
		// during *any* pass
		unsigned int lastDrawFrame = 0;
	};

	std::vector<RdrContProxy> modelRenderers;
	std::array< std::vector<int>, CCamera::CAMTYPE_ENVMAP> camVisibleQuads;
	std::array<unsigned int, CCamera::CAMTYPE_ENVMAP> camVisDrawFrames;
	std::vector<CFeature*> unsortedFeatures;

	GL::GeometryBuffer* geomBuffer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
