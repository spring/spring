#ifndef GROUNDDECALHANDLER_H
#define GROUNDDECALHANDLER_H

#include "Object.h"
#include <set>
#include <list>
#include <vector>
#include <string>
#include "Rendering/UnitModels/UnitDrawer.h"

class CUnit;
class CBuilding;

struct TrackPart {
	float3 pos1;
	float3 pos2;
	float texPos;
	bool connected;
	int creationTime;
};

struct UnitTrackStruct{
	CUnit* owner;
	int lifeTime;
	int trackAlpha;
	float alphaFalloff;
	std::list<TrackPart> parts;
};

struct BuildingGroundDecal{
	CBuilding* owner;
	CUnitDrawer::GhostBuilding* gbOwner;
	int posx,posy;
	int xsize,ysize;

	float3 pos;
	float radius;

	float alpha;
	float AlphaFalloff;
};

class CGroundDecalHandler :
	public CObject
{
public:
	CGroundDecalHandler(void);
	virtual ~CGroundDecalHandler(void);
	void Draw(void);
	void Update(void);

	void UnitMoved(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	int GetTrackType(std::string name);

	void AddExplosion(float3 pos, float damage, float radius);

	void AddBuilding(CBuilding* building);
	void RemoveBuilding(CBuilding* building,CUnitDrawer::GhostBuilding* gb);
	int GetBuildingDecalType(std::string name);

	unsigned int scarTex;
	int decalLevel;

	struct TrackType {
		std::string name;
		std::set<UnitTrackStruct*> tracks;
		unsigned int texture;
	};
	std::vector<TrackType*> trackTypes;

	struct BuildingDecalType {
		std::string name;
		std::set<BuildingGroundDecal*> buildingDecals;
		unsigned int texture;
	};
	std::vector<BuildingDecalType*> buildingDecalTypes;

	struct Scar {
		float3 pos;
		float radius;
		int creationTime;
		int lifeTime;
		float alphaFalloff;
		float startAlpha;
		float texOffsetX;
		float texOffsetY;

		int x1,x2;
		int y1,y2;
		
		float basesize;
		float overdrawn;

		int lastTest;
	};

	std::list<Scar*> scars;
	
	int lastTest;
	float maxOverlap;
protected:
	std::set<Scar*>* scarField;
	int scarFieldX;
	int scarFieldY;

	unsigned int decalVP;
	unsigned int decalFP;

	int OverlapSize(Scar* s1, Scar* s2);
	void TestOverlaps(Scar* scar);
	void RemoveScar(Scar* scar,bool removeFromScars);
	unsigned int LoadTexture(std::string name);
	void LoadScar(std::string file, unsigned char* buf, int xoffset, int yoffset);
	void SetTexGen(float scalex,float scaley, float offsetx, float offsety);
};

extern CGroundDecalHandler* groundDecals;

#endif
