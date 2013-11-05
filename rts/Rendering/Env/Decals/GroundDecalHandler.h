/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_DECAL_HANDLER_H
#define GROUND_DECAL_HANDLER_H

#include <set>
#include <list>
#include <vector>
#include <string>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "System/float3.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
class CVertexArray;
struct SolidObjectGroundDecal;
struct SolidObjectDecalType;
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
		, alphaFalloff(0.0f)
		, lastAdded(NULL)
	{}

	CUnit* owner;

	unsigned int lastUpdate;
	unsigned int lifeTime;

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


class CGroundDecalHandler: public IGroundDecalDrawer, public CEventClient, public IExplosionListener
{
public:
	CGroundDecalHandler();
	virtual ~CGroundDecalHandler();

	virtual void Draw();
	virtual void Update() {}

	virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb);
	virtual void GhostDestroyed(GhostSolidObject* gb);

	virtual void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb);
	virtual void ForceRemoveSolidObject(CSolidObject* object);

private:
	void AddExplosion(float3 pos, float damage, float radius, bool);
	void MoveSolidObject(CSolidObject* object, const float3& pos);
	int GetSolidObjectDecalType(const std::string& name);
	int GetTrackType(const std::string& name);

public:
	//CEventClient
	bool WantsEvent(const std::string& eventName) {
		return 
			(eventName == "SunChanged") ||
			(eventName == "RenderUnitCreated") ||
			(eventName == "RenderUnitDestroyed") ||
			(eventName == "RenderUnitMoved") ||
			(eventName == "RenderFeatureCreated") ||
			(eventName == "RenderFeatureMoved") ||
			(eventName == "UnitLoaded") ||
			(eventName == "UnitUnloaded");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void SunChanged(const float3& sunDir);
	void RenderUnitCreated(const CUnit*, int cloaked);
	void RenderUnitDestroyed(const CUnit*);
	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureMoved(const CFeature* feature, const float3& oldpos, const float3& newpos);
	void RenderUnitMoved(const CUnit* unit, const float3& newpos);
	void UnitLoaded(const CUnit* unit, const CUnit* transport);
	void UnitUnloaded(const CUnit* unit, const CUnit* transport);

	//IExplosionListener
	void ExplosionOccurred(const CExplosionEvent& event);

private:
	struct TrackType {
		TrackType()
			: texture(0)
		{}
		std::string name;
		std::set<UnitTrackStruct*> tracks;
		unsigned int texture;
	};

	struct SolidObjectDecalType {
		SolidObjectDecalType(): texture(0) {}

		std::string name;
		std::set<SolidObjectGroundDecal*> objectDecals;

		unsigned int texture;
	};

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
			, x1(0), x2(0)
			, y1(0), y2(0)
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

		int x1, x2;
		int y1, y2;

		float basesize;
		float overdrawn;

		int lastTest;
		CVertexArray* va;
	};

	void LoadDecalShaders();
	void DrawObjectDecals();

	void AddTracks();
	void DrawTracks();
	void CleanTracks();

	void AddScars();
	void DrawScars();

	void GatherDecalsForType(SolidObjectDecalType* decalType);
	void AddDecalAndTrack(CUnit* unit, const float3& newPos);

private:
	unsigned int scarTex;
	bool groundScarAlphaFade;

	std::vector<SolidObjectDecalType*> objectDecalTypes;
	std::vector<TrackType*> trackTypes;

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

	int lastTest;
	float maxOverlap;

	std::set<Scar*>* scarField;
	int scarFieldX;
	int scarFieldY;

	void DrawObjectDecal(SolidObjectGroundDecal* decal);
	void DrawGroundScar(Scar* scar, bool fade);

	int OverlapSize(Scar* s1, Scar* s2);
	void TestOverlaps(Scar* scar);
	void RemoveScar(Scar* scar, bool removeFromScars);
	unsigned int LoadTexture(const std::string& name);
	void LoadScar(const std::string& file, unsigned char* buf, int xoffset, int yoffset);
};

extern CGroundDecalHandler* groundDecals_;

#endif // GROUND_DECAL_HANDLER_H
