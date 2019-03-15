/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sound.h"

#include <cstdlib>
#include <cmath>
#include <al.h>
#include <alc.h>
#include <alext.h>

#ifdef ALC_SOFT_loopback
#include <SDL.h>
#endif

#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
// needed for ALC_ALL_DEVICES_SPECIFIER on some special *nix
// #include <alext.h>
#endif

#include <climits>
#include <cinttypes>
#include <functional>

#include "System/Sound/ISoundChannels.h"
#include "System/Sound/SoundLog.h"
#include "SoundSource.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "ALShared.h"
#include "EFX.h"
#include "EFXPresets.h"

#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/SpringMath.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"
#include "System/Threading/SpringThreading.h"

#include "System/float3.h"


spring::recursive_mutex soundMutex;


CSound::CSound()
{
	configHandler->NotifyOnChange(this, {"snd_volmaster", "snd_eaxpreset", "snd_filter", "UseEFX", "snd_volgeneral", "snd_volunitreply", "snd_volbattle", "snd_volui", "snd_volmusic", "PitchAdjust"});
}

CSound::~CSound()
{
	configHandler->RemoveObserver(this);
}


void CSound::Init()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	{
		curDevice = nullptr;
		curContext = nullptr;

		sdlDeviceID = 0;

		masterVolume = configHandler->GetInt("snd_volmaster") * 0.01f;
		pitchAdjustMode = configHandler->GetInt("PitchAdjust");
		frameSize = -1;

		mute = false;
		appIsIconified = false;

		updateListener = false;
		soundThreadQuit = false;
		canLoadDefs = false;
	}
	{
		Channels::General->SetVolume(configHandler->GetInt("snd_volgeneral") * 0.01f);
		Channels::UnitReply->SetVolume(configHandler->GetInt("snd_volunitreply") * 0.01f);
		Channels::UnitReply->SetMaxConcurrent(1);
		Channels::UnitReply->SetMaxEmits(1);
		Channels::Battle->SetVolume(configHandler->GetInt("snd_volbattle") * 0.01f);
		Channels::UserInterface->SetVolume(configHandler->GetInt("snd_volui") * 0.01f);
		Channels::BGMusic->SetVolume(configHandler->GetInt("snd_volmusic") * 0.01f);
	}
	{
		SoundBuffer::Initialise();

		soundMap.clear();
		soundMap.reserve(256);
		preloadSet.clear();
		preloadSet.reserve(16);
		failureSet.clear();

		defaultItemNameMap.clear();
		soundItemDefsMap.clear();

		// sources are cleared and reserved by GenSources
		soundItems.clear();
		soundItems.reserve(1024);
		soundItems.emplace_back();
	}

	soundThread = std::move(Threading::CreateNewThread(std::bind(&CSound::UpdateThread, this, configHandler->GetInt("MaxSounds"))));
}

void CSound::Kill()
{
	LOG("[Sound::%s] soundThread.joinable()=%d", __func__, soundThread.joinable());

	{
		soundThreadQuit = true;

		if (soundThread.joinable())
			soundThread.join();
	}

	SoundBuffer::Deinitialise();
}


void CSound::Cleanup() {
	if (curContext != nullptr) {
		LOG("[Sound::%s][alcDestroyContext(%p)]", __func__, curContext);
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(curContext);
		curContext = nullptr;
	}

	if (curDevice != nullptr) {
		LOG("[Sound::%s][alcCloseDevice(%p)]", __func__, curDevice);
		alcCloseDevice(curDevice);
		curDevice = nullptr;
	}

#ifdef ALC_SOFT_loopback
	if (sdlDeviceID != 0) {
		LOG("[Sound::%s][SDL_CloseAudioDevice(%d)]", __func__, sdlDeviceID);
		SDL_CloseAudioDevice(sdlDeviceID);
		SDL_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);

		sdlDeviceID = 0;
	}
#endif
}


bool CSound::HasSoundItem(const std::string& name) const
{
	// soundMap can be concurrently touched by GetSoundId if preloading
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (soundMap.find(name) != soundMap.end())
		return true;

	return (soundItemDefsMap.find(StringToLower(name)) != soundItemDefsMap.end());
}

