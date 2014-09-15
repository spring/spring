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
#include <list>

CONFIG(bool, Sound).defaultValue(true).description("Select the Sound driver, true = OpenAL, false = NullAudio");


ISound* ISound::singleton = NULL;

ISound::ISound()
	: numEmptyPlayRequests(0)
	, numAbortedPlays(0)
{
}

static std::list<std::string> sounddefs;

void ISound::Initialize()
{
	if (singleton == NULL) {
#ifndef NO_SOUND
		if (!IsNullAudio()) {
			Channels::BGMusic = new AudioChannel();
			Channels::General = new AudioChannel();
			Channels::Battle = new AudioChannel();
			Channels::UnitReply = new AudioChannel();
			Channels::UserInterface = new AudioChannel();
			singleton = new CSound();
			for(const std::string& filename: sounddefs) {
				sleep(1); //FIXME BADHACK: the sound device is initialized asynchron in a thread this is why loading sounds instantly fails
				singleton->LoadSoundDefsImpl(filename);
			}
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

bool ISound::IsNullAudio()
{
	return !configHandler->GetBool("Sound");
}


bool ISound::ChangeOutput()
{
	if (IsNullAudio()) {
		//FIXME: on reload, sound-ids change (depends on order when they are requested, see GetSoundId()/GetSoundItem()
		LOG_L(L_ERROR, "re-enabling sound isn't supported yet, expect problems!");
	}
	Shutdown();
	configHandler->Set("Sound", IsNullAudio());
	Initialize();
	return IsNullAudio();
}

bool ISound::LoadSoundDefs(const std::string& filename)
{
	sounddefs.push_back(filename);
	return singleton->LoadSoundDefsImpl(filename);
}

