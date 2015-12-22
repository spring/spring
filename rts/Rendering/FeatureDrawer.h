/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <vector>
#include <array>
#include "System/creg/creg_cond.h"
#include "System/EventClient.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"

class CCamera;
class CFeature;
class IWorldObjectModelRenderer;

namespace GL {
	struct GeometryBuffer;
}

class CFeatureDrawer: public CEventClient
{
	CR_DECLARE_STRUCT(CFeatureDrawer)

	typedef std::vector<CFeature*> FeatureSet;

public:
	CFeatureDrawer();
	~CFeatureDrawer();

	void UpdateDrawQuad(CFeature* feature);
	void Update();

	void Draw();
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction);
	void DrawShadowPass();
	void DrawAlphaPass(bool disableAdvShading = false);

	void DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);
	void DrawFeature(const CFeature*, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);

	/// LuaOpenGL::Feature{Raw}: draw a single feature with full state setup
	bool DrawIndividualPreCommon(const CFeature* feature);
	void DrawIndividualPostCommon(const CFeature* feature, bool dbg);
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

	void DrawOpaqueFeatures(int modelType, int luaMatType);
	void DrawFarFeatures();

	bool CanDrawFeature(const CFeature*) const;

	void DrawFeatureModel(const CFeature* feature, bool noLuaCall);

	void DrawAlphaPassHelper(int, int);
	void DrawAlphaPassSet(const FeatureSet&, int, int);
	void GetVisibleFeatures(CCamera*, int, bool drawFar);

	void PostLoad();

private:
	int drawQuadsX;
	int drawQuadsY;

	float farDist;
	float featureDrawDistance;
	float featureFadeDistance;

	bool drawForward;
	bool drawDeferred;

	friend class CFeatureQuadDrawer;
	struct ModelRendererProxy {
		ModelRendererProxy(): lastDrawFrame(0) {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				rendererTypes[modelType] = IWorldObjectModelRenderer::GetInstance(modelType);
			}
		}
		~ModelRendererProxy() {
			for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_OTHER; modelType++) {
				delete rendererTypes[modelType];
			}
		}

		const IWorldObjectModelRenderer* GetRenderer(unsigned int i) const { return rendererTypes[i]; }
		      IWorldObjectModelRenderer* GetRenderer(unsigned int i)       { return rendererTypes[i]; }

		unsigned int GetLastDrawFrame() const { return lastDrawFrame; }
		void SetLastDrawFrame(unsigned int f) { lastDrawFrame = f; }

	private:
		std::array<IWorldObjectModelRenderer*, MODELTYPE_OTHER> rendererTypes;

		// frame on which this proxy's owner quad last
		// received a DrawQuad call (i.e. was in view)
		// during *any* pass
		unsigned int lastDrawFrame;
	};

	std::vector<ModelRendererProxy> modelRenderers;
	std::vector<unsigned int> camVisibleQuadFlags;
	std::vector<CFeature*> unsortedFeatures;

	GL::GeometryBuffer* geomBuffer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