bool CSound::PreloadSoundItem(const std::string& name)
{
	#if 0
	ThreadPool::Enqueue([name]() { sound->GetSoundId(name); });
	#else
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);
	return ((preloadSet.insert(name)).second);
	#endif
}


size_t CSound::GetDefSoundId(const std::string& name)
{
	// only attempt to load if sounds.lua has an entry for this sound
	if (!HasSoundItem(name))
		return 0;

	return (GetSoundId(name));
}

size_t CSound::GetSoundId(const std::string& name)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	// do not preload-loop forever, erase even if the sound fails to load
	// note: this breaks the name reference, has to be done when returning
	// regular GetSoundId calls will pre-empt any preload items
	if (soundSources.empty())
		return (preloadSet.erase(name), 0);

	const auto soundMapIt = soundMap.find(name);
	if (soundMapIt != soundMap.end())
		return (preloadSet.erase(name), soundMapIt->second);

	const auto itemDefIt = soundItemDefsMap.find(StringToLower(name));

	if (itemDefIt != soundItemDefsMap.end())
		return (preloadSet.erase(name), MakeItemFromDef(itemDefIt->second));

	// name does not match any sounds.lua item, interpret as raw file reference
	if (LoadSoundBuffer(name) > 0) {
		SoundItemNameMap itemMap = defaultItemNameMap;
		itemMap.erase("file");
		itemMap.insert("file", name);
		return (preloadSet.erase(name), MakeItemFromDef(itemMap));
	}

	LOG_L(L_ERROR, "[Sound::%s] could not find sound \"%s\"", __func__, name.c_str());
	return (preloadSet.erase(name), 0);
}


SoundItem* CSound::GetSoundItem(size_t id) {
	// id==0 is a special id and invalid
	if (id == 0 || id >= soundItems.size())
		return nullptr;

	// WARNING: may not be leaked, soundItems vector grows on-demand via GetSoundId -> MakeItemFromDef
	return &soundItems[id];
}

CSoundSource* CSound::GetNextBestSource(bool lock)
{
	std::unique_lock<spring::recursive_mutex> lck(soundMutex, std::defer_lock);
	if (lock)
		lck.lock();

	if (soundSources.empty())
		return nullptr;

	// find a free source; pointer remains valid until thread exits
	const auto pred = [](const CSoundSource& src) { return (!src.IsPlaying(false)); };
	const auto iter = std::find_if(soundSources.begin(), soundSources.end(), pred);

	if (iter != soundSources.end())
		return &(*iter);

	// check the next best free source
	CSoundSource* bestSrc = nullptr;
	int bestPriority = INT_MAX;

	for (CSoundSource& src: soundSources) {
		#if 0
		if (!src.IsPlaying(true))
			return &src;
		#endif
		if (src.GetCurrentPriority() > bestPriority)
			continue;

		bestSrc = &src;
		bestPriority = src.GetCurrentPriority();
	}

	return bestSrc;
}

void CSound::PitchAdjust(const float newPitch)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	switch (pitchAdjustMode) {
		case  1: { CSoundSource::SetPitch(std::sqrt(newPitch)); } break;
		case  2: { CSoundSource::SetPitch(          newPitch ); } break;
		default: {                                              } break;
	}
}

