#ifndef __FEATURE_HANDLER_H__
#define __FEATURE_HANDLER_H__

#include "Object.h"
#include <string>
#include <list>
#include <deque>
#include <vector>
#include "TdfParser.h"
#include "creg/creg.h"
#include "FeatureDef.h"
#include "FeatureSet.h"

struct S3DOModel;
class CFileHandler;
class CLoadSaveInterface;
class CVertexArray;

#define DRAW_QUAD_SIZE 32

class CFeatureHandler :
	public CObject
{
public:
	CR_DECLARE(CFeatureHandler);

	CFeatureHandler();
	~CFeatureHandler();

	void Serialize(creg::ISerializer *s);
	void PostLoad();

	CFeature* CreateWreckage(const float3& pos, const std::string& name,
	                         float rot, int facing, int iter, int team, int allyteam,
	                         bool emitSmoke,std::string fromUnit);

	void Update();

	void LoadWreckFeatures();
	int AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);
	void UpdateDrawQuad(CFeature* feature, const float3& newPos);

	void LoadFeaturesFromMap(bool onlyCreateDefs);
	void AddFeatureDef(const std::string& name, FeatureDef* feature);
	FeatureDef* GetFeatureDef(const std::string name);
	FeatureDef* GetFeatureDefByID(int id);

	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);

	void Draw();
	void DrawShadowPass();
	void DrawRaw(int extraSize, std::vector<CFeature*>* farFeatures);		//the part of draw that both draw and drawshadowpass can use

	TdfParser wreckParser;
	std::map<std::string, FeatureDef*> featureDefs;
	std::vector<FeatureDef*> featureDefsVector;

	int nextFreeID;
	std::deque<int> freeIDs;
	CFeatureSet activeFeatures;

	std::list<int> toBeRemoved;
	CFeatureSet updateFeatures;

	CR_DECLARE_SUB(DrawQuad);
	struct DrawQuad {
		CR_DECLARE_STRUCT(DrawQuad);
		CFeatureSet features;
	};

	std::vector<DrawQuad> drawQuads;

	int drawQuadsX;
	int drawQuadsY;

	void DrawFar(CFeature* feature,CVertexArray* va);
};

extern CFeatureHandler* featureHandler;

#endif // __FEATURE_HANDLER_H__
