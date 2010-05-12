/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ISound.h"

#include "Sound.h"
#include "SoundLog.h"
#include "System/LogOutput.h"

ISound* ISound::singleton = NULL;

void ISound::Initialize() {

	if (singleton == NULL) {
		singleton = new CSound();
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

