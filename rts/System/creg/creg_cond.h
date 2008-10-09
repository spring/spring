/* 
 * Author: hoijui (I am giving the copyright to Jelmer Cnossen)
 *
 * Created on October 9, 2008, 7:07 AM
 */

#ifndef _CREG_COND_H
#define	_CREG_COND_H

// AIs which want to use creg have to specify this when compiling: '-DUSING_CREG'
#if (defined BUILDING_AI || defined BUILDING_AI_INTERFACE) && !defined USING_CREG
	#define NOT_USING_CREG
#else	/* defined BUILDING_AI || defined BUILDING_AI_INTERFACE */
	#ifndef USING_CREG
		#define USING_CREG
	#endif	/* USING_CREG */
	#include "creg/creg.h"
#endif	/* defined BUILDING_AI || defined BUILDING_AI_INTERFACE */

#ifdef NOT_USING_CREG
// define all creg preprocessor macros from creg/creg.h to do nothing
#define CR_DECLARE(TCls)
#define CR_DECLARE_STRUCT(TStr)
#define CR_DECLARE_SUB(cl)
#define CR_BIND_DERIVED(TCls, TBase, ctor_args)
#define CR_BIND_DERIVED(TCls, TBase, ctor_args)
#define CR_BIND(TCls, ctor_args)
#define CR_BIND_DERIVED_INTERFACE(TCls, TBase)
#define CR_BIND_INTERFACE(TCls)
#define CR_REG_METADATA(TClass, Members)
#define CR_REG_METADATA_SUB(TSuperClass, TSubClass, Members)
#define CR_MEMBER(Member)
#define CR_ENUM_MEMBER(Member)
#define CR_RESERVED(Size)
#define CR_MEMBER_SETFLAG(Member, Flag)
#define CR_MEMBER_BEGINFLAG(Flag)
#define CR_MEMBER_ENDFLAG(Flag)
#define CR_SERIALIZER(SerializeFunc)
#define CR_POSTLOAD(PostLoadFunc)
#endif	/* NOT_USING_CREG */

#endif	/* _CREG_COND_H */
