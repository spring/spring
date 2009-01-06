#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"
#include "Sim/Units/Unit.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Exceptions.h"

#ifdef _WIN32
#include "Platform/Win/DxSound.h"
#else
#include "Platform/Linux/OpenALSound.h"
#endif
#include "Platform/NullSound.h"


CSound* sound = NULL;


CSound* CSound::GetSoundSystem()
{
	CSound* sound = NULL;

	const int maxSounds = configHandler.Get("MaxSounds", 16);
	if (maxSounds <= 0) {
		logOutput.Print("Sound disabled with \"MaxSounds=%i\"", maxSounds);
		return new CNullSound;
	}

	// try to get a real sound driver
	try {
#ifdef _WIN32
		sound = new CDxSound();
#else
		sound = new COpenALSound();
#endif
	}
	catch (content_error& e) {
		logOutput.Print("Loading sound driver failed, disabling sound");
		logOutput.Print("Error: %s", e.what());
      	
		delete sound;
		sound = new CNullSound;
	}

#ifdef STREFLOP_H
	// Something in DirectSoundCreate (sound drivers?) messes with the FPU control word.
	// Set single precision floating point math.
	streflop_init<streflop::Simple>();
#endif

	return sound;
}


void CSound::PlaySample(int id,CWorldObject* p,float volume)
{
	PlaySample(id,p->pos,volume);
}

void CSound::PlayUnitActivate(int id, CUnit* p, float volume)
{
	PlaySample (id, p, volume);
}

void CSound::PlayUnitReply(int id, CUnit * p, float volume, bool squashDupes)
{
	GML_RECMUTEX_LOCK(sound); // PlayUnitReply

	if (squashDupes) {
		/* HACK Squash multiple command acknowledgements in the same frame, so
		   we aren't deafened by the construction horde, or the metalmaker
		   horde. */

		/* If we've already played the sound this frame, don't play it again. */
		if (repliesPlayed.find(id) != repliesPlayed.end()) {
			return;
		}
		
		repliesPlayed.insert(id);
	}
	
	/* Play the sound at 'full volume'. 
	   TODO Lower the volume if it's off-screen.
	   TODO Give it a location if it's off-screen, but don't lower the volume too much. */
	PlaySample(id, volume * unitReplyVolume);
}

void CSound::NewFrame()
{
	GML_RECMUTEX_LOCK(sound); // NewFrame

	repliesPlayed.clear();
}


void CSound::SetUnitReplyVolume (float vol)
{
	unitReplyVolume = vol;
}

CSound::~CSound() {
}
