/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ISound.h"

#ifndef   NO_SOUND
#include "OpenAL/Sound.h"
#endif // NO_SOUND
#include "Null/NullSound.h"

#include "SoundLog.h"
#include "System/Config/ConfigHandler.h"

#include "ISoundChannels.h"
#include "Null/NullAudioChannel.h"
#ifndef NO_SOUND
#include "OpenAL/AudioChannel.h"
#endif

CONFIG(bool, NoSound).defaultValue(false);


ISound* ISound::singleton = NULL;

ISound::ISound()
	: numEmptyPlayRequests(0)
	, numAbortedPlays(0)
{
}

void ISound::Initialize()
{
	if (singleton == NULL) {
#ifndef NO_SOUND
		const bool noSound = configHandler->GetBool("NoSound");
		if (!noSound) {
			Channels::BGMusic = new AudioChannel();
			Channels::General = new AudioChannel();
			Channels::Battle = new AudioChannel();
			Channels::UnitReply = new AudioChannel();
			Channels::UserInterface = new AudioChannel();
			singleton = new CSound();
		} else
#endif // NO_SOUND
		{
			Channels::BGMusic = new NullAudioChannel();
			Channels::General = new NullAudioChannel();
			Channels::Battle = new NullAudioChannel();
			Channels::UnitReply = new NullAudioChannel();
			Channels::UserInterface = new NullAudioChannel();
			singleton = new NullSound();
		}
	} else {
		LOG_L(L_WARNING, "Sound is already initialized!");
	}
}

#define SafeDelete(var) \
	delete var; \
	var = NULL;

void ISound::Shutdown()
{
	ISound* tmpSound = singleton;
	singleton = NULL;
	SafeDelete(tmpSound);

	SafeDelete(Channels::BGMusic);
	SafeDelete(Channels::General);
	SafeDelete(Channels::Battle);
	SafeDelete(Channels::UnitReply);
	SafeDelete(Channels::UserInterface);

}


bool ISound::IsInitialized()
{
	return (singleton != NULL);
}

