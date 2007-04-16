#ifndef __FEATURE_HANDLER_H__
#define __FEATURE_HANDLER_H__

#include "Object.h"
#include <string>
#include <list>
#include <deque>
#include <set>
#include <vector>
#include <map>
#include SPRING_HASH_SET_H
#include "TdfParser.h"
#include "creg/creg.h"

#include "FeatureDef.h"

struct S3DOModel;
class CFeature;
class CFileHandler;
class CLoadSaveInterface;
class CVertexArray;

#define MAX_FEATURES 100000
#define DRAW_QUAD_SIZE 32

class CFeatureHandler :
	public CObject
{
public:
	CFeatureHandler();
	~CFeatureHandler();
	CFeature* CreateWreckage(const float3& pos, const std::string& name,
	                         float rot, int facing, int iter, int team,
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
	void LoadSaveFeatures(CLoadSaveInterface* file, bool loading);

	void Draw();
	void DrawShadowPass();
	void DrawRaw(int extraSize, std::vector<CFeature*>* farFeatures);		//the part of draw that both draw and drawshadowpass can use

	TdfParser wreckParser;
	std::map<std::string, FeatureDef*> featureDefs;
	std::vector<FeatureDef*> featureDefsVector;

//	std::set<CFeature*> featureSet;
	CFeature* features[MAX_FEATURES];
	std::deque<int> freeIDs;

	std::list<int> toBeRemoved;
	SPRING_HASH_SET<int> updateFeatures;

	struct DrawQuad {
		std::set<CFeature*> features;
	};
	
	DrawQuad* drawQuads;
	
	int drawQuadsX;
	int drawQuadsY;
	int numQuads;

	int overrideId;		//used when loading from savefile
	void DrawFar(CFeature* feature,CVertexArray* va);
};

extern CFeatureHandler* featureHandler;

#endif // __FEATURE_HANDLER_H__
