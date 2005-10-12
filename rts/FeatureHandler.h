#ifndef __FEATURE_HANDLER_H__
#define __FEATURE_HANDLER_H__

#include "Object.h"
#include <map>
#include <string>
#include <list>
#include <stack>
#include <deque>
#include <set>
#include <vector>
#ifdef __GNUG__
#include <ext/hash_set>
#endif
#include "TdfParser.h"

struct S3DOModel;
class CFeature;
class CFileHandler;
class CLoadSaveInterface;
class CVertexArray;

#define MAX_FEATURES 100000
#define DRAW_QUAD_SIZE 32

#define DRAWTYPE_3DO 0
#define DRAWTYPE_TREE 1
#define DRAWTYPE_NONE -1

struct FeatureDef
{
	FeatureDef():geoThermal(0),floating(false){};

	std::string myName;

	float metal;
	float energy;
	float maxHealth;

	float radius;
	float mass;									//used to see if the object can be overrun

	int drawType;
	S3DOModel* model;						//used by 3do obects
	std::string modelname;			//used by 3do obects
	int modelType;							//used by tree etc

	bool destructable;
	bool blocking;
	bool burnable;
	bool floating;

	bool geoThermal;

	std::string deathFeature;		//name of feature that this turn into when killed (not reclaimed)

	int xsize;									//each size is 8 units
	int ysize;									//each size is 8 units
};

class CFeatureHandler :
	public CObject
{
public:
	CFeatureHandler();
	~CFeatureHandler();
	CFeature* CreateWreckage(const float3& pos, const std::string& name, float rot, int iter,int allyteam,bool emitSmoke,std::string fromUnit);

	void Update();

	void LoadWreckFeatures();
	int AddFeature(CFeature* feature);
	void DeleteFeature(CFeature* feature);

	void ResetDrawQuad(int quad);
	void LoadFeaturesFromMap(CFileHandler* file,bool onlyCreateDefs);

	void SetFeatureUpdateable(CFeature* feature);
	void TerrainChanged(int x1, int y1, int x2, int y2);
	void LoadSaveFeatures(CLoadSaveInterface* file, bool loading);

	void Draw();
	void DrawShadowPass();
	void DrawRaw(int extraSize);		//the part of draw that both draw and drawshadowpass can use

	TdfParser wreckParser;
	std::map<std::string,FeatureDef*> featureDefs;

//	std::set<CFeature*> featureSet;
	CFeature* features[MAX_FEATURES];
	std::deque<int> freeIDs;

	std::list<int> toBeRemoved;
#ifdef __GNUG__
	__gnu_cxx::hash_set<int> updateFeatures;
#else
	std::set<int> updateFeatures;
#endif

	struct DrawQuad {
		std::set<CFeature*> staticFeatures;
		std::set<CFeature*> nonStaticFeatures;

		unsigned int displist;
		bool hasFarList;

		int lastSeen;
		float3 lastCamPos;
	};
	
	DrawQuad* drawQuads;

	std::vector<DrawQuad*> farQuads;

	int drawQuadsX;
	int drawQuadsY;
	int numQuads;
	int curClean;

	int overrideId;		//used when loading from savefile
	void DrawFar(CFeature* feature,CVertexArray* va);
	void DrawFarQuads();
	FeatureDef* GetFeatureDef(const std::string name);
};

extern CFeatureHandler* featureHandler;

#endif // __FEATURE_HANDLER_H__
