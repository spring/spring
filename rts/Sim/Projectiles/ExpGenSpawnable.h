/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_H
#define EXP_GEN_SPAWNABLE_H

#include "Sim/Objects/WorldObject.h"

struct SExpGenSpawnableMemberInfo;
class CUnit;

class CExpGenSpawnable: public CWorldObject
{
	CR_DECLARE(CExpGenSpawnable)
public:
	CExpGenSpawnable(const float3& pos, const float3& spd);

	virtual ~CExpGenSpawnable();
	virtual void Init(const CUnit* owner, const float3& offset);

	static bool GetSpawnableMemberInfo(const std::string& spawnableName, SExpGenSpawnableMemberInfo& memberInfo);
	static int GetSpawnableID(const std::string& spawnableName);

	//Memory handled in projectileHandler
	static CExpGenSpawnable* CreateSpawnable(int spawnableID);

protected:
	float3 rotParams = { 0.0f, 0.0f, 0.0f }; // speed, accel, startRot |deg/s, deg/s2, deg|

	CExpGenSpawnable();
	static bool GetMemberInfo(SExpGenSpawnableMemberInfo& memberInfo);
};

#endif //EXP_GEN_SPAWNABLE_H