void CSound::ConfigNotify(const std::string& key, const std::string& value)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	switch (hashString(key.c_str())) {
		case hashString("snd_volmaster"): {
			masterVolume = std::atoi(value.c_str()) * 0.01f;

			if (!mute && !appIsIconified)
				alListenerf(AL_GAIN, masterVolume);

		} break;
		case hashString("snd_eaxpreset"): {
			efx.SetPreset(value);
		} break;
		case hashString("snd_filter"): {
			float gainlf = 1.0f;
			float gainhf = 1.0f;
			sscanf(value.c_str(), "%f %f", &gainlf, &gainhf);
			efx.sfxProperties.filter_props_f[AL_LOWPASS_GAIN  ] = gainlf;
			efx.sfxProperties.filter_props_f[AL_LOWPASS_GAINHF] = gainhf;
			efx.CommitEffects();
		} break;

		case hashString("UseEFX"): {
			if (std::atoi(value.c_str()) != 0) {
				efx.Enable();
			} else {
				efx.Disable();
			}
		} break;

		case hashString("snd_volgeneral"): {
			Channels::General->SetVolume(std::atoi(value.c_str()) * 0.01f);
		} break;
		case hashString("snd_volunitreply"): {
			Channels::UnitReply->SetVolume(std::atoi(value.c_str()) * 0.01f);
		} break;
		case hashString("snd_volbattle"): {
			Channels::Battle->SetVolume(std::atoi(value.c_str()) * 0.01f);
		} break;
		case hashString("snd_volui"): {
			Channels::UserInterface->SetVolume(std::atoi(value.c_str()) * 0.01f);
		} break;
		case hashString("snd_volmusic"): {
			Channels::BGMusic->SetVolume(std::atoi(value.c_str()) * 0.01f);
		} break;

		case hashString("PitchAdjust"): {
			const int tempPitchAdjustMode = std::atoi(value.c_str());

			// reset adjustment factor if disabling
			if (tempPitchAdjustMode == 0)
				PitchAdjust(1.0f);

			pitchAdjustMode = tempPitchAdjustMode;
		} break;

		default: {
		} break;
	}
}

bool CSound::Mute()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if ((mute = !mute))
		alListenerf(AL_GAIN, 0.0f);
	else
		alListenerf(AL_GAIN, masterVolume);

	return mute;
}

void CSound::Iconified(bool state)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (appIsIconified != state && !mute) {
		if (!state)
			alListenerf(AL_GAIN, masterVolume);
		else
			alListenerf(AL_GAIN, 0.0);
	}

	appIsIconified = state;
}


void CSound::OpenOpenALDevice(const std::string& deviceName)
{
	assert(curDevice == nullptr);

	if (!deviceName.empty()) {
		LOG("[Sound::%s] opening configured device \"%s\"", __func__, deviceName.c_str());

		curDevice = alcOpenDevice(deviceName.c_str());
	}

	if (curDevice == nullptr) {
		LOG("[Sound::%s] opening default device \"%s\"", __func__, alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER));

		curDevice = alcOpenDevice(nullptr);
	}

	if (curDevice == nullptr) {
		LOG_L(L_ERROR, "[%s][2] failed to open device", __func__);

		// fall back to NullSound
		Cleanup();

		soundThreadQuit = true;
		return;
	}

	curContext = alcCreateContext(curDevice, nullptr);
	LOG("[Sound::%s] device=%p context=%p", __func__, curDevice, curContext);
}



#ifdef ALC_SOFT_loopback

static LPALCLOOPBACKOPENDEVICESOFT alcLoopbackOpenDeviceSOFT;
static LPALCISRENDERFORMATSUPPORTEDSOFT alcIsRenderFormatSupportedSOFT;
static LPALCRENDERSAMPLESSOFT alcRenderSamplesSOFT;

static void SDLCALL RenderSDLSamples(void* userdata, Uint8* stream, int len)
{
	CSound* snd = reinterpret_cast<CSound*>(userdata);
	ALCdevice* dev = snd->GetCurrentDevice();

	assert(snd->GetFrameSize() > 0);
	alcRenderSamplesSOFT(dev, stream, len / snd->GetFrameSize());
}

static const char* ChannelsName(ALCenum chans)
{
	switch (chans) {
		case ALC_MONO_SOFT: return "Mono";
		case ALC_STEREO_SOFT: return "Stereo";
		case ALC_QUAD_SOFT: return "Quadraphonic";
		case ALC_5POINT1_SOFT: return "5.1 Surround";
		case ALC_6POINT1_SOFT: return "6.1 Surround";
		case ALC_7POINT1_SOFT: return "7.1 Surround";
	}
	return "Unknown Channels";
}

