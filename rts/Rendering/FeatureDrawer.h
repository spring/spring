/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <vector>
#include <array>
#include "System/creg/creg_cond.h"
#include "System/EventClient.h"
#include "Rendering/Models/WorldObjectModelRenderer.h"

class CFeature;
class IWorldObjectModelRenderer;


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
	void DrawShadowPass();

	void DrawFadeFeatures(bool noAdvShading = false);

	void DrawFeatureNoLists(const CFeature*);
	void DrawFeatureWithLists(const CFeature*, unsigned int preList, unsigned int postList, bool luaCall);


	bool WantsEvent(const std::string& eventName) {
		return (eventName == "RenderFeatureCreated" || eventName == "RenderFeatureDestroyed" || eventName == "FeatureMoved");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
	void FeatureMoved(const CFeature* feature, const float3& oldpos);

private:
	static void UpdateDrawPos(CFeature* f);

	void DrawOpaqueFeatures(int modelType, int luaMatType);
	void DrawFarFeatures();

	bool CanDrawFeature(const CFeature*) const;

	void DrawFadeFeaturesHelper(int, int);
	void DrawFadeFeaturesSet(const FeatureSet&, int, int);
	void GetVisibleFeatures(int, bool drawFar);

	void PostLoad();

private:
	int drawQuadsX;
	int drawQuadsY;

	float farDist;
	float featureDrawDistance;
	float featureFadeDistance;

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

		std::array<IWorldObjectModelRenderer*, MODELTYPE_OTHER> rendererTypes;

		// frame on which this proxy's owner quad last
		// received a DrawQuad call (i.e. was in view)
		unsigned int lastDrawFrame;
	};

	std::vector<ModelRendererProxy> modelRenderers;
	std::vector<CFeature*> unsortedFeatures;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
