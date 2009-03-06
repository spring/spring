/*
 * aibase.h
 * Base header for ai shared libaries
 * Copyright (C) 2005 Christopher Han
 */
#ifndef AIBASE_H
#define AIBASE_H

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Weapons/WeaponDefHandler.h"

// Changing these classes or classes being a member of these classes breaks ABI.
// This also means any classes not in this list nor being a member of one of
// these classes may not be used by AI code.
// TODO: Use for the Legacy C++ wrapper
#define AI_INTERFACE_GENERATED_VERSION     (\
	  sizeof(CommandDescription) \
	+ sizeof(Command) \
	+ sizeof(UnitDef) \
	+ sizeof(UnitDef::UnitDefWeapon) \
	+ sizeof(WeaponDef) \
	)

#endif /* AIBASE_H */
