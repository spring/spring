/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <vector>
#include <array>
#include "Game/Camera.h"
#include "System/creg/creg_cond.h"
#include "System/EventClient.h"
#include "Rendering/Models/ModelRenderContainer.h"

class CFeature;
class IModelRenderContainer;

namespace GL {
	struct GeometryBuffer;
}

class CFeatureDrawer: public CEventClient
{
	typedef std::vector<CFeature*> FeatureSet;

public:
	CFeatureDrawer();
	~CFeatureDrawer();

	void UpdateDrawQuad(CFeature* feature);
	void Update();

	void Draw();
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction);
	void DrawShadowPass();
	void DrawAlphaPass();

	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);
	void DrawFeature(const CFeature*, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	void PushIndividualState(const CFeature* feature, bool deferredPass);
	void PopIndividualState(const CFeature* feature, bool deferredPass);
	void DrawIndividual(const CFeature* feature, bool noLuaCall);
	void DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall);

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

	void DrawFeatureModel(const CFeature* feature, bool noLuaCall);

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
	bool ffpAlphaMat;

private:
	friend class CFeatureQuadDrawer;
	struct RdrContProxy {
		RdrContProxy(): lastDrawFrame(0) {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				rendererTypes[modelType] = IModelRenderContainer::GetInstance(modelType);
			}
		}
		~RdrContProxy() {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				delete rendererTypes[modelType];
			}
		}

		const IModelRenderContainer* GetRenderer(unsigned int i) const { return rendererTypes[i]; }
		      IModelRenderContainer* GetRenderer(unsigned int i)       { return rendererTypes[i]; }

		unsigned int GetLastDrawFrame() const { return lastDrawFrame; }
		void SetLastDrawFrame(unsigned int f) { lastDrawFrame = f; }

	private:
		std::array<IModelRenderContainer*, MODELTYPE_OTHER> rendererTypes;

		// frame on which this proxy's owner quad last
		// received a DrawQuad call (i.e. was in view)
		// during *any* pass
		unsigned int lastDrawFrame;
	};

	std::vector<RdrContProxy> modelRenderers;
	std::array< std::vector<int>, CCamera::CAMTYPE_ENVMAP> camVisibleQuads;
	std::array<unsigned int, CCamera::CAMTYPE_ENVMAP> camVisDrawFrames;
	std::vector<CFeature*> unsortedFeatures;

	GL::GeometryBuffer* geomBuffer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
