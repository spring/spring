/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_BASE_H
#define AI_BASE_H

#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandDescription.h"
#include "WeaponDef.h"

// Changing these classes or classes being a member of these classes breaks ABI.
// This also means any engine-side classes not in this list nor being a member
// of one of these classes may not be used by AI code.
// TODO: Use for the Legacy C++ wrapper
#define AI_INTERFACE_GENERATED_VERSION   \
	(sizeof(SCommandDescription) +       \
	 sizeof(Command) +                   \
	 sizeof(springLegacyAI::WeaponDef))  \


#endif // AI_BASE_H
