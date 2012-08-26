/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_DECAL_HANDLER_H
#define GROUND_DECAL_HANDLER_H

#include <set>
#include <list>
#include <vector>
#include <string>

#include "System/float3.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
class CBuilding;
class CVertexArray;
struct S3DModel;

namespace Shader {
	struct IProgramObject;
}

struct TrackPart {
	TrackPart()
		: pos1(ZeroVector)
		, pos2(ZeroVector)
		, texPos(0.0f)
		, connected(false)
		, creationTime(0)
	{}
	float3 pos1;
	float3 pos2;
	float texPos;
	bool connected;
	unsigned int creationTime;
};

struct UnitTrackStruct {
	UnitTrackStruct(CUnit* owner)
		: owner(owner)
		, lastUpdate(0)
		, lifeTime(0)
		, trackAlpha(255)
		, alphaFalloff(0.0f)
		, lastAdded(NULL)
	{}

	CUnit* owner;
	unsigned int lastUpdate;
	unsigned int lifeTime;
	int trackAlpha;
	float alphaFalloff;
	TrackPart* lastAdded;
	std::list<TrackPart*> parts;
};

struct TrackToAdd {
	TrackToAdd()
		: tp(NULL)
		, unit(NULL)
		, ts(NULL)
	{}
	TrackPart* tp;
	CUnit* unit;
	UnitTrackStruct* ts;
};

struct TrackToClean {
	TrackToClean()
		: track(NULL)
		, tracks(NULL)
	{}
	TrackToClean(UnitTrackStruct* t, std::set<UnitTrackStruct*>* ts)
		: track(t)
		, tracks(ts)
	{}
	UnitTrackStruct* track;
	std::set<UnitTrackStruct*>* tracks;
};

struct SolidObjectGroundDecal;
struct GhostSolidObject {
	SolidObjectGroundDecal* decal;
	S3DModel* model;

	float3 pos;

	int facing;
	int team;
};

struct SolidObjectGroundDecal {
	SolidObjectGroundDecal()
		: va(NULL)
		, owner(NULL)
		, gbOwner(NULL)
		, posx(0)
		, posy(0)
		, xsize(0)
		, ysize(0)
		, facing(-1)
		, pos(ZeroVector)
		, radius(0.0f)
		, alpha(1.0f)
		, alphaFalloff(1.0f)
	{}
	~SolidObjectGroundDecal();

	CVertexArray* va;
	CSolidObject* owner;
	GhostSolidObject* gbOwner;

	int posx;
	int posy;
	int xsize;
	int ysize;
	int facing;

	float3 pos;
	float radius;

	float alpha;
	float alphaFalloff;
};


class CGroundDecalHandler: public CEventClient, public IExplosionListener
{
public:
	CGroundDecalHandler();
	~CGroundDecalHandler();

	void Draw();
	void Update();

	void RemoveUnit(CUnit* unit);
	int GetTrackType(const std::string& name);

	void AddExplosion(float3 pos, float damage, float radius, bool);
	void ExplosionOccurred(const CExplosionEvent& event);

	void AddSolidObject(CSolidObject* object);
	void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb);
	void ForceRemoveSolidObject(CSolidObject* object);
	int GetSolidObjectDecalType(const std::string& name);

	bool GetDrawDecals() const { return drawDecals; }
	void SetDrawDecals(bool v) { if (decalLevel > 0) { drawDecals = v; } }

public:
	bool WantsEvent(const std::string& eventName) {
		return 
			(eventName == "UnitMoved") ||
			(eventName == "SunChanged") ||
			(eventName == "RenderFeatureMoved") ||
			(eventName == "UnitLoaded") ||
			(eventName == "UnitUnloaded");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void SunChanged(const float3& sunDir);
	void UnitMoved(const CUnit*);
	void RenderFeatureMoved(const CFeature* feature, const float3& oldpos, const float3& newpos);
	void UnitLoaded(const CUnit* unit, const CUnit* transport);
	void UnitUnloaded(const CUnit* unit, const CUnit* transport);

private:
	void LoadDecalShaders();
	void DrawObjectDecals();

	void AddTracks();
	void DrawTracks();
	void CleanTracks();

	void AddScars();
	void DrawScars();

	void UnitMovedNow(CUnit* unit);

private:
	unsigned int scarTex;
	unsigned int decalLevel;
	bool groundScarAlphaFade;

	struct TrackType {
		TrackType()
			: texture(0)
		{}
		std::string name;
		std::set<UnitTrackStruct*> tracks;
		unsigned int texture;
	};
	std::vector<TrackType*> trackTypes;

	struct SolidObjectDecalType {
		SolidObjectDecalType(): texture(0) {}

		std::string name;
		std::set<SolidObjectGroundDecal*> objectDecals;

		unsigned int texture;
	};
	std::vector<SolidObjectDecalType*> objectDecalTypes;

	struct Scar {
		Scar()
			: pos(ZeroVector)
			, radius(0.0f)
			, creationTime(0)
			, lifeTime(0)
			, alphaFalloff(0.0f)
			, startAlpha(1.0f)
			, texOffsetX(0.0f)
			, texOffsetY(0.0f)
			, x1(0)
			, x2(0)
			, y1(0)
			, y2(0)
			, basesize(0.0f)
			, overdrawn(0.0f)
			, lastTest(0)
			, va(NULL)
		{}
		~Scar();

		float3 pos;
		float radius;
		int creationTime;
		int lifeTime;
		float alphaFalloff;
		float startAlpha;
		float texOffsetX;
		float texOffsetY;

		int x1;
		int x2;
		int y1;
		int y2;

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
	std::vector<SolidObjectGroundDecal*> decalsToDraw;

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

	void DrawObjectDecal(SolidObjectGroundDecal* decal);
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