static const char* TypeName(ALCenum type)
{
	switch (type) {
		case ALC_BYTE_SOFT: return "S8";
		case ALC_UNSIGNED_BYTE_SOFT: return "U8";
		case ALC_SHORT_SOFT: return "S16";
		case ALC_UNSIGNED_SHORT_SOFT: return "U16";
		case ALC_INT_SOFT: return "S32";
		case ALC_UNSIGNED_INT_SOFT: return "U32";
		case ALC_FLOAT_SOFT: return "Float32";
	}
	return "Unknown Type";
}

#endif



void CSound::OpenLoopbackDevice(const std::string& deviceName)
{
	assert(curDevice == nullptr);

#ifdef ALC_SOFT_loopback

#define LOAD_PROC(x) ((x) = (decltype(x)) alcGetProcAddress(nullptr, #x))
	LOAD_PROC(alcLoopbackOpenDeviceSOFT);
	LOAD_PROC(alcIsRenderFormatSupportedSOFT);
	LOAD_PROC(alcRenderSamplesSOFT);
#undef LOAD_PROC

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
		LOG("[Sound::%s] failed to initialize SDL audio, error:  \"%s\"", __func__, SDL_GetError());
		return;
	}

	SDL_AudioSpec desiredSpec;
	SDL_AudioSpec obtainedSpec;

	desiredSpec.channels = 2;
	desiredSpec.format = AUDIO_S16SYS;
	desiredSpec.freq = 44100;
	desiredSpec.padding = 0;
	desiredSpec.samples = 4096;
	desiredSpec.callback = RenderSDLSamples;
	desiredSpec.userdata = this;


	sdlDeviceID = 0;

	if (!deviceName.empty()) {
		LOG("[Sound::%s] opening configured device \"%s\"", __func__, deviceName.c_str());
		sdlDeviceID = SDL_OpenAudioDevice(deviceName.c_str(), 0, &desiredSpec, &obtainedSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	}

	if (sdlDeviceID == 0) {
		LOG("[Sound::%s] opening default device", __func__);
		sdlDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &obtainedSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	}

	if (sdlDeviceID == 0) {
		LOG("[Sound::%s] failed to open SDL audio, error:  \"%s\"", __func__, SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	// needs to be at least 1 or the callback will divide by 0
	if ((frameSize = obtainedSpec.channels * SDL_AUDIO_BITSIZE(obtainedSpec.format) / 8) <= 0) {
		LOG("[Sound::%s] failed to obtain valid SDL spec: numChannels=%d formatBits=%d", __func__, obtainedSpec.channels, SDL_AUDIO_BITSIZE(obtainedSpec.format));
		Cleanup();
		return;
	}


	ALCint attrs[16];

	// set up our OpenAL attributes based on what we got from SDL
	attrs[0] = ALC_FORMAT_CHANNELS_SOFT;

	switch (obtainedSpec.channels) {
		case 1: { attrs[1] = ALC_MONO_SOFT  ; } break;
		case 2: { attrs[1] = ALC_STEREO_SOFT; } break;
		default: {
			LOG("[Sound::%s] unhandled SDL channel count: %d", __func__, obtainedSpec.channels);
			Cleanup();
			return;
		} break;
	}

	attrs[2] = ALC_FORMAT_TYPE_SOFT;

	switch (obtainedSpec.format) {
		case AUDIO_U8    : { attrs[3] = ALC_UNSIGNED_BYTE_SOFT ; } break;
		case AUDIO_S8    : { attrs[3] = ALC_BYTE_SOFT          ; } break;
		case AUDIO_U16SYS: { attrs[3] = ALC_UNSIGNED_SHORT_SOFT; } break;
		case AUDIO_S16SYS: { attrs[3] = ALC_SHORT_SOFT         ; } break;
		default: {
			LOG("[Sound::%s] unhandled SDL format: 0x%04x", __func__, obtainedSpec.format);
			Cleanup();
			return;
		} break;
	}

	attrs[4] = ALC_FREQUENCY;
	attrs[5] = obtainedSpec.freq;
	attrs[6] = 0; // end of list


	// initialize OpenAL loopback device, using our format attributes
	if ((curDevice = alcLoopbackOpenDeviceSOFT(nullptr)) == nullptr) {
		LOG("[Sound::%s] failed to create loopback device", __func__);
		Cleanup();
		return;
	}

	// make sure the format is supported before setting them on the device
	if (alcIsRenderFormatSupportedSOFT(curDevice, attrs[5], attrs[1], attrs[3]) == ALC_FALSE) {
		LOG("[Sound::%s] render format not supported: %s, %s, %dhz\n", __func__, ChannelsName(attrs[1]), TypeName(attrs[3]), attrs[5]);
		Cleanup();
		return;
	}

	if ((curContext = alcCreateContext(curDevice, attrs)) == nullptr) {
		LOG_L(L_ERROR, "[Sound::%s] failed to create loopback context", __func__);
		Cleanup();
		return;
	}

	LOG("[Sound::%s] device=%p context=%p  numChannels=%d frameSize=%d", __func__, curDevice, curContext, obtainedSpec.channels, frameSize);
#endif
}



void CSound::InitThread(int cfgMaxSounds)
{
	assert(cfgMaxSounds > 0);

	{
		std::lock_guard<spring::recursive_mutex> lck(soundMutex);
		assert(curDevice == nullptr && curContext == nullptr);
		// if empty, open default device
		std::string configDeviceName;

		LOG("[Sound::%s][1]", __func__);

		// call IsSet; we do not want to create a default for snd_device
		if (configHandler->IsSet("snd_device"))
			configDeviceName = configHandler->GetString("snd_device");

		OpenLoopbackDevice(configDeviceName);

		if (curContext == nullptr)
			OpenOpenALDevice(configDeviceName);

		if (curContext == nullptr) {
			LOG_L(L_ERROR, "[Sound::%s][3] failed to create context", __func__);

			Cleanup();
			soundThreadQuit = true;
			return;
		}

		if (!alcMakeContextCurrent(curContext)) {
			LOG_L(L_ERROR, "[Sound::%s][3] failed to set current context", __func__);

			Cleanup();
			soundThreadQuit = true;
			return;
		}
		CheckError("[Sound::Init::MakeContextCurrent]");

		{
			LOG("[Sound::%s][4][OpenAL API Info]", __func__);
			LOG("  Vendor:         %s", (const char*) alGetString(AL_VENDOR));
			LOG("  Version:        %s", (const char*) alGetString(AL_VERSION));
			LOG("  Renderer:       %s", (const char*) alGetString(AL_RENDERER));
			LOG("  AL Extensions:  %s", (const char*) alGetString(AL_EXTENSIONS));
			LOG("  ALC Extensions: %s", (const char*) alcGetString(curDevice, ALC_EXTENSIONS));
			// same as renderer
			// LOG("  Implementation: %s", (const char*) alcGetString(curDevice, ALC_DEVICE_SPECIFIER));
			LOG("  Devices:");

			const bool hasAllEnumExt = alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
			const bool hasDefEnumExt = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");

			if (hasAllEnumExt || hasDefEnumExt) {
				const char* deviceSpecStr = alcGetString(nullptr, hasAllEnumExt? ALC_ALL_DEVICES_SPECIFIER: ALC_DEVICE_SPECIFIER);

				while (*deviceSpecStr != '\0') {
					LOG("    [%s]", deviceSpecStr);
					while (*deviceSpecStr++ != '\0');
				}
			} else {
				LOG("    [N/A]");
			}
		}

		// generate sound sources; after this <soundSources> never changes size
		GenSources(GetMaxMonoSources(curDevice, cfgMaxSounds));

		{
			// set distance model (sound attenuation)
			alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
			alDopplerFactor(0.2f);
			alListenerf(AL_GAIN, masterVolume);
			CheckError("[Sound::Init]");
		}

		efx.ResetState();
		efx.Init(curDevice);
		CheckError("[Sound::Init::EFX]");
	}

	canLoadDefs = true;
#ifdef ALC_SOFT_loopback
	if (sdlDeviceID != 0)
		SDL_PauseAudioDevice(sdlDeviceID, 0);
#endif
}

__FORCE_ALIGN_STACK__
void CSound::UpdateThread(int cfgMaxSounds)
{
	{
		LOG("[Sound::%s][1] cfgMaxSounds=%d", __func__, cfgMaxSounds);

		Threading::SetAudioThread();
		// InitThread can hang, pre-register
		Watchdog::RegisterThread(WDT_AUDIO);

		// OpenAL will create its own thread and copy the current's name
		Threading::SetThreadName("openal");
		InitThread(cfgMaxSounds);
		Threading::SetThreadName("audio");

		LOG("[Sound::%s][2]", __func__);
	}

	while (!soundThreadQuit) {
		// update at roughly 30Hz
		spring::this_thread::sleep_for(std::chrono::milliseconds(1000 / GAME_SPEED));

		Watchdog::ClearTimer(WDT_AUDIO);
		Update();
	}

	{
		Watchdog::DeregisterThread(WDT_AUDIO);

		LOG("[Sound::%s][3] #sources=%u #items=%u", __func__, uint32_t(soundSources.size()), uint32_t(soundItems.size()));

		// destruct items before context cleanup
		soundSources.clear();
		soundItems.clear();

		LOG("[Sound::%s][4] ctx=%p dev=%p", __func__, curContext, curDevice);

		// must happen after sources and before context
		efx.Kill();
		efx.ResetState();

		LOG("[Sound::%s][5] ctx=%p dev=%p", __func__, curContext, curDevice);

		Cleanup();

		LOG("[Sound::%s][6]", __func__);
	}
}

void CSound::Update()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	// limit consumption-rate to prevent source starvation
	// lock is held, size can not be changed except by loop
	for (size_t i = 0, n = std::min(size_t(4), preloadSet.size()); i < n; i++) {
		GetSoundId(*preloadSet.begin());
	}

	for (CSoundSource& source: soundSources) {
		source.Update();
	}

	CheckError("[Sound::Update]");
	UpdateListenerReal();
}

size_t CSound::MakeItemFromDef(const SoundItemNameMap& itemDef)
{
	// only callers are LoadSoundDefs{Impl} and GetSoundId which both grab this
	// std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	const auto defIt = itemDef.find("file");

	if (defIt == itemDef.end())
		return 0;

	const size_t bufferID = LoadSoundBuffer(defIt->second);
	const size_t   itemID = soundItems.size();

	if (bufferID == 0)
		return 0;

	soundItems.emplace_back(itemID, bufferID, itemDef);
	soundMap[ soundItems[itemID].Name() ] = itemID;
	return itemID;
}


void CSound::UpdateListenerReal()
{
	// call from sound thread, cause OpenAL calls tend to cause L2 misses and so are slow (no reason to call them from mainthread)
	if (!updateListener)
		return;

	updateListener = false;

	const float3 myPosInMeters = myPos * ELMOS_TO_METERS;
	alListener3f(AL_POSITION, myPosInMeters.x, myPosInMeters.y, myPosInMeters.z);

	// reduce the rolloff when the camera is high above the ground (so we still hear something in tab mode or far zoom)
	// for altitudes up to and including 600 elmos, the rolloff is always clamped to 1
	const float camHeight = std::max(1.0f, myPos.y - CGround::GetHeightAboveWater(myPos.x, myPos.z));
	const float rolloffMod = std::min(600.0f / camHeight, 1.0f);

	CSoundSource::SetHeightRolloffModifer(rolloffMod);
	efx.SetHeightRolloffModifer(rolloffMod);

	// Result were bad with listener related doppler effects.
	// The user experiences the camera/listener not as a world-interacting object.
	// So changing sounds on camera movements were irritating, esp. because zooming with the mouse wheel
	// often is faster than the speed of sound, causing very high frequencies.
	// Note: soundsource related doppler effects are not deactivated by this! Flying cannon shoots still change their frequencies.
	// Note2: by not updating the listener velocity soundsource related velocities are calculated wrong,
	// so even if the camera is moving with a cannon shoot the frequency gets changed.
	/*
	const float3 velocity = (myPos - prevPos) / (lastFrameTime);
	float3 velocityAvg = velocity * 0.6f + prevVelocity * 0.4f;
	prevVelocity = velocityAvg;
	velocityAvg *= ELMOS_TO_METERS;
	velocityAvg.y *= 0.001f; //! scale vertical axis separatly (zoom with mousewheel is faster than speed of sound!)
	velocityAvg *= 0.15f;
	alListener3f(AL_VELOCITY, velocityAvg.x, velocityAvg.y, velocityAvg.z);
	*/

	ALfloat ListenerOri[] = {camDir.x, camDir.y, camDir.z, camUp.x, camUp.y, camUp.z};
	alListenerfv(AL_ORIENTATION, ListenerOri);
	CheckError("[Sound::UpdateListener]");
}


void CSound::PrintDebugInfo()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	LOG_L(L_DEBUG, "OpenAL Sound System:");
	LOG_L(L_DEBUG, "# SoundSources: %i", (int)soundSources.size());
	LOG_L(L_DEBUG, "# SoundBuffers: %i", (int)SoundBuffer::Count());

	LOG_L(L_DEBUG, "# reserved for buffers: %i kB", (int)(SoundBuffer::AllocedSize() / 1024));
	LOG_L(L_DEBUG, "# PlayRequests for empty sound: %i", numEmptyPlayRequests);
	LOG_L(L_DEBUG, "# Samples disrupted: %i", numAbortedPlays);
	LOG_L(L_DEBUG, "# SoundItems: %i", (int)soundItems.size());
}

