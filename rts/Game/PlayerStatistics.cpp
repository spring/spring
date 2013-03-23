/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PlayerStatistics.h"

#include "System/Platform/byteorder.h"

CR_BIND_DERIVED(PlayerStatistics, TeamControllerStatistics, );
CR_REG_METADATA(PlayerStatistics, (
	CR_MEMBER(mousePixels),
	CR_MEMBER(mouseClicks),
	CR_MEMBER(keyPresses)
));


PlayerStatistics::PlayerStatistics()
	: mousePixels(0)
	, mouseClicks(0)
	, keyPresses(0)
{
}

void PlayerStatistics::swab()
{
	swabDWordInPlace(mousePixels);
	swabDWordInPlace(mouseClicks);
	swabDWordInPlace(keyPresses);
	TeamControllerStatistics::swab();
}
