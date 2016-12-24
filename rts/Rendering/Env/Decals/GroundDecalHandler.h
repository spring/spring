/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_DECAL_HANDLER_H
#define GROUND_DECAL_HANDLER_H

#include <vector>
#include <string>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/LegacyTrackHandler.h"
#include "Rendering/GL/VertexArray.h"
#include "System/float3.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
class CVertexArray;
struct SolidObjectGroundDecal;
struct SolidObjectDecalType;

namespace Shader {
	struct IProgramObject;
}



struct SolidObjectGroundDecal {
public:
	SolidObjectGroundDecal()
		: owner(nullptr)
		, gbOwner(nullptr)
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
	SolidObjectGroundDecal(const SolidObjectGroundDecal& d) = delete;
	SolidObjectGroundDecal(SolidObjectGroundDecal&& d) { *this = std::move(d); }

	SolidObjectGroundDecal& operator = (const SolidObjectGroundDecal& d) = delete;
	SolidObjectGroundDecal& operator = (SolidObjectGroundDecal&& d) {
		va = std::move(d.va);

		owner   = d.owner;   d.owner   = nullptr;
		gbOwner = d.gbOwner; d.gbOwner = nullptr;

		posx   = d.posx;
		posy   = d.posy;
		xsize  = d.xsize;
		ysize  = d.ysize;
		facing = d.facing;

		pos = d.pos;

		radius       = d.radius;
		alpha        = d.alpha;
		alphaFalloff = d.alphaFalloff;
		return *this;
	}

public:
	CVertexArray va;

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

	virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb);
	virtual void GhostDestroyed(GhostSolidObject* gb);

	virtual void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb);
	void ForceRemoveSolidObject(CSolidObject* object);
	static void RemoveTrack(CUnit* unit);

	void OnDecalLevelChanged() override {}

private:
	void BindTextures();
	void KillTextures();
	void BindShader(const float3& ambientColor);
	void DrawDecals();

	void AddExplosion(float3 pos, float damage, float radius);
	void MoveSolidObject(CSolidObject* object, const float3& pos);
	int GetSolidObjectDecalType(const std::string& name);

public:
	//CEventClient
	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "SunChanged") ||
			(eventName == "RenderUnitCreated") ||
			(eventName == "RenderUnitDestroyed") ||
			(eventName == "UnitMoved") ||
			(eventName == "RenderFeatureCreated") ||
			(eventName == "RenderFeatureDestroyed") ||
			(eventName == "FeatureMoved") ||
			(eventName == "UnitLoaded") ||
			(eventName == "UnitUnloaded");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void SunChanged();
	void RenderUnitCreated(const CUnit*, int cloaked);
	void RenderUnitDestroyed(const CUnit*);
	void RenderFeatureCreated(const CFeature* feature);
	void RenderFeatureDestroyed(const CFeature* feature);
	void FeatureMoved(const CFeature* feature, const float3& oldpos);
	void UnitMoved(const CUnit* unit);
	void UnitLoaded(const CUnit* unit, const CUnit* transport);
	void UnitUnloaded(const CUnit* unit, const CUnit* transport);

	//IExplosionListener
	void ExplosionOccurred(const CExplosionParams& event);

private:
	struct SolidObjectDecalType {
		SolidObjectDecalType(): texture(0) {}

		std::string name;
		std::vector<SolidObjectGroundDecal*> objectDecals;

		unsigned int texture;
	};

	struct Scar {
	public:
		Scar()
			: id(-1)
			, creationTime(0)
			, lifeTime(0)

			, x1(0), x2(0)
			, y1(0), y2(0)

			, lastTest(0)
			, lastDraw(-1)

			, pos(ZeroVector)

			, radius(0.0f)
			, basesize(0.0f)
			, overdrawn(0.0f)

			, alphaFalloff(0.0f)
			, startAlpha(1.0f)
			, texOffsetX(0.0f)
			, texOffsetY(0.0f)

			, va(2048)
		{}

		Scar(const Scar& s) = delete;
		Scar(Scar&& s) { *this = std::move(s); }

		Scar& operator = (const Scar& s) = delete;
		Scar& operator = (Scar&& s) {
			id = s.id;

			creationTime = s.creationTime;
			lifeTime     = s.lifeTime;

			x1 = s.x1; x2 = s.x2;
			y1 = s.y1; y2 = s.y2;

			lastTest = s.lastTest;
			lastDraw = s.lastDraw;

			pos = s.pos;

			radius    = s.radius;
			basesize  = s.basesize;
			overdrawn = s.overdrawn;

			alphaFalloff = s.alphaFalloff;
			startAlpha   = s.startAlpha;
			texOffsetX   = s.texOffsetX;
			texOffsetY   = s.texOffsetY;

			va = std::move(s.va);
			return *this;
		}

	public:
		int id;
		int creationTime;
		int lifeTime;

		int x1, x2;
		int y1, y2;

		int lastTest;
		int lastDraw;

		float3 pos;

		float radius;
		float basesize;
		float overdrawn;

		float alphaFalloff;
		float startAlpha;
		float texOffsetX;
		float texOffsetY;

		CVertexArray va;
	};

	void LoadDecalShaders();
	void DrawObjectDecals();

	void AddScars();
	void DrawScars();

	void GatherDecalsForType(SolidObjectDecalType& decalType);
	void AddDecal(CUnit* unit, const float3& newPos);

	void DrawObjectDecal(SolidObjectGroundDecal* decal);
	void DrawGroundScar(Scar& scar, bool fade);

	int GetScarID();
	int OverlapSize(const Scar& s1, const Scar& s2);
	void TestOverlaps(const Scar& scar);
	void RemoveScar(Scar& scar);
	void LoadScar(const std::string& file, std::vector<unsigned char>& buf, int xoffset, int yoffset);

private:
	unsigned int scarTex;
	bool groundScarAlphaFade;

	enum DecalShaderProgram {
		DECAL_SHADER_ARB,
		DECAL_SHADER_GLSL,
		DECAL_SHADER_CURR,
		DECAL_SHADER_LAST
	};

	std::vector<SolidObjectDecalType> objectDecalTypes;

	std::vector<Shader::IProgramObject*> decalShaders;
	std::vector<SolidObjectGroundDecal*> decalsToDraw;

	std::vector<Scar> scars;
	std::vector<Scar> scarsToBeAdded;
	// stores the free slots in <scars>
	std::vector< int> scarIndices;

	// stores indices into <scars>
	std::vector< std::vector<int> > scarField;

	int scarFieldX;
	int scarFieldY;

	int lastTest;
	float maxOverlap;

	LegacyTrackHandler trackHandler;
};

#endif // GROUND_DECAL_HANDLER_H
