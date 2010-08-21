/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <set>
#include <vector>
#include "creg/creg_cond.h"
#include "System/EventClient.h"

class CFeature;
class IWorldObjectModelRenderer;
class CVertexArray;


class CFeatureDrawer: public CEventClient
{
	CR_DECLARE(CFeatureDrawer);
	CR_DECLARE_SUB(DrawQuad);

public:
	CFeatureDrawer();
	~CFeatureDrawer();

	void UpdateDrawQuad(CFeature* feature);
	void Update();

	void Draw();
	void DrawShadowPass();

	void DrawFadeFeatures(bool noAdvShading = false);

	bool WantsEvent(const std::string& eventName) {
		return (eventName == "RenderFeatureCreated" || eventName == "RenderFeatureDestroyed" || eventName == "RenderFeatureMoved");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
	void RenderFeatureMoved(const CFeature* feature);

#ifdef USE_GML
	void DrawFeatureStats(); 	  	
	void DrawFeatureStatBars(const CFeature*);
	std::vector<CFeature*> drawStat;
	bool showRezBars;
	void SetShowRezBars(bool b) { showRezBars = b; }
	bool GetShowRezBars() const { return showRezBars; }
#endif

private:
	void UpdateDrawPos(CFeature* f);

	void DrawOpaqueFeatures(int);
	void DrawFarFeatures();
	bool DrawFeatureNow(const CFeature*);
	void DrawFadeFeaturesHelper(int);
	void DrawFadeFeaturesSet(std::set<CFeature*>&, int);
	void GetVisibleFeatures(int, bool drawFar);

	void PostLoad();

	std::set<CFeature*> unsortedFeatures;

	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		std::set<CFeature*> features;
	};

	std::vector<DrawQuad> drawQuads;

	int drawQuadsX;
	int drawQuadsY;

	float farDist;

	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> cloakedModelRenderers;

	friend class CFeatureQuadDrawer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
