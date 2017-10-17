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

#include "System/TimeProfiler.h"
#include "System/SafeUtil.h"

CONFIG(bool, Sound).defaultValue(true).description("Enables (OpenAL) or disables sound.");

// defined here so spring-headless contains them, too (default & headless should contain the same set of configtags!)
CONFIG(int, MaxSounds).defaultValue(128).headlessValue(1).minimumValue(1).description("Maximum sounds played in parallel.");
CONFIG(int, PitchAdjust).defaultValue(0).description("Adjusts sound pitch proportional to [if set to 1, the square root of] game speed. Set to 2 for linear scaling.");

CONFIG(int, snd_volmaster).defaultValue(60).minimumValue(0).maximumValue(200).description("Master sound volume.");
CONFIG(int, snd_volgeneral).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"general\" sound channel.");
CONFIG(int, snd_volunitreply).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"unit reply\" sound channel.");
CONFIG(int, snd_volbattle).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"battle\" sound channel.");
CONFIG(int, snd_volui).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"ui\" sound channel.");
CONFIG(int, snd_volmusic).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"music\" sound channel.");
CONFIG(std::string, snd_device).defaultValue("").description("Sets the used output device. See \"Available Devices\" section in infolog.txt.");

CONFIG(float, snd_airAbsorption).defaultValue(0.1f);
CONFIG(bool, UseEFX).defaultValue(true).safemodeValue(false);


ISound* ISound::singleton = nullptr;

ISound::ISound()
	: numEmptyPlayRequests(0)
	, numAbortedPlays(0)
{
}

void ISound::Initialize(bool forceNullSound)
{
	if (singleton != nullptr) {
		LOG_L(L_WARNING, "[ISound::%s] already initialized!", __func__);
		return;
	}

#ifndef NO_SOUND
	if (!IsNullAudio() && !forceNullSound) {
		Channels::BGMusic = new AudioChannel();
		Channels::General = new AudioChannel();
		Channels::Battle = new AudioChannel();
		Channels::UnitReply = new AudioChannel();
		Channels::UserInterface = new AudioChannel();

		{
			ScopedOnceTimer timer("ISound::Init::New");
			singleton = new CSound();
		}
		{
			ScopedOnceTimer timer("ISound::Init::Dev");

			// sound device is initialized in a thread, must wait
			// for it to finish (otherwise LoadSoundDefs can fail)
			while (!singleton->CanLoadSoundDefs()) {
				LOG("[ISound::%s] spawning sound-thread (%.1fms)", __func__, (timer.GetDuration()).toMilliSecsf());

				if (singleton->SoundThreadQuit()) {
					// no device or context found, fallback
					ChangeOutput(true); break;
				}

				spring_sleep(spring_msecs(100));
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
}

void ISound::Shutdown()
{
	spring::SafeDelete(singleton);

	spring::SafeDelete(Channels::BGMusic);
	spring::SafeDelete(Channels::General);
	spring::SafeDelete(Channels::Battle);
	spring::SafeDelete(Channels::UnitReply);
	spring::SafeDelete(Channels::UserInterface);
}


bool ISound::IsNullAudio()
{
	return !configHandler->GetBool("Sound");
}


bool ISound::ChangeOutput(bool forceNullSound)
{
	// FIXME: on reload, sound-ids change (depends on order when they are requested, see GetSoundId()/GetSoundItem()
	if (IsNullAudio())
		LOG_L(L_ERROR, "[ISound::%s] re-enabling sound isn't supported yet, expect problems!", __func__);

	Shutdown();
	configHandler->Set("Sound", IsNullAudio() || forceNullSound);
	Initialize(forceNullSound);
	return (IsNullAudio() || forceNullSound);
}

bool ISound::LoadSoundDefs(const std::string& filename, const std::string& modes)
{
	return (singleton->LoadSoundDefsImpl(filename, modes));
}