bool CSound::LoadSoundDefsImpl(LuaParser* defsParser)
{
	// can be called from LuaUnsyncedCtrl too
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	defsParser->Execute();

	const std::string& fileName = defsParser->fileName;
	const std::string& errorLog = defsParser->GetErrorLog();

	if (!defsParser->IsValid()) {
		LOG_L(L_WARNING, "[%s] could not load %s: %s", __func__, fileName.c_str(), errorLog.c_str());
		return false;
	}

	{
		const LuaTable& soundRoot = defsParser->GetRoot();
		const LuaTable& soundItemTable = soundRoot.SubTable("SoundItems");

		if (!soundItemTable.IsValid()) {
			LOG_L(L_WARNING, "[%s] could not parse SoundItems table in %s", __func__, fileName.c_str());
			return false;
		}

		{
			std::vector<std::string> keys;
			soundItemTable.GetKeys(keys);

			for (const std::string& name: keys) {
				SoundItemNameMap bufmap;
				const LuaTable buf(soundItemTable.SubTable(name));
				buf.GetMap(bufmap);
				bufmap["name"] = name;

				if (name == "default") {
					defaultItemNameMap = std::move(bufmap);
					defaultItemNameMap.erase("name"); // must be empty for default item
					defaultItemNameMap.erase("file");
					continue;
				}

				if (soundItemDefsMap.find(name) != soundItemDefsMap.end())
					LOG_L(L_WARNING, "[%s] sound %s overwrites %s", __func__, fileName.c_str(), name.c_str());

				if (!buf.KeyExists("file")) {
					// no file, drop
					LOG_L(L_WARNING, "[%s] sound %s is missing file tag (ignoring)", __func__, name.c_str());
					continue;
				}

				soundItemDefsMap[name] = std::move(bufmap);

				if (!buf.KeyExists("preload"))
					continue;

				MakeItemFromDef(soundItemDefsMap[name]);
			}

			LOG("[%s] parsed %i sounds from %s", __func__, (int)keys.size(), fileName.c_str());
		}
	}

	for (auto& pair: soundItemDefsMap) {
		SoundItemNameMap& snddef = pair.second;

		if (snddef.find("name") != snddef.end())
			continue;

		// uses defaultItemNameMap! update it!
		const std::string file = snddef["file"];

		//FIXME why do sounds w/o an own soundItemDef create (!=pointer) a new one from the defaultItemNameMap?
		snddef = defaultItemNameMap;
		snddef["file"] = file;
	}

	return true;
}

