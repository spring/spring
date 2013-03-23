/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "TeamController.h" // for swabDWord

CR_BIND(TeamController, );
CR_REG_METADATA(TeamController, (
	CR_MEMBER(team),
	CR_MEMBER(name)
));

CR_BIND(TeamControllerStatistics, );
CR_REG_METADATA(TeamControllerStatistics, (
	CR_MEMBER(numCommands),
	CR_MEMBER(unitCommands)
));
