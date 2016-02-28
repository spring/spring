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
#include "System/Misc/SpringTime.h" //FIXME: remove this

CONFIG(bool, Sound).defaultValue(true).description("Select the Sound driver, true = OpenAL, false = NullAudio");

// defined here so spring-headless contains them, too (default & headless should contain the same set of configtags!)
CONFIG(int, MaxSounds).defaultValue(128).headlessValue(1).minimumValue(1).description("Maximum parallel played sounds.");
CONFIG(bool, PitchAdjust).defaultValue(false).description("When enabled adjust sound speed/pitch to game speed.");
CONFIG(int, snd_volmaster).defaultValue(60).minimumValue(0).maximumValue(200).description("Master sound volume.");
CONFIG(int, snd_volgeneral).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"general\" sound channel.");
CONFIG(int, snd_volunitreply).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"unit reply\" sound channel.");
CONFIG(int, snd_volbattle).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"battle\" sound channel.");
CONFIG(int, snd_volui).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"ui\" sound channel.");
CONFIG(int, snd_volmusic).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"music\" sound channel.");
CONFIG(std::string, snd_device).defaultValue("").description("Sets the used output device. See \"Available Devices\" section in infolog.txt.");

CONFIG(float, snd_airAbsorption).defaultValue(0.1f);
CONFIG(bool, UseEFX).defaultValue(true).safemodeValue(false);


ISound* ISound::singleton = NULL;

ISound::ISound()
	: numEmptyPlayRequests(0)
	, numAbortedPlays(0)
{
}

void ISound::Initialize(bool forceNullSound)
{
	if (singleton == NULL) {
#ifndef NO_SOUND
		if (!IsNullAudio() && !forceNullSound) {
			Channels::BGMusic = new AudioChannel();
			Channels::General = new AudioChannel();
			Channels::Battle = new AudioChannel();
			Channels::UnitReply = new AudioChannel();
			Channels::UserInterface = new AudioChannel();
			singleton = new CSound();

			// sound device is initialized in a thread, must wait
			// for it to finish (otherwise LoadSoundDefs can fail)
			while (!singleton->CanLoadSoundDefs()) {
				spring_sleep(spring_msecs(100));

				if (singleton->SoundThreadQuit()) {
					// no device or context found, fallback
					ChangeOutput(true); break;
				}
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


bool ISound::IsNullAudio()
{
	return !configHandler->GetBool("Sound");
}


bool ISound::ChangeOutput(bool forceNullSound)
{
	if (IsNullAudio()) {
		//FIXME: on reload, sound-ids change (depends on order when they are requested, see GetSoundId()/GetSoundItem()
		LOG_L(L_ERROR, "re-enabling sound isn't supported yet, expect problems!");
	}
	Shutdown();
	configHandler->Set("Sound", IsNullAudio() || forceNullSound);
	Initialize(forceNullSound);
	return (IsNullAudio() || forceNullSound);
}

bool ISound::LoadSoundDefs(const std::string& filename, const std::string& modes)
{
	return (singleton->LoadSoundDefsImpl(filename, modes));
}