// only used internally, locked in caller's scope
size_t CSound::LoadSoundBuffer(const std::string& path)
{
	const size_t id = SoundBuffer::GetId(path);

	if (id > 0)
		return id; // file is loaded already

	// do not keep generating AL buffers for files that previously failed to load, etc
	if (failureSet.find(path) != failureSet.end())
		return 0;

	CFileHandler file("", "");

	loadBuffer.clear();
	loadBuffer.reserve(1024 * 1024);

	file.GetBuffer() = std::move(loadBuffer);
	file.Open(path, SPRING_VFS_RAW_FIRST);

	// steal back
	loadBuffer = std::move(file.GetBuffer());

	if (!file.FileExists()) {
		LOG_L(L_ERROR, "[%s] unable to open audio file \"%s\"", __func__, path.c_str());
		failureSet.insert(path);
		return 0;
	}

	if (loadBuffer.empty()) {
		// copy file into buffer manually if not in VFS
		loadBuffer.resize(file.FileSize());
		file.Read(loadBuffer.data(), loadBuffer.size());
	}


	SoundBuffer soundBuf;
	const std::string& soundExt = file.GetFileExt();

	switch (soundExt[0]) {
		case 'w': { soundBuf.LoadWAV   (path, loadBuffer); } break; // wav
		case 'o': { soundBuf.LoadVorbis(path, loadBuffer); } break; // ogg
		default : {
			LOG_L(L_WARNING, "[%s] unknown audio format \"%s\"", __func__, soundExt.c_str());
		} break;
	}

	CheckError("[Sound::LoadSoundBuffer]");

	if (soundBuf.GetLength() <= 0.0f) {
		LOG_L(L_WARNING, "[%s] failed to load file \"%s\"", __func__, path.c_str());
		failureSet.insert(path);
		return 0;
	}

	return (SoundBuffer::Insert(std::move(soundBuf)));
}

