#include "StdAfx.h"
#include "Sound.h"
#include "GlobalStuff.h"
#include "Sim/Units/Unit.h"

void CSound::PlayUnitActivate(int id, CUnit* p, float volume)
{
	/* If it's my unit, */
	if (p->team == gu->myTeam){
		/* I may have told a bunch of metalmakers to turn off.
		   Only play the sound once. */
		PlayUnitReply(id, p, volume, true);
	}
	else {
		PlaySound(id, p, volume);
	}
}

void CSound::PlayUnitReply(int id, CUnit * p, float volume, bool squashDupes)
{
	if (squashDupes) {
		/* HACK Squash multiple command acknowledgements in the same frame, so
		   we aren't deafened by the construction horde, or the metalmaker
		   horde. */

		std::set<unsigned int>::iterator played;
		
		/* If we've already played the sound this frame, don't play it again. */
		if (repliesPlayed.find(id) != repliesPlayed.end()) {
			return;
		}
		
		repliesPlayed.insert(id);
	}
	
	/* Play the sound at 'full volume'. 
	   TODO Lower the volume if it's off-screen.
	   TODO Give it a location if it's off-screen, but don't lower the volume too much. */
	PlaySound(id, volume);
}

void CSound::NewFrame()
{
	repliesPlayed.clear();
}
