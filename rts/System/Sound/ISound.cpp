/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ISound.h"

#ifndef   NO_SOUND
#include "Sound.h"
#endif // NO_SOUND
#include "NullSound.h"

#include "SoundLog.h"
#include "System/LogOutput.h"
#include "System/ConfigHandler.h"

ISound* ISound::singleton = NULL;

void ISound::Initialize() {

	if (singleton == NULL) {
#ifndef NO_SOUND
		const bool noSound = configHandler->Get("NoSound", false);
		if (!noSound) {
			singleton = new CSound();
		} else
#endif // NO_SOUND
		{
			singleton = new NullSound();
		}
	} else {
		LogObject(LOG_SOUND) <<  "warning: Sound is already initialized!";
	}
}

void ISound::Shutdown() {

	ISound* tmpSound = singleton;
	singleton = NULL;
	delete tmpSound;
	tmpSound = NULL;
}

bool ISound::IsInitialized() {
	return (singleton != NULL);
}