void CSound::NewFrame()
{
	Channels::General->UpdateFrame();
	Channels::Battle->UpdateFrame();
	Channels::UnitReply->UpdateFrame();
	Channels::UserInterface->UpdateFrame();
}



// try to get the maximum number of supported sounds; feeds into GenSources
int CSound::GetMaxMonoSources(ALCdevice* device, int cfgMaxSounds)
{
	ALCint size;
	ALCint attrs[1024 + 1];

	constexpr size_t maxSize = sizeof(attrs) / sizeof(ALCint);

	memset(attrs, 0, sizeof(attrs));
	alcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, 1, &size);

	// size of the array required to hold the key/value pairs plus 0
	// (a size of 13 implies there are six attribute key/value pairs)
	LOG("[Sound::%s] #attribs=%d", __func__, size);

	// must have at least one attribute pair
	if (size < 3)
		return cfgMaxSounds;

	alcGetIntegerv(device, ALC_ALL_ATTRIBUTES, size = std::min(size_t(size), maxSize), &attrs[0]);

	for (size_t i = 0, s = (size - 1) >> 1; i < s; ++i) {
		const ALCint key = attrs[i * 2 + 0];
		const ALCint val = attrs[i * 2 + 1];

		if (key != ALC_MONO_SOURCES)
			continue;

		LOG("[Sound::%s] {cfg,alc}MaxSounds={%d,%d}", __func__, cfgMaxSounds, val);
		return (std::min(cfgMaxSounds, val));
	}

	return cfgMaxSounds;
}

void CSound::GenSources(int alMaxSounds)
{
	soundSources.clear();
	soundSources.reserve(alMaxSounds);

	for (int i = 0; i < alMaxSounds; i++) {
		soundSources.emplace_back();

		if (soundSources[i].IsValid())
			continue;

		soundSources.pop_back();

		// should be unreachable
		LOG_L(L_WARNING, "[Sound::%s] alMaxSounds=%d but numSources=%d", __func__, alMaxSounds, int(soundSources.size()));
		break;
	}
}

