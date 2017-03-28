/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StaticMoveType.h"
#include "Map/Ground.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Sync/HsiehHash.h"

CR_BIND_DERIVED(CStaticMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CStaticMoveType, (
	CR_IGNORED(boolMemberData),
	CR_MEMBER(floatOnWater)
))

CStaticMoveType::CStaticMoveType(CUnit* owner):
	AMoveType(owner),
	floatOnWater(false)
{
	// initialize member hashes and pointers needed by SyncedMoveCtrl
	InitMemberData();

	// creg
	if (owner == nullptr)
		return;

	floatOnWater = owner->FloatOnWater(true);
}

void CStaticMoveType::SlowUpdate()
{
	// buildings and pseudo-static units can be transported
	if (owner->GetTransporter() != nullptr)
		return;

	// NOTE:
	//   static buildings don't have any MoveDef instance, hence we need
	//   to get the ground height instead of calling CMoveMath::yLevel()
	// FIXME: intercept heightmapUpdate events and update buildings y-pos only on-demand!
	if (FloatOnWater() && owner->IsInWater()) {
		owner->Move(UpVector * (-owner->waterline - owner->pos.y), true);
	} else {
		owner->Move(UpVector * (CGround::GetHeightReal(owner->pos.x, owner->pos.z) - owner->pos.y), true);
	}
}

void CStaticMoveType::InitMemberData()
{
	#define MEMBER_CHARPTR_HASH(memberName) HsiehHash(memberName, strlen(memberName),     0)
	#define MEMBER_LITERAL_HASH(memberName) HsiehHash(memberName, sizeof(memberName) - 1, 0)
	boolMemberData[0].first = MEMBER_LITERAL_HASH("floatOnWater");
	#undef MEMBER_CHARPTR_HASH
	#undef MEMBER_LITERAL_HASH

	boolMemberData[0].second = &floatOnWater;
}

bool CStaticMoveType::SetMemberValue(unsigned int memberHash, void* memberValue)
{
	// try the generic members first
	if (AMoveType::SetMemberValue(memberHash, memberValue))
		return true;

	// note: <memberHash> should be calculated via HsiehHash
	// todo: use template lambdas in C++14
	{
		const auto pred = [memberHash](const std::pair<unsigned int, bool*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(boolMemberData.begin(), boolMemberData.end(), pred);
		if (iter != boolMemberData.end()) { *(iter->second) = *(reinterpret_cast<bool*>(memberValue)); return true; }
	}
	/*
	{
		const auto pred = [memberHash](const std::pair<unsigned int, short*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(shortMemberData.begin(), shortMemberData.end(), pred);
		if (iter != shortMemberData.end()) { *(iter->second) = *(reinterpret_cast<short*>(memberValue)); return true; }
	}
	{
		const auto pred = [memberHash](const std::pair<unsigned int, float*>& p) { return (memberHash == p.first); };
		const auto iter = std::find_if(floatMemberData.begin(), floatMemberData.end(), pred);
		if (iter != floatMemberData.end()) { *(iter->second) = *(reinterpret_cast<float*>(memberValue)); return true; }
	}
	*/

	return false;
}

