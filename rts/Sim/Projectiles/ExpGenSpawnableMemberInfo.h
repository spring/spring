/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_MEMBER_INFO_H
#define EXP_GEN_SPAWNABLE_MEMBER_INFO_H

#include "System/Util.h"

//note the cast to CExpGenSpawnable that should deal with all kinds of inheritance
#define offsetof_expgen(type, member) (size_t)(((char*)&((type *)0xe7707e77)->member) - ((char*)(static_cast<CExpGenSpawnable*>((type *)0xe7707e77))))
#define sizeof_expgen(type, member) sizeof(((type *)0xe7707e77)->member)

#define CHECK_MEMBER_INFO_INT(type, member) \
	static_assert(std::is_integral<decltype(member)>::value, "Member type mismatch"); \
	if (memberName == StringToLower(#member)) { memberInfo = { offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_INT  , nullptr }; return true; }

#define CHECK_MEMBER_INFO_FLOAT(type, member) \
	static_assert(std::is_same<decltype(member), float>::value, "Member type mismatch"); \
	if (memberName == StringToLower(#member)) { memberInfo = { offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr }; return true; }

#define CHECK_MEMBER_INFO_FLOAT3(type, member) \
	static_assert(std::is_same<decltype(member), float3>::value, "Member type mismatch"); \
	if (memberName == StringToLower(#member)) { memberInfo = { offsetof_expgen(type, member), sizeof_expgen(type, member.x), 3, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr }; return true; }

#define CHECK_MEMBER_INFO_FLOAT4(type, member) \
	static_assert(std::is_same<decltype(member), float4>::value, "Member type mismatch"); \
	if (memberName == StringToLower(#member)) { memberInfo = { offsetof_expgen(type, member), sizeof_expgen(type, member.x), 4, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr }; return true; }

#define CHECK_MEMBER_INFO_PTR(t, member, callback) \
	static_assert(std::is_same<decltype(callback("")), decltype(member)>::value, "Member and callback type mismatch"); \
	if (memberName == StringToLower(#member)) { memberInfo = { offsetof_expgen(t, member), sizeof_expgen(t, member), 1, SExpGenSpawnableMemberInfo::TYPE_PTR, [](const std::string& s) { return (void *) callback(s); } }; return true; }

#define CHECK_MEMBER_INFO_BOOL(type, member) CHECK_MEMBER_INFO_INT(type, member)
#define CHECK_MEMBER_INFO_SCOLOR(type, member) CHECK_MEMBER_INFO_INT(type, member.i)




struct SExpGenSpawnableMemberInfo
{
	size_t offset;
	size_t size;
	size_t length; // for arrays etc.
	enum
	{
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_PTR,
	} type;
	std::function<void*(const std::string&)> ptrCallback;
};

#endif //EXP_GEN_SPAWNABLE_MEMBER_INFO_H