/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_DECAL_HANDLER_H
#define GROUND_DECAL_HANDLER_H

#include <set>
#include <list>
#include <vector>
#include <string>

#include "System/float3.h"
#include "Rendering/GL/myGL.h"

class CUnit;
class CBuilding;
class CVertexArray;
struct GhostBuilding;

namespace Shader {
	struct IProgramObject;
}

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
	TrackPart* lastAdded;
	std::list<TrackPart*> parts;
};

struct TrackToAdd {
	TrackPart* tp;
	CUnit* unit;
	UnitTrackStruct* ts;
};

struct TrackToClean {
	TrackToClean() {}
	TrackToClean(UnitTrackStruct* t, std::set<UnitTrackStruct*>* ts)
		: track(t), tracks(ts) {}
	UnitTrackStruct* track;
	std::set<UnitTrackStruct*>* tracks;
};

struct BuildingGroundDecal {
	BuildingGroundDecal(): va(NULL), owner(NULL), gbOwner(NULL), alpha(1.0f) {}
	~BuildingGroundDecal();

	CVertexArray* va;
	CBuilding* owner;
	GhostBuilding* gbOwner;
	int posx, posy;
	int xsize, ysize;
	int facing;

	float3 pos;
	float radius;

	float alpha;
	float alphaFalloff;
};


class CGroundDecalHandler
{
public:
	CGroundDecalHandler();
	virtual ~CGroundDecalHandler();
	void Draw();
	void Update();

	void UnitMoved(CUnit* unit);
	void UnitMovedNow(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	int GetTrackType(const std::string& name);

	void AddExplosion(float3 pos, float damage, float radius);

	void AddBuilding(CBuilding* building);
	void RemoveBuilding(CBuilding* building, GhostBuilding* gb);
	void ForceRemoveBuilding(CBuilding* building);
	int GetBuildingDecalType(const std::string& name);

	bool GetDrawDecals() const { return drawDecals; }
	void SetDrawDecals(bool v) { if (decalLevel > 0) { drawDecals = v; } }

private:
	void LoadDecalShaders();

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
		Scar(): va(NULL) {}
		~Scar();

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

	enum DecalShaderProgram {
		DECAL_SHADER_ARB,
		DECAL_SHADER_GLSL,
		DECAL_SHADER_CURR,
		DECAL_SHADER_LAST
	};

	std::vector<Shader::IProgramObject*> decalShaders;
	std::vector<BuildingGroundDecal*> decalsToDraw;

	std::list<Scar*> scars;
	std::vector<Scar*> scarsToBeAdded;
	std::vector<Scar*> scarsToBeChecked;

	std::vector<TrackToAdd> tracksToBeAdded;
	std::vector<TrackToClean> tracksToBeCleaned;
	std::vector<UnitTrackStruct*> tracksToBeDeleted;

	std::vector<CUnit*> moveUnits;

	int lastTest;
	float maxOverlap;

	bool drawDecals;

	std::set<Scar*>* scarField;
	int scarFieldX;
	int scarFieldY;

	void DrawBuildingDecal(BuildingGroundDecal* decal);
	void DrawGroundScar(Scar* scar, bool fade);

	int OverlapSize(Scar* s1, Scar* s2);
	void TestOverlaps(Scar* scar);
	void RemoveScar(Scar* scar, bool removeFromScars);
	unsigned int LoadTexture(const std::string& name);
	void LoadScar(const std::string& file, unsigned char* buf, int xoffset,
			int yoffset);
};

extern CGroundDecalHandler* groundDecals;

#endif // GROUND_DECAL_HANDLER_H
