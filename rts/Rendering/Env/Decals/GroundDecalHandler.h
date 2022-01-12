/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUND_DECAL_HANDLER_H
#define GROUND_DECAL_HANDLER_H

#include <vector>
#include <string>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/LegacyTrackHandler.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "System/float3.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
struct SolidObjectGroundDecal;
struct SolidObjectDecalType;

namespace Shader {
	struct IProgramObject;
}



struct SolidObjectGroundDecal {
public:
	SolidObjectGroundDecal() = default;
	SolidObjectGroundDecal(const SolidObjectGroundDecal& d) = delete;
	SolidObjectGroundDecal(SolidObjectGroundDecal&& d) { *this = std::move(d); }

	SolidObjectGroundDecal& operator = (const SolidObjectGroundDecal& d) = delete;
	SolidObjectGroundDecal& operator = (SolidObjectGroundDecal&& d) {
		owner   = d.owner;   d.owner   = nullptr;
		gbOwner = d.gbOwner; d.gbOwner = nullptr;

		bufIndx = d.bufIndx;
		bufSize = d.bufSize;

		lastUpdateFrame = d.lastUpdateFrame;

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
	CSolidObject* owner = nullptr;
	GhostSolidObject* gbOwner = nullptr;

	unsigned int bufIndx = 0; // verts
	unsigned int bufSize = 0; // bytes

	unsigned int lastUpdateFrame = 0;

	int posx = 0;
	int posy = 0;
	int xsize = 0;
	int ysize = 0;
	int facing = -1;

	float3 pos;

	float radius = 0.0f;
	float alpha = 1.0f;
	float alphaFalloff = 1.0f;
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

	void AddSolidObject(CSolidObject* object) override;
	void ForceRemoveSolidObject(CSolidObject* object) override;

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
		std::string name;
		std::vector<SolidObjectGroundDecal*> objectDecals;

		unsigned int texture = 0;
	};

	struct Scar {
	public:
		Scar() = default;
		Scar(const Scar& s) = delete;
		Scar(Scar&& s) { *this = std::move(s); }

		Scar& operator = (const Scar& s) = delete;
		Scar& operator = (Scar&& s) {
			id = s.id;

			bufIndx = s.bufIndx;
			bufSize = s.bufSize;

			x1 = s.x1; x2 = s.x2;
			y1 = s.y1; y2 = s.y2;

			creationTime = s.creationTime;
			lifeTime     = s.lifeTime;

			lastOverlapTest = s.lastOverlapTest;
			lastUpdateFrame = s.lastUpdateFrame;

			pos = s.pos;

			radius    = s.radius;
			basesize  = s.basesize;
			overdrawn = s.overdrawn;

			alphaDecay = s.alphaDecay;
			startAlpha = s.startAlpha;
			fadedAlpha = s.fadedAlpha;

			texOffsetX = s.texOffsetX;
			texOffsetY = s.texOffsetY;
			return *this;
		}

	public:
		int id = -1;

		unsigned int bufIndx = 0; // verts
		unsigned int bufSize = 0; // bytes

		unsigned int lastOverlapTest = 0;
		unsigned int lastUpdateFrame = 0;

		int x1 = 0, x2 = 0;
		int y1 = 0, y2 = 0;

		int creationTime = 0;
		int lifeTime = 0;

		float3 pos;

		float radius = 0.0f;
		float basesize = 0.0f;
		float overdrawn = 0.0f;

		float alphaDecay = 0.0f;
		float startAlpha = 1.0f;
		float fadedAlpha = 1.0f;

		float texOffsetX = 0.0f;
		float texOffsetY = 0.0f;
	};

private:
	void GenDecalBuffers();
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
	bool LoadScarTexture(const std::string& file, uint8_t* buf, int xoffset, int yoffset);

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
	std::vector<std::vector<int>> scarField;


	GL::RenderDataBuffer decalBuffer;

	VA_TYPE_TC* mapBufferPtr = nullptr; // start-pos
	VA_TYPE_TC* curBufferPos = nullptr; // write-pos


	int scarFieldX = 0;
	int scarFieldY = 0;

	unsigned int scarAtlasTex = 0;

	// number of calls made to TestScarOverlaps
	int lastScarOverlapTest = 0;

	float maxScarOverlapSize = 0.0f;

	LegacyTrackHandler trackHandler;
};

#endif // GROUND_DECAL_HANDLER_H
