/*
	Copyright (c) 2009 Tobi Vollebregt <tobivollebregt@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef FEATUREDRAWER_H_
#define FEATUREDRAWER_H_

#include <set>
#include <vector>
#include "creg/creg_cond.h"


class CFeature;
class CVertexArray;


class CFeatureDrawer
{
	CR_DECLARE(CFeatureDrawer);
	CR_DECLARE_SUB(DrawQuad);

public:
	CFeatureDrawer();
	~CFeatureDrawer();

	void FeatureCreated(CFeature* feature);
	void FeatureDestroyed(CFeature* feature);

	void UpdateDrawQuad(CFeature* feature);
	void UpdateDraw();
	void UpdateDrawPos(CFeature* feature);

	void Draw();
	void DrawShadowPass();
	void DrawRaw(int extraSize, std::vector<CFeature*>* farFeatures); //the part of draw that both draw and drawshadowpass can use

	void DrawFadeFeatures(bool submerged, bool noAdvShading = false);

	void SwapFadeFeatures();

	bool showRezBars;

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

	friend class CFeatureQuadDrawer;
};

extern CFeatureDrawer* featureDrawer;


#endif /* FEATUREDRAWER_H_ */
