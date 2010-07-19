/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AIBASE_H
#define AIBASE_H

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Weapons/WeaponDefHandler.h"

// Changing these classes or classes being a member of these classes breaks ABI.
// This also means any engine-side classes not in this list nor being a member
// of one of these classes may not be used by AI code.
// TODO: Use for the Legacy C++ wrapper
#define AI_INTERFACE_GENERATED_VERSION     (\
	  sizeof(CommandDescription) \
	+ sizeof(Command) \
	+ sizeof(WeaponDef) \
	)

#endif // AIBASE_H
