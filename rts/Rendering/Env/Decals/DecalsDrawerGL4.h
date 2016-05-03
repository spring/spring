/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADER_GROUND_DECAL_DRAWER_H
#define SHADER_GROUND_DECAL_DRAWER_H

#include <vector>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/FBO.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/type2.h"
#include "System/float3.h"
#include "System/float4.h"


#if !defined(GL_VERSION_4_0) || HEADLESS
	class CDecalsDrawerGL4: public IGroundDecalDrawer
	{
	public:
		CDecalsDrawerGL4();

		void Draw() override {}
		void OnDecalLevelChanged() override {}

		void ForceRemoveSolidObject(CSolidObject* object) override {}
		void GhostDestroyed(GhostSolidObject* gb) override {}
		void GhostCreated(CSolidObject* object, GhostSolidObject* gb) override {}
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
	void ForceRemoveSolidObject(CSolidObject* object) override;
	void OnDecalLevelChanged() override;

	void ExplosionOccurred(const CExplosionEvent&) override;

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
		Decal()
			: pos(OnesVector * (-1e6))
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

		bool IsValid() const;
		bool InView() const;
		float GetRating(bool inview_test) const;
		void SetOwner(const void* owner);
		//FIXME void Free();
		//FIXME void Update(); // call when position, size, ... changed

		int GetIdx() const;

		float3 pos;
		float2 size;
		float rot;
		float alpha;
		float alphaFalloff;
		float4 texOffsets;
		float4 texNormalOffsets;
		const void* owner;
		int generation;
		enum { EXPLOSION, BUILDING, LUA } type;
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

public:
	void NewDecal(const Decal& d); //FIXME?
	void FreeDecal(int idx);

	Decal& GetDecalOwnedBy(const void* owner);
	const std::vector<Decal>& GetAllDecals() const { return decals; }
	Decal& GetDecalByIdx(unsigned idx) {
		if (idx >= decals.size()) {
			return decals.front();
		} else {
			return decals[idx];
		}
	}

private:
	void AddExplosion(float3 pos, float damage, float radius, bool addScar);
	void CreateBuildingDecal(const CSolidObject* unit);
	void DeownBuildingDecal(const CSolidObject* object);

	bool FindAndAddToGroup(int decalIdx);
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
	void UpdateOverlap_stage1();
	std::vector<int> UpdateOverlap_stage2();
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
	std::vector<std::pair<int,GLuint>> waitingOverlapGlQueries;
	std::vector<int> waitingDecalsForOverlapTest;
	FBO fboOverlap;

	VBO vboVertices;
	VBO vboIndices;

	VBO uboGroundLighting;

	VBO uboDecalsStructures;
	VBO uboDecalGroups;

	GLuint depthTex;
	GLuint atlasTex;
	Shader::IProgramObject* decalShader;
};

#endif // !defined(GL_VERSION_4_0) || HEADLESS

#endif // SHADER_GROUND_DECAL_DRAWER_H
