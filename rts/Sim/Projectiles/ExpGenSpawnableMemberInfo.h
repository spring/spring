/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXP_GEN_SPAWNABLE_MEMBER_INFO_H
#define EXP_GEN_SPAWNABLE_MEMBER_INFO_H

#include "System/StringUtil.h"
#include "System/Sync/HsiehHash.h"

#include <functional>

//note the cast to CExpGenSpawnable that should deal with all kinds of inheritance
#define offsetof_expgen(type, member) (size_t)(((char*)&((type *)0xe7707e77)->member) - ((char*)(static_cast<CExpGenSpawnable*>((type *)0xe7707e77))))
#define sizeof_expgen(type, member) sizeof(((type *)0xe7707e77)->member)


// slow, use the CHECK_MEMBER_INFO_*_HASH macros instead
#define STRING_HASH(memberName) HsiehHash(memberName.c_str(), memberName.size(), 0)
#define MEMBER_NAME(member)     StringToLower(#member)
#define MEMBER_HASH(member)     STRING_HASH(std::move(MEMBER_NAME(member)))


struct SExpGenSpawnableMemberInfo
{
	size_t offset;
	size_t size;
	size_t length; // for arrays etc.
	size_t phash;

	enum {
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_PTR,
	} type;

	std::function<void*(const std::string&)> ptrCallback;
};


#define SET_MEMBER_INFO(memberInfo, memberHash, Offset, Size, Length, Type, Ptr)  \
	if (memberInfo.phash == memberHash) {                                         \
		memberInfo.offset      = (Offset);                                        \
		memberInfo.size        = (Size);                                          \
		memberInfo.length      = (Length);                                        \
		memberInfo.type        = (Type);                                          \
		memberInfo.ptrCallback = (Ptr);                                           \
		return true;                                                              \
	}

#define CHECK_MEMBER_INFO_INT(type, member) \
	static_assert(std::is_integral<decltype(member)>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, MEMBER_HASH(member), offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_INT  , nullptr );

#define CHECK_MEMBER_INFO_FLOAT(type, member) \
	static_assert(std::is_same<decltype(member), float>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, MEMBER_HASH(member), offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_FLOAT3(type, member) \
	static_assert(std::is_same<decltype(member), float3>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, MEMBER_HASH(member), offsetof_expgen(type, member), sizeof_expgen(type, member.x), 3, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_FLOAT4(type, member) \
	static_assert(std::is_same<decltype(member), float4>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, MEMBER_HASH(member), offsetof_expgen(type, member), sizeof_expgen(type, member.x), 4, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_PTR(t, member, callback) \
	static_assert(std::is_same<decltype(callback("")), decltype(member)>::value, "Member and callback type mismatch"); \
	SET_MEMBER_INFO(memberInfo, MEMBER_HASH(member), offsetof_expgen(t, member), sizeof_expgen(t, member), 1, SExpGenSpawnableMemberInfo::TYPE_PTR, [](const std::string& s) { return (void *) callback(s.c_str()); } );

#define CHECK_MEMBER_INFO_BOOL(type, member) CHECK_MEMBER_INFO_INT(type, member)
#define CHECK_MEMBER_INFO_SCOLOR(type, member) CHECK_MEMBER_INFO_INT(type, member.i)



#define CHECK_MEMBER_INFO_INT_HASH(type, member, memberHash) \
	static_assert(std::is_integral<decltype(member)>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, memberHash, offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_INT  , nullptr );

#define CHECK_MEMBER_INFO_FLOAT_HASH(type, member, memberHash) \
	static_assert(std::is_same<decltype(member), float>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, memberHash, offsetof_expgen(type, member), sizeof_expgen(type, member)  , 1, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_FLOAT3_HASH(type, member, memberHash) \
	static_assert(std::is_same<decltype(member), float3>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, memberHash, offsetof_expgen(type, member), sizeof_expgen(type, member.x), 3, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_FLOAT4_HASH(type, member, memberHash) \
	static_assert(std::is_same<decltype(member), float4>::value, "Member type mismatch"); \
	SET_MEMBER_INFO(memberInfo, memberHash, offsetof_expgen(type, member), sizeof_expgen(type, member.x), 4, SExpGenSpawnableMemberInfo::TYPE_FLOAT, nullptr );

#define CHECK_MEMBER_INFO_PTR_HASH(t, member, callback, memberHash) \
	static_assert(std::is_same<decltype(callback("")), decltype(member)>::value, "Member and callback type mismatch"); \
	SET_MEMBER_INFO(memberInfo, memberHash, offsetof_expgen(t, member), sizeof_expgen(t, member), 1, SExpGenSpawnableMemberInfo::TYPE_PTR, [](const std::string& s) { return (void *) callback(s); } );

#define CHECK_MEMBER_INFO_BOOL_HASH(type, member, memberHash) CHECK_MEMBER_INFO_INT_HASH(type, member, memberHash)
#define CHECK_MEMBER_INFO_SCOLOR_HASH(type, member, memberHash) CHECK_MEMBER_INFO_INT_HASH(type, member.i, memberHash)



#endif //EXP_GEN_SPAWNABLE_MEMBER_INFO_H
