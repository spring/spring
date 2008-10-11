#ifndef GROUNDDECALHANDLER_H
#define GROUNDDECALHANDLER_H

#include "Object.h"
#include <set>
#include <list>
#include <vector>
#include <string>
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"

class CUnit;
class CBuilding;
class CVertexArray;

struct TrackPart {
	float3 pos1;
	float3 pos2;
	float texPos;
	bool connected;
	int creationTime;
};

struct UnitTrackStruct {
	CUnit* owner;
	int lifeTime;
	int trackAlpha;
	float alphaFalloff;
	std::list<TrackPart> parts;
};

struct BuildingGroundDecal {
	BuildingGroundDecal(): va(0x0), owner(0x0), gbOwner(0x0), alpha(1.0f) {
	}

	CVertexArray* va;
	CBuilding* owner;
	CUnitDrawer::GhostBuilding* gbOwner;
	int posx, posy;
	int xsize, ysize;
	int facing;

	float3 pos;
	float radius;

	float alpha;
	float AlphaFalloff;
};


class CGroundDecalHandler:
	public CObject
{
public:
	CGroundDecalHandler(void);
	virtual ~CGroundDecalHandler(void);
	void Draw(void);
	void Update(void);

	void UnitMoved(CUnit* unit);
	void UnitMovedNow(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	int GetTrackType(const std::string& name);

	void AddExplosion(float3 pos, float damage, float radius);

	void AddBuilding(CBuilding* building);
	void RemoveBuilding(CBuilding* building,CUnitDrawer::GhostBuilding* gb);
	int GetBuildingDecalType(const std::string& name);

	GLuint scarTex;
	int decalLevel;
	int groundScarAlphaFade;

	struct TrackType {
		std::string name;
		std::set<UnitTrackStruct*> tracks;
		GLuint texture;
	};
	std::vector<TrackType*> trackTypes;

	struct BuildingDecalType {
		std::string name;
		std::set<BuildingGroundDecal*> buildingDecals;
		GLuint texture;
	};
	std::vector<BuildingDecalType*> buildingDecalTypes;

	struct Scar {
		Scar(): va(0x0) {
		}
		~Scar() { delete va; va = 0; }

		float3 pos;
		float radius;
		int creationTime;
		int lifeTime;
		float alphaFalloff;
		float startAlpha;
		float texOffsetX;
		float texOffsetY;

		int x1, x2;
		int y1, y2;

		float basesize;
		float overdrawn;

		int lastTest;
		CVertexArray* va;
	};

	std::list<Scar*> scars;

	std::vector<CUnit *> moveUnits;

	int lastTest;
	float maxOverlap;

	bool GetDrawDecals() const { return drawDecals; }
	void SetDrawDecals(bool v) { if (decalLevel > 0) { drawDecals = v; } }

protected:
	bool drawDecals;

	std::set<Scar*>* scarField;
	int scarFieldX;
	int scarFieldY;

	unsigned int decalVP;
	unsigned int decalFP;

	int OverlapSize(Scar* s1, Scar* s2);
	void TestOverlaps(Scar* scar);
	void RemoveScar(Scar* scar, bool removeFromScars);
	unsigned int LoadTexture(const std::string& name);
	void LoadScar(const std::string& file, unsigned char* buf,
	              int xoffset, int yoffset);
	void SetTexGen(float scalex, float scaley, float offsetx, float offsety);
};

extern CGroundDecalHandler* groundDecals;

#endif
