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

CONFIG(bool, UseEFX     ).defaultValue( true).safemodeValue(false);
CONFIG(bool, UseSDLAudio).defaultValue(false).safemodeValue(false);

// defined here so spring-headless contains them, too (default & headless should contain the same set of configtags!)
CONFIG(int, MaxSounds).defaultValue(128).headlessValue(1).minimumValue(1).description("Maximum sounds played in parallel.");
CONFIG(int, PitchAdjust).defaultValue(0).description("Adjusts sound pitch proportional to [if set to 1, the square root of] game speed. Set to 2 for linear scaling.");

CONFIG(int, snd_volmaster).defaultValue(60).minimumValue(0).maximumValue(200).description("Master sound volume.");
CONFIG(int, snd_volgeneral).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"general\" sound channel.");
CONFIG(int, snd_volunitreply).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"unit reply\" sound channel.");
CONFIG(int, snd_volbattle).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"battle\" sound channel.");
CONFIG(int, snd_volui).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"ui\" sound channel.");
CONFIG(int, snd_volmusic).defaultValue(100).minimumValue(0).maximumValue(200).description("Volume for \"music\" sound channel.");
CONFIG(float, snd_airAbsorption).defaultValue(0.1f);

CONFIG(std::string, snd_device).defaultValue("").description("Sets the used output device. See \"Available Devices\" section in infolog.txt.");



#ifndef NO_SOUND
// [0] := Music, [1] := General, [2] := Battle, [3] := UnitReply, [4] := UserInterface
static uint8_t audioChannelMem[5][sizeof(AudioChannel)];
#else
static uint8_t audioChannelMem[5][sizeof(NullAudioChannel)];
#endif


ISound* ISound::singleton = nullptr;

void ISound::Initialize(bool reload, bool forceNullSound)
{
#ifndef NO_SOUND
	if (!IsNullAudio() && !forceNullSound) {
		Channels::BGMusic       = new (audioChannelMem[0]) AudioChannel();
		Channels::General       = new (audioChannelMem[1]) AudioChannel();
		Channels::Battle        = new (audioChannelMem[2]) AudioChannel();
		Channels::UnitReply     = new (audioChannelMem[3]) AudioChannel();
		Channels::UserInterface = new (audioChannelMem[4]) AudioChannel();

		if (!reload) {
			ScopedOnceTimer timer("ISound::Init::New");
			assert(singleton == nullptr);
			singleton = new CSound();
		}
		{
			ScopedOnceTimer timer("ISound::Init::Dev");

			// sound device is initialized in a thread, must wait
			// for it to finish (otherwise LoadSoundDefs can fail)
			singleton->Init();

			while (!singleton->CanLoadSoundDefs()) {
				LOG("[ISound::%s] spawning sound-thread (%.1fms)", __func__, (timer.GetDuration()).toMilliSecsf());

				if (singleton->SoundThreadQuit()) {
					// no device or context found, fallback
					ChangeOutput(true);
					break;
				}

				spring_sleep(spring_msecs(100));
			}
		}
	} else
#endif // NO_SOUND
	{
		if (!reload) {
			assert(singleton == nullptr);
			singleton = new NullSound();
		}

		Channels::BGMusic       = new (audioChannelMem[0]) NullAudioChannel();
		Channels::General       = new (audioChannelMem[1]) NullAudioChannel();
		Channels::Battle        = new (audioChannelMem[2]) NullAudioChannel();
		Channels::UnitReply     = new (audioChannelMem[3]) NullAudioChannel();
		Channels::UserInterface = new (audioChannelMem[4]) NullAudioChannel();
	}
}

void ISound::Shutdown(bool reload)
{
	// kill thread before setting singleton pointer to null
	if (singleton != nullptr)
		singleton->Kill();

	if (!reload)
		spring::SafeDelete(singleton);

	spring::SafeDestruct(Channels::BGMusic);
	spring::SafeDestruct(Channels::General);
	spring::SafeDestruct(Channels::Battle);
	spring::SafeDestruct(Channels::UnitReply);
	spring::SafeDestruct(Channels::UserInterface);

	std::memset(audioChannelMem[0], 0, sizeof(audioChannelMem[0]));
	std::memset(audioChannelMem[1], 0, sizeof(audioChannelMem[1]));
	std::memset(audioChannelMem[2], 0, sizeof(audioChannelMem[2]));
	std::memset(audioChannelMem[3], 0, sizeof(audioChannelMem[3]));
	std::memset(audioChannelMem[4], 0, sizeof(audioChannelMem[4]));
}


bool ISound::IsNullAudio()
{
	return !configHandler->GetBool("Sound");
}


bool ISound::ChangeOutput(bool forceNullSound)
{
	// FIXME: on reload, sound-ids change (depends on order when they are requested, see GetSoundId()/GetSoundItem()
	if (IsNullAudio()) {
		LOG_L(L_ERROR, "[ISound::%s] re-enabling sound isn't supported yet!", __func__);
		return true;
	}

	Shutdown(false);
	configHandler->Set("Sound", IsNullAudio() || forceNullSound);
	Initialize(false, forceNullSound);

	return (IsNullAudio() || forceNullSound);
}

bool ISound::LoadSoundDefs(LuaParser* defsParser)
{
	return (singleton->LoadSoundDefsImpl(defsParser));
}

