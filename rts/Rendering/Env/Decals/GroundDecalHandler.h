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
	~CGroundDecalHandler();

	void Draw() override;

	void GhostCreated(CSolidObject* object, GhostSolidObject* gb) override;
	void GhostDestroyed(GhostSolidObject* gb) override;

	void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb);
	void ForceRemoveSolidObject(CSolidObject* object) override;
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
	// CEventClient
	bool WantsEvent(const std::string& eventName) override {
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
	bool GetFullRead() const override { return true; }
	int GetReadAllyTeam() const override { return AllAccessTeam; }

	void SunChanged() override;
	void RenderUnitCreated(const CUnit*, int cloaked) override;
	void RenderUnitDestroyed(const CUnit*) override;
	void RenderFeatureCreated(const CFeature* feature) override;
	void RenderFeatureDestroyed(const CFeature* feature) override;
	void FeatureMoved(const CFeature* feature, const float3& oldpos) override;
	void UnitMoved(const CUnit* unit) override;
	void UnitLoaded(const CUnit* unit, const CUnit* transport) override;
	void UnitUnloaded(const CUnit* unit, const CUnit* transport) override;

	// IExplosionListener
	void ExplosionOccurred(const CExplosionParams& event) override;

public:
	struct SolidObjectDecalType {
		SolidObjectDecalType(): texture(0) {}

		std::string name;
		std::vector<SolidObjectGroundDecal*> objectDecals;

		unsigned int texture;
	};

	struct Scar {
	public:
		Scar(): va(2048) {}
		Scar(const Scar& s) = delete;
		Scar(Scar&& s) { *this = std::move(s); }

		Scar& operator = (const Scar& s) = delete;
		Scar& operator = (Scar&& s) {
			id = s.id;

			x1 = s.x1; x2 = s.x2;
			y1 = s.y1; y2 = s.y2;

			creationTime = s.creationTime;
			lifeTime     = s.lifeTime;

			lastTest = s.lastTest;
			lastDraw = s.lastDraw;

			pos = s.pos;

			radius    = s.radius;
			basesize  = s.basesize;
			overdrawn = s.overdrawn;

			alphaDecay = s.alphaDecay;
			startAlpha = s.startAlpha;
			texOffsetX = s.texOffsetX;
			texOffsetY = s.texOffsetY;

			va = std::move(s.va);
			return *this;
		}

		void Reset() {
			id = -1;

			x1 = 0; x2 = 0;
			y1 = 0; y2 = 0;

			creationTime = 0;
			lifeTime = 0;
			lastTest = 0;
			lastDraw = -1;

			pos = ZeroVector;

			radius = 0.0f;
			basesize = 0.0f;
			overdrawn = 0.0f;

			alphaDecay = 0.0f;
			startAlpha = 1.0f;
			texOffsetX = 0.0f;
			texOffsetY = 0.0f;

			va.Initialize();
		}

	public:
		int id;

		int x1, x2;
		int y1, y2;

		int creationTime;
		int lifeTime;
		int lastTest;
		int lastDraw;

		float3 pos;

		float radius;
		float basesize;
		float overdrawn;

		float alphaDecay;
		float startAlpha;
		float texOffsetX;
		float texOffsetY;

		CVertexArray va;
	};

private:
	void LoadScarTextures();
	void LoadDecalShaders();
	void DrawObjectDecals();

	void AddScars();
	void DrawScars();

	void GatherDecalsForType(SolidObjectDecalType& decalType);
	void AddDecal(CUnit* unit, const float3& newPos);

	void DrawObjectDecal(SolidObjectGroundDecal* decal);
	void DrawGroundScar(Scar& scar);

	int GetScarID() const;
	int ScarOverlapSize(const Scar& s1, const Scar& s2);
	void TestScarOverlaps(const Scar& scar);
	void RemoveScar(Scar& scar);
	void LoadScarTexture(const std::string& file, uint8_t* buf, int xoffset, int yoffset);

private:
	enum DecalShaderProgram {
		DECAL_SHADER_NULL,
		DECAL_SHADER_GLSL,
		DECAL_SHADER_CURR,
		DECAL_SHADER_LAST
	};

	std::vector<SolidObjectDecalType> objectDecalTypes;

	std::array<Shader::IProgramObject*, DECAL_SHADER_LAST> decalShaders;
	std::vector<SolidObjectGroundDecal*> decalsToDraw;

	std::vector<int> addedScars;

	// stores indices into <scars> of reserved slots, per quad
	std::vector< std::vector<int> > scarField;


	int scarFieldX;
	int scarFieldY;

	unsigned int scarTex;

	// number of calls made to TestScarOverlaps
	int lastScarOverlapTest;

	float maxScarOverlapSize;

	bool groundScarAlphaFade;

	LegacyTrackHandler trackHandler;
};

#endif // GROUND_DECAL_HANDLER_H
