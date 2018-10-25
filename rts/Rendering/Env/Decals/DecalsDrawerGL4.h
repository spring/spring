/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADER_GROUND_DECAL_DRAWER_H
#define SHADER_GROUND_DECAL_DRAWER_H

#include <vector>
#include <array>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/LegacyTrackHandler.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/EventClient.h"
#include "System/type2.h"
#include "System/float3.h"
#include "System/float4.h"


#if !defined(GL_VERSION_4_3) || HEADLESS
	class CDecalsDrawerGL4: public IGroundDecalDrawer
	{
	public:
		struct Decal {
			Decal()
				: rot(0.0f)
				, alpha(0.0f)
				, owner(nullptr)
				, type(EXPLOSION)
			{}
			void Free() const {};
			bool IsValid() const { return false; }
			void Invalidate() const {}
			bool InvalidateExtents() const { return false; }
			void SetTexture(const std::string& name) {}
			int GetIdx() const { return -1; }
			std::string GetTexture() const { return ""; }

			float3 pos;
			float2 size;
			float rot;
			float alpha;
			const void* owner;
			enum { EXPLOSION, BUILDING, LUA } type;
		};
		CDecalsDrawerGL4();

		void Draw() override {}
		void OnDecalLevelChanged() override {}

		void ForceRemoveSolidObject(CSolidObject* object) override {}
		void GhostDestroyed(GhostSolidObject* gb) override {}
		void GhostCreated(CSolidObject* object, GhostSolidObject* gb) override {}
		int CreateLuaDecal() { return 0;}
		const std::vector<Decal>& GetAllDecals() const { return decals; }

		const Decal& GetDecalByIdx(unsigned idx) const { assert(false); static Decal tmp; return tmp; }
		      Decal& GetDecalByIdx(unsigned idx)       { assert(false); static Decal tmp; return tmp; }
	private:
		std::vector<Decal> decals;
	};
#else

namespace Shader {
	struct IProgramObject;
}



class CDecalsDrawerGL4: public IGroundDecalDrawer, public CEventClient, public IExplosionListener
{
public:
	CDecalsDrawerGL4();
	~CDecalsDrawerGL4();

	void Draw() override;

	//FIXME make eventClient!!!
	void GhostDestroyed(GhostSolidObject* gb) override;
	void GhostCreated(CSolidObject* object, GhostSolidObject* gb) override;

	void AddSolidObject(CSolidObject* object) override { /*TODO*/ }
	void ForceRemoveSolidObject(CSolidObject* object) override;

	void OnDecalLevelChanged() override;

	void ExplosionOccurred(const CExplosionParams&) override;

public:
	bool WantsEvent(const std::string& eventName) override {
		return
			   (eventName == "RenderUnitCreated")
			|| (eventName == "RenderUnitDestroyed")
			|| (eventName == "RenderFeatureCreated")
			|| (eventName == "RenderFeatureDestroyed")
			|| (eventName == "ViewResize")
			|| (eventName == "Update")
			|| (eventName == "GameFrame")
			|| (eventName == "SunChanged");
		;
		/*return
			(eventName == "UnitMoved")*/
	}
	bool GetFullRead() const override { return true; }
	int GetReadAllyTeam() const override { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit* unit, int cloaked) override;
	void RenderUnitDestroyed(const CUnit* unit) override;
	void RenderFeatureCreated(const CFeature* feature) override;
	void RenderFeatureDestroyed(const CFeature* feature) override;

	//FIXME
	/*void FeatureMoved(const CFeature* feature, const float3& oldpos);
	void UnitMoved(const CUnit* unit);
	void UnitLoaded(const CUnit* unit, const CUnit* transport);
	void UnitUnloaded(const CUnit* unit, const CUnit* transport);*/

	void SunChanged() override;
	void ViewResize() override;
	void Update() override;
	void GameFrame(int n) override;

public:
	static constexpr int MAX_DECALS_PER_GROUP = 4;
	static constexpr int MAX_OVERLAP = 3;
	static constexpr int OVERLAP_TEST_TEXTURE_SIZE = 256;

	struct Decal {
	public:
		Decal()
		: pos(-1e6, -1e6, -1e6)
		, size(-1.0f, -1.0f)
		, rot(0.0f)
		, alpha(0.0f)
		, alphaFalloff(0.0f)
		, texOffsets(0.0f, 0.0f, 0.0f, 0.0f)
		, texNormalOffsets(0.0f, 0.0f, 0.0f, 0.0f)
		, owner(nullptr)
		, generation(0)
		, type(EXPLOSION)
		{}

