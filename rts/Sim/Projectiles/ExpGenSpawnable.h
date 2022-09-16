/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_H
#define EXP_GEN_SPAWNABLE_H

#include <memory>

#include "Sim/Objects/WorldObject.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GL/RenderBuffersFwd.h"

struct SExpGenSpawnableMemberInfo;
class CUnit;

class CExpGenSpawnable : public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable)
public:
	CExpGenSpawnable(const float3& pos, const float3& spd);

	~CExpGenSpawnable() override;
	virtual void Init(const CUnit* owner, const float3& offset);

	static bool GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo);
	static int GetSpawnableID(const std::string& spawnableName);

	//Memory handled in projectileHandler
	static CExpGenSpawnable* CreateSpawnable(int spawnableID);
	static TypedRenderBuffer<VA_TYPE_PROJ>& GetPrimaryRenderBuffer();
protected:
	CExpGenSpawnable();

	//update in Draw() of CGroundFlash or CProjectile
	void UpdateRotation();
	void UpdateAnimParams();

	void AddEffectsQuad(const VA_TYPE_TC& tl, const VA_TYPE_TC& tr, const VA_TYPE_TC& br, const VA_TYPE_TC& bl) const;

	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);

	float3 animParams = { 1.0f, 1.0f, 30.0f }; // numX, numY, animLength, 
	float animProgress = 0.0f; // animProgress = (gf_dt % animLength) / animLength

	float3 rotParams = { 0.0f, 0.0f, 0.0f }; // speed, accel, startRot |deg/s, deg/s2, deg|

	float rotVal;
	float rotVel;

	int createFrame;
};

#endif //EXP_GEN_SPAWNABLE_H
