/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <set>
#include <vector>
#include "creg/creg_cond.h"
#include "System/EventClient.h"

class CFeature;
class CVertexArray;


class CFeatureDrawer: public CEventClient
{
	CR_DECLARE(CFeatureDrawer);
	CR_DECLARE_SUB(DrawQuad);

public:
	CFeatureDrawer();
	~CFeatureDrawer();

	void UpdateDrawQuad(CFeature* feature);
	void UpdateDraw();
	void UpdateDrawPos(CFeature* feature);

	void Draw();
	void DrawShadowPass();
	void DrawRaw(int extraSize, std::vector<CFeature*>* farFeatures); //the part of draw that both draw and drawshadowpass can use

	void DrawFadeFeatures(bool submerged, bool noAdvShading = false);
	void SwapFadeFeatures();

	void SetShowRezBars(bool b) { showRezBars = b; }
	bool GetShowRezBars() const { return showRezBars; }



	bool WantsEvent(const std::string& eventName) {
		return (eventName == "FeatureCreated" || eventName == "FeatureDestroyed");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void FeatureCreated(const CFeature* feature);
	void FeatureDestroyed(const CFeature* feature);

private:
	std::set<CFeature *> fadeFeatures;
	std::set<CFeature *> fadeFeaturesS3O;
	std::set<CFeature *> fadeFeaturesSave;
	std::set<CFeature *> fadeFeaturesS3OSave;

	std::set<CFeature *> updateDrawFeatures;

	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		std::set<CFeature *> features;
	};

	std::vector<DrawQuad> drawQuads;

	int drawQuadsX;
	int drawQuadsY;

	float farDist;

	std::vector<CFeature*> drawFar;
	std::vector<CFeature*> drawStat;

	void DrawFeatureStats(CFeature* feature);

	void PostLoad();

	bool showRezBars;

	friend class CFeatureQuadDrawer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
