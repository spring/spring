/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SHADER_GROUND_DECAL_DRAWER_H
#define SHADER_GROUND_DECAL_DRAWER_H

#include <list>
#include <vector>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VBO.h"
#include "System/EventClient.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "System/float3.h"
#include "System/float4.h"
#include "System/type2.h"

#if !defined(GL_VERSION_4_0) || HEADLESS
	class CDecalsDrawerGL4: public IGroundDecalDrawer
	{
	public:
		CDecalsDrawerGL4();
		virtual ~CDecalsDrawerGL4() {}

		virtual void Draw() {}
		virtual void Update() {}

		virtual void ForceRemoveSolidObject(CSolidObject* object) {}
		virtual void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb) {}
		virtual void GhostDestroyed(GhostSolidObject* gb) {}
		virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb) {}
	};
#else

namespace Shader {
	struct IProgramObject;
}


class CDecalsDrawerGL4: public IGroundDecalDrawer, public CEventClient, public IExplosionListener
{
public:
	CDecalsDrawerGL4();
	virtual ~CDecalsDrawerGL4();

	virtual void Draw();
	virtual void Update() {}

	virtual void ForceRemoveSolidObject(CSolidObject* object) {/*FIXME*/}
	virtual void RemoveSolidObject(CSolidObject* object, GhostSolidObject* gb) {/*FIXME*/}
	virtual void GhostDestroyed(GhostSolidObject* gb) {/*FIXME*/}
	virtual void GhostCreated(CSolidObject* object, GhostSolidObject* gb) {/*FIXME*/}

	virtual void ExplosionOccurred(const CExplosionEvent&);

private:
	void AddExplosion(float3 pos, float damage, float radius, bool addScar);

public:
	bool WantsEvent(const std::string& eventName) {
		return
			   (eventName == "UnitCreated")
			|| (eventName == "UnitDestroyed")
			|| (eventName == "ViewResize")
		;
		/*return
			(eventName == "UnitMoved") ||
			(eventName == "SunChanged");*/
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void UnitCreated(const CUnit* unit, const CUnit* builder);
	void UnitDestroyed(const CUnit* unit, const CUnit* attacker);

	void ViewResize();

	//void SunChanged(const float3& sunDir);
	//void UnitMoved(const CUnit*);

private:
	struct Decal {
		Decal()
			: pos(ZeroVector)
			, size(0.0f, 0.0f)
			, rot(0.0f)
			, alpha(1.0f)
			, texOffsets(0.0f, 0.0f, 0.0f, 0.0f)
		{}

		float3 pos;
		float2 size;
		float rot;
		float alpha;
		float4 texOffsets;
	};

	struct BuildingGroundDecal { //FIXME
		BuildingGroundDecal()
			: owner(NULL)
			, gbOwner(NULL)
			, size(0,0)
			, facing(-1)
			, pos(ZeroVector)
			, radius(0.0f)
			, alpha(1.0f)
			, alphaFalloff(1.0f)
		{}

		const CUnit* owner;
		const GhostSolidObject* gbOwner;
		int2 size;
		int facing;
		float3 pos;
		float radius;
		float alpha;
		float alphaFalloff;
	};

private:
	void LoadShaders();
	void GenerateAtlasTexture();

	void CreateBoundingBoxVBOs();
	void CreateStructureVBOs();

	void UpdateVisibilityVBO();
	void UpdateDecalsVBO();

	void DrawDecals();
	//void DrawTracks();

private:
	VBO vboVertices;
	VBO vboIndices;
	VBO uboDecalsStructures;
	VBO uboGroundLighting;
	VBO vboVisibilityFeeback;

	std::vector<Decal*> decals; //FIXME use mt-safe container!
	size_t lastUpdate;

	size_t maxDecals;

	GLuint tbo;
	GLuint depthTex;
	GLuint atlasTex;
	//GLuint decalShader;
	Shader::IProgramObject* decalShader;
};

#endif // !defined(GL_VERSION_4_0) || HEADLESS

#endif // SHADER_GROUND_DECAL_DRAWER_H
