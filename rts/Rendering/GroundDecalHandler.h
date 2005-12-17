#ifndef GROUNDDECALHANDLER_H
#define GROUNDDECALHANDLER_H

#include "Object.h"
#include <set>
#include <list>
#include <vector>
#include <string>

class CUnit;

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
	unsigned int LoadTexture(std::string name);
	void LoadScar(std::string file, unsigned char* buf, int xoffset, int yoffset);

	void AddExplosion(float3 pos, float damage, float radius);

	unsigned int scarTex;

	int decalLevel;

	struct TrackType {
		std::string name;
		std::set<UnitTrackStruct*> tracks;
		unsigned int texture;
	};
	std::vector<TrackType*> trackTypes;

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

	int OverlapSize(Scar* s1, Scar* s2);
	void TestOverlaps(Scar* scar);
	void RemoveScar(Scar* scar,bool removeFromScars);
};

extern CGroundDecalHandler* groundDecals;

#endif
