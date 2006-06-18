#include "StdAfx.h"
#include "Sound.h"
#include "GlobalStuff.h"
#include "Sim/Units/Unit.h"


void CSound::PlaySound(int id,CWorldObject* p,float volume)
{
	PlaySound(id,p->pos,volume);
}

void CSound::PlayUnitActivate(int id, CUnit* p, float volume)
{
	PlaySound (id, p, volume);
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
	PlaySound(id, volume * unitReplyVolume);
}

void CSound::NewFrame()
{
	repliesPlayed.clear();
}


void CSound::SetUnitReplyVolume (float vol)
{
	unitReplyVolume = vol;
}
