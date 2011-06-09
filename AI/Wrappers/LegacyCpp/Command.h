/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#ifndef __COMMAND_H_
#define __COMMAND_H_

// HACK:
//     engine and legacy C++ AI's need to know
//     the *exact* same ABI for struct Command
#undef private
#define private public
#include "Sim/Units/CommandAI/Command.h"
#undef private

#endif