		void Free() const;
		void Invalidate() const; ///< call after alpha & texture changes (to upload changes to GPU)
		bool InvalidateExtents() const; ///< call after pos, size, rot changes (does same as Invalidate() + searches a new `DecalGroup`), returns true whena  group was found (else it isn't rendered)

		int GetIdx() const;
		bool IsValid() const;
		bool InView() const;
		float GetRating(bool inview_test) const;
		void SetOwner(const void* owner);
		void SetTexture(const std::string& name);
		std::string GetTexture() const;

	public:
		float3 pos;
		float2 size;
		float rot;
		float alpha;
		float alphaFalloff;
		float4 texOffsets;
		float4 texNormalOffsets;
		const void* owner;
		int generation;
		enum { EXPLOSION, BUILDING, LUA } type; //FIXME merge with owner?
	};

	struct SDecalGroup {
		std::array<float4, 2> boundAABB;
		std::array<int, MAX_DECALS_PER_GROUP> ids;

		int size() const {
			for (int i = 0; i < MAX_DECALS_PER_GROUP; ++i) {
				if (ids[i] == 0) {
					return i;
				}
			}
			return MAX_DECALS_PER_GROUP;
		}
	};

	friend Decal;

public:
	int CreateLuaDecal();
	int NewDecal(const Decal& d);
	void FreeDecal(int idx);

	Decal& GetDecalOwnedBy(const void* owner);
	const std::vector<Decal>& GetAllDecals() const { return decals; }

	const Decal& GetDecalByIdx(unsigned idx) const { return ((idx < decals.size())? decals[idx]: decals.front()); }
	      Decal& GetDecalByIdx(unsigned idx)       { return ((idx < decals.size())? decals[idx]: decals.front()); }


private:
	void AddExplosion(float3 pos, float damage, float radius);
	void CreateBuildingDecal(const CSolidObject* unit);
	void DeownBuildingDecal(const CSolidObject* object);

	bool FindAndAddToGroup(int decalIdx);
	void RemoveFromGroup(int decalIdx);
	bool AddDecalToGroup(SDecalGroup& g, const Decal& d, const int decalIdx);
	bool TryToCombineDecalGroups(SDecalGroup& g1, SDecalGroup& g2);
	void UpdateBoundingBox(SDecalGroup& g);

	void GetWorstRatedDecal(int* idx, float* rating, const bool inview_test) const;
	bool AnyDecalsInView() const;
	void DetectMaxDecals();

	void LoadShaders();
	void GenerateAtlasTexture();
	void CreateBoundingBoxVBOs();
	void CreateStructureVBOs();

	void UpdateDecalsVBO();

	void OptimizeGroups();

	void UpdateOverlap();
	std::vector<int> UpdateOverlap_PreCheck();
	void UpdateOverlap_Initialize(GL::RenderDataBufferTC* rdb);
	std::vector<int> UpdateOverlap_CheckQueries(GL::RenderDataBufferTC* rdb);
	void UpdateOverlap_GenerateQueries(const std::vector<int>& candidatesForOverlap, GL::RenderDataBufferTC* rdb);
	std::vector<int> CandidatesForOverlap() const;

	void DrawDecals();
	//void DrawTracks();

private:
	std::vector<Decal> decals;
	std::vector<SDecalGroup> groups;

	int curWorstDecalIdx;
	float curWorstDecalRating;
	std::vector<int> freeIds;
	std::vector<int> decalsToUpdate;
	std::vector<int> alphaDecayingDecals;

	int maxDecals;
	int maxDecalGroups;

	bool useSSBO;

	// used to turn off parallax mapping when fps drops too low
	int laggedFrames;

	// used to remove decals that are totally overlapped by others
	int overlapStage;

	std::vector<std::pair<int, GLuint>> waitingOverlapGlQueries;
	std::vector<int> waitingDecalsForOverlapTest;

	FBO fboOverlap;

	VAO bboxArray;
	VBO bboxVerts;
	VBO bboxIndcs;

	VBO uboGroundLighting;

	VBO uboDecalsStructures;
	VBO uboDecalGroups;

	GLuint depthTex;
	GLuint atlasTex;
	Shader::IProgramObject* decalShader;

	LegacyTrackHandler trackHandler;
};

#endif // !defined(GL_VERSION_4_0) || HEADLESS

#endif // SHADER_GROUND_DECAL_DRAWER_H
