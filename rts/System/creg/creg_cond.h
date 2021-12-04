/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CREG_COND_H_
#define _CREG_COND_H_

// AIs which want to use creg have to specify this when compiling:
// '-DUSING_CREG'

#if defined BUILDING_AI && !defined USING_CREG
	#if !defined NOT_USING_CREG
		#define NOT_USING_CREG
	#endif
#elif !defined NOT_USING_CREG // defined BUILDING_AI && !defined USING_CREG
	#if !defined USING_CREG
		#define USING_CREG
	#endif // !defined USING_CREG
	#include "creg.h"
#endif // defined BUILDING_AI && !defined USING_CREG

#ifdef NOT_USING_CREG
#undef USING_CREG
#include "ISerializer.h" // prevent some compiler errors
// define all creg preprocessor macros from creg_cond.h to nothing
#define CR_DECLARE(TCls)
#define CR_DECLARE_DERIVED(TCls)
#define CR_DECLARE_STRUCT(TStr)
#define CR_DECLARE_SUB(cl)
#define CR_BIND_DERIVED(TCls, TBase, ctor_args)
#define CR_BIND(TCls, ctor_args)
#define CR_BIND_DERIVED_INTERFACE(TCls, TBase)
#define CR_BIND_DERIVED_INTERFACE_POOL(TCls, TBase, poolAlloc, poolFree)
#define CR_BIND_DERIVED_POOL(TCls, TBase, ctor_args, poolAlloc, poolFree)
#define CR_BIND_INTERFACE(TCls)
#define CR_BIND_TEMPLATE(TCls, ctor_args)
#define CR_REG_METADATA(TClass, Members)
#define CR_REG_METADATA_SUB(TSuperClass, TSubClass, Members)
#define CR_REG_METADATA_TEMPLATE(TCls, Members)
#define CR_MEMBER(Member)
#define CR_IGNORED(Member)
#define CR_MEMBER_UN(Member)
#define CR_SETFLAG(Flag)
#define CR_MEMBER_SETFLAG(Member, Flag)
#define CR_MEMBER_BEGINFLAG(Flag)
#define CR_MEMBER_ENDFLAG(Flag)
#define CR_SERIALIZER(SerializeFunc)
#define CR_POSTLOAD(PostLoadFunc)
#endif // NOT_USING_CREG

#endif // _CREG_COND_H_
