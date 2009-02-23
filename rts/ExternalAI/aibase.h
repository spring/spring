/*
 * aibase.h
 * Base header for ai shared libaries
 * Copyright (C) 2005 Christopher Han
 */
#ifndef AIBASE_H
#define AIBASE_H

// Shared library support
#ifdef _WIN32
	#define SPRING_API
#elif __GNUC__ >= 4
	// Support for '-fvisibility=hidden'.
	#define SPRING_API __attribute__ ((visibility("default")))
#else
	// Older versions of gcc have everything visible; no need for fancy stuff.
	#define SPRING_API
#endif

// Virtual destructor support (across DLL/SO interface)
#if defined(_WIN32) || defined(__APPLE__)
	// MSVC chokes on (pure) virtual destructors across DLL boundaries,
	// just dont define/declare any.
	// MinGW crashes on pure virtual destructors and doesn't link non
	// pure ones (as below), so we disable them on MinGW too.
	// FIXME Note that this may introduce bugs like invalid vtables,
	// memory leaks because destructors not being called etc.
	#define DECLARE_PURE_VIRTUAL(proto)
	#define IMPLEMENT_PURE_VIRTUAL(proto)
#else
	// GCC on linux is the only config which works fine, but vtable is
	// emitted at place of the implementation of the virtual destructor,
	// so even if it's pure, it must be implemented.
	#define DECLARE_PURE_VIRTUAL(proto) virtual proto = 0;
	#define IMPLEMENT_PURE_VIRTUAL(proto) proto{}
#endif

#include "Interface/aidefines.h"

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"

// Changing these classes or classes being a member of these classes breaks ABI.
// This also means any classes not in this list nor being a member of one of
// these classes may not be used by AI code.
#define AI_INTERFACE_GENERATED_VERSION \
		(sizeof(CommandDescription) + sizeof(Command) + sizeof(UnitDef) + sizeof(UnitDef::UnitDefWeapon) + sizeof(WeaponDef))

#endif /* AIBASE_H */
