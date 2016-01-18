/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sound.h"

#include <cstdlib>
#include <cmath>
#include <alc.h>

#ifndef ALC_ALL_DEVICES_SPECIFIER
#define ALC_ALL_DEVICES_SPECIFIER 0x1013
// needed for ALC_ALL_DEVICES_SPECIFIER on some special *nix
// #include <alext.h>
#endif

#include <boost/cstdint.hpp>
#include <boost/thread/thread.hpp>

#include "System/Sound/ISoundChannels.h"
#include "System/Sound/SoundLog.h"
#include "SoundSource.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "ALShared.h"
#include "EFX.h"
#include "EFXPresets.h"

#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/myMath.h"
#include "System/Util.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Watchdog.h"

#include "System/float3.h"


boost::recursive_mutex soundMutex;


CSound::CSound()
	: listenerNeedsUpdate(false)
	, soundThread(NULL)
	, soundThreadQuit(false)
	, canLoadDefs(false)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	mute = false;
	appIsIconified = false;
	int maxSounds = configHandler->GetInt("MaxSounds");
	pitchAdjust = configHandler->GetBool("PitchAdjust");

	masterVolume = configHandler->GetInt("snd_volmaster") * 0.01f;
	Channels::General->SetVolume(configHandler->GetInt("snd_volgeneral") * 0.01f);
	Channels::UnitReply->SetVolume(configHandler->GetInt("snd_volunitreply") * 0.01f);
	Channels::UnitReply->SetMaxConcurrent(1);
	Channels::UnitReply->SetMaxEmmits(1);
	Channels::Battle->SetVolume(configHandler->GetInt("snd_volbattle") * 0.01f);
	Channels::UserInterface->SetVolume(configHandler->GetInt("snd_volui") * 0.01f);
	Channels::BGMusic->SetVolume(configHandler->GetInt("snd_volmusic") * 0.01f);

	SoundBuffer::Initialise();
	soundItemDef temp;
	temp["name"] = "EmptySource";
	sounds.push_back(NULL);

	assert(maxSounds>0);
	soundThread = new boost::thread();
	*soundThread = Threading::CreateNewThread(boost::bind(&CSound::StartThread, this, maxSounds));

	configHandler->NotifyOnChange(this);
}

CSound::~CSound()
{
	soundThreadQuit = true;
	configHandler->RemoveObserver(this);

	LOG_L(L_INFO, "[%s][1] soundThread=%p", __FUNCTION__, soundThread);

	if (soundThread != NULL) {
		soundThread->join();
		delete soundThread;
		soundThread = NULL;
	}

	LOG_L(L_INFO, "[%s][2]", __FUNCTION__);

	for (soundVecT::iterator it = sounds.begin(); it != sounds.end(); ++it)
		delete *it;

	sounds.clear();
	SoundBuffer::Deinitialise();

	LOG_L(L_INFO, "[%s][3]", __FUNCTION__);
}

bool CSound::HasSoundItem(const std::string& name) const
{
	if (soundMap.find(name) != soundMap.end())
		return true;

	return (soundItemDefs.find(StringToLower(name)) != soundItemDefs.end());
}

size_t CSound::GetSoundId(const std::string& name)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (sources.empty())
		return 0;

	soundMapT::const_iterator it = soundMap.find(name);
	if (it != soundMap.end()) {
		// sounditem found
		return it->second;
	}

	soundItemDefMap::const_iterator itemDefIt = soundItemDefs.find(StringToLower(name));
	if (itemDefIt != soundItemDefs.end()) {
		return MakeItemFromDef(itemDefIt->second);
	}

	if (LoadSoundBuffer(name) > 0) {
		// maybe raw filename?
		soundItemDef temp = defaultItem;
		temp["file"] = name;
		return MakeItemFromDef(temp);
	}

	LOG_L(L_ERROR, "CSound::GetSoundId: could not find sound: %s", name.c_str());
	return 0;
}

SoundItem* CSound::GetSoundItem(size_t id) const {
	// id==0 is a special id and invalid
	if (id == 0 || id >= sounds.size())
		return NULL;
	return sounds[id];
}

CSoundSource* CSound::GetNextBestSource(bool lock)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex, boost::defer_lock);
	if (lock)
		lck.lock();

	if (sources.empty())
		return nullptr;

	// find if we got a free source
	for (CSoundSource& src: sources) {
		if (!src.IsPlaying(false)){
			return &src;
		}
	}

	// check the next best free source
	CSoundSource* bestSrc = nullptr;
	int bestPriority = INT_MAX;
	for (CSoundSource& src: sources) {
		/*if (!src.IsPlaying(true)) {
			return &src;
		}
		else*/ if (src.GetCurrentPriority() <= bestPriority) {
			bestSrc = &src;
			bestPriority = src.GetCurrentPriority();
		}
	}
	return bestSrc;
}

void CSound::PitchAdjust(const float newPitch)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	if (pitchAdjust)
		CSoundSource::SetPitch(newPitch);
}

void CSound::ConfigNotify(const std::string& key, const std::string& value)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	if (key == "snd_volmaster")
	{
		masterVolume = std::atoi(value.c_str()) * 0.01f;
		if (!mute && !appIsIconified)
			alListenerf(AL_GAIN, masterVolume);
	}
	else if (key == "snd_eaxpreset")
	{
		efx->SetPreset(value);
	}
	else if (key == "snd_filter")
	{
		float gainlf = 1.f;
		float gainhf = 1.f;
		sscanf(value.c_str(), "%f %f", &gainlf, &gainhf);
		efx->sfxProperties->filter_properties_f[AL_LOWPASS_GAIN]   = gainlf;
		efx->sfxProperties->filter_properties_f[AL_LOWPASS_GAINHF] = gainhf;
		efx->CommitEffects();
	}
	else if (key == "UseEFX")
	{
		bool enable = (std::atoi(value.c_str()) != 0);
		if (enable)
			efx->Enable();
		else
			efx->Disable();
	}
	else if (key == "snd_volgeneral")
	{
		Channels::General->SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volunitreply")
	{
		Channels::UnitReply->SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volbattle")
	{
		Channels::Battle->SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volui")
	{
		Channels::UserInterface->SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volmusic")
	{
		Channels::BGMusic->SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "PitchAdjust")
	{
		bool tempPitchAdjust = (std::atoi(value.c_str()) != 0);
		if (!tempPitchAdjust)
			PitchAdjust(1.0);
		pitchAdjust = tempPitchAdjust;
	}
}

bool CSound::Mute()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	mute = !mute;
	if (mute)
		alListenerf(AL_GAIN, 0.0f);
	else
		alListenerf(AL_GAIN, masterVolume);
	return mute;
}

bool CSound::IsMuted() const
{
	return mute;
}

void CSound::Iconified(bool state)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	if (appIsIconified != state && !mute)
	{
		if (state == false)
			alListenerf(AL_GAIN, masterVolume);
		else if (state == true)
			alListenerf(AL_GAIN, 0.0);
	}
	appIsIconified = state;
}

__FORCE_ALIGN_STACK__
void CSound::StartThread(int maxSounds)
{
	{
		boost::recursive_mutex::scoped_lock lck(soundMutex);

		// alc... will create its own thread it will copy the name from the current thread.
		// Later we finally rename `our` audio thread.
		Threading::SetThreadName("openal");

		// NULL -> default device
		const ALchar* deviceName = NULL;
		std::string configDeviceName = "";

		// we do not want to set a default for snd_device,
		// so we do it like this ...
		if (configHandler->IsSet("snd_device")) {
			configDeviceName = configHandler->GetString("snd_device");
			deviceName = configDeviceName.c_str();
		}

		ALCdevice* device = alcOpenDevice(deviceName);

		if ((device == NULL) && (deviceName != NULL)) {
			LOG_L(L_WARNING,
					"Could not open the sound device \"%s\", trying the default device ...",
					deviceName);
			configDeviceName = "";
			deviceName = NULL;
			device = alcOpenDevice(deviceName);
		}

		if (device == NULL) {
			LOG_L(L_ERROR, "Could not open a sound device, disabling sounds");
			CheckError("CSound::InitAL");
			// fall back to NullSound
			soundThreadQuit = true;
			return;
		} else {
			ALCcontext* context = alcCreateContext(device, NULL);

			if (context != NULL) {
				alcMakeContextCurrent(context);
				CheckError("CSound::CreateContext");
			} else {
				alcCloseDevice(device);
				LOG_L(L_ERROR, "Could not create OpenAL audio context");
				// fall back to NullSound
				soundThreadQuit = true;
				return;
			}
		}
		maxSounds = GetMaxMonoSources(device, maxSounds);

		LOG("OpenAL info:");
		const bool hasAllEnum = alcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT");
		if (hasAllEnum || alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT")) {
			LOG("  Available Devices:");
			const char* deviceSpecifier = alcGetString(NULL, hasAllEnum ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);
			while (*deviceSpecifier != '\0') {
				LOG("              %s", deviceSpecifier);
				while (*deviceSpecifier++ != '\0')
					;
			}
			LOG("  Device:     %s", (const char*)alcGetString(device, ALC_DEVICE_SPECIFIER));
		}
		LOG("  Vendor:         %s", (const char*)alGetString(AL_VENDOR));
		LOG("  Version:        %s", (const char*)alGetString(AL_VERSION));
		LOG("  Renderer:       %s", (const char*)alGetString(AL_RENDERER));
		LOG("  AL Extensions:  %s", (const char*)alGetString(AL_EXTENSIONS));
		LOG("  ALC Extensions: %s", (const char*)alcGetString(device, ALC_EXTENSIONS));

		// Init EFX
		efx = new CEFX(device);

		// Generate sound sources
		for (int i = 0; i < maxSounds; i++) {
			CSoundSource* thenewone = new CSoundSource();

			if (thenewone->IsValid()) {
				sources.push_back(thenewone);
			} else {
				maxSounds = std::max(i-1, 0);
				LOG_L(L_WARNING,
						"Your hardware/driver can not handle more than %i soundsources",
						maxSounds);
				delete thenewone;
				break;
			}
		}
		LOG("  Max Sounds: %i", maxSounds);

		// Set distance model (sound attenuation)
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		alDopplerFactor(0.2f);

		alListenerf(AL_GAIN, masterVolume);
	}

	canLoadDefs = true;

	Threading::SetThreadName("audio");
	Watchdog::RegisterThread(WDT_AUDIO);

	while (!soundThreadQuit) {
		constexpr int FREQ_IN_HZ = 30;
		boost::this_thread::sleep(boost::posix_time::millisec(1000 / FREQ_IN_HZ));
		Watchdog::ClearTimer(WDT_AUDIO);
		Update();
	}

	Watchdog::DeregisterThread(WDT_AUDIO);

	sources.clear(); // delete all sources
	delete efx; // must happen after sources and before context
	efx = NULL;
	ALCcontext* curcontext = alcGetCurrentContext();
	ALCdevice* curdevice = alcGetContextsDevice(curcontext);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(curcontext);
	alcCloseDevice(curdevice);
}

void CSound::Update()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex); // lock
	for (sourceVecT::iterator it = sources.begin(); it != sources.end(); ++it)
		it->Update();
	CheckError("CSound::Update");
	UpdateListenerReal();
}

size_t CSound::MakeItemFromDef(const soundItemDef& itemDef)
{
	//! MakeItemFromDef is private. Only caller is LoadSoundDefs and it sets the mutex itself.
	//boost::recursive_mutex::scoped_lock lck(soundMutex);
	const size_t newid = sounds.size();
	soundItemDef::const_iterator it = itemDef.find("file");
	if (it == itemDef.end())
		return 0;

	boost::shared_ptr<SoundBuffer> buffer = SoundBuffer::GetById(LoadSoundBuffer(it->second));

	if (!buffer)
		return 0;

	SoundItem* buf = new SoundItem(buffer, itemDef);
	sounds.push_back(buf);
	soundMap[buf->Name()] = newid;
	return newid;
}

void CSound::UpdateListener(const float3& campos, const float3& camdir, const float3& camup)
{
	myPos  = campos;
	camDir = camdir;
	camUp  = camup;
	listenerNeedsUpdate = true;
}


void CSound::UpdateListenerReal()
{
	// call from sound thread, cause OpenAL calls tend to cause L2 misses and so are slow (no reason to call them from mainthread)
	if (!listenerNeedsUpdate)
		return;

	// not 100% threadsafe, but worst case we would skip a single listener update (and it runs at multiple Hz!)
	listenerNeedsUpdate = false;

	const float3 myPosInMeters = myPos * ELMOS_TO_METERS;
	alListener3f(AL_POSITION, myPosInMeters.x, myPosInMeters.y, myPosInMeters.z);

	// reduce the rolloff when the camera is high above the ground (so we still hear something in tab mode or far zoom)
	// for altitudes up to and including 600 elmos, the rolloff is always clamped to 1
	const float camHeight = std::max(1.0f, myPos.y - CGround::GetHeightAboveWater(myPos.x, myPos.z));
	const float newMod = std::min(600.0f / camHeight, 1.0f);

	CSoundSource::SetHeightRolloffModifer(newMod);
	efx->SetHeightRolloffModifer(newMod);

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
	CheckError("CSound::UpdateListener");
}


void CSound::PrintDebugInfo()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	LOG_L(L_DEBUG, "OpenAL Sound System:");
	LOG_L(L_DEBUG, "# SoundSources: %i", (int)sources.size());
	LOG_L(L_DEBUG, "# SoundBuffers: %i", (int)SoundBuffer::Count());

	LOG_L(L_DEBUG, "# reserved for buffers: %i kB", (int)(SoundBuffer::AllocedSize() / 1024));
	LOG_L(L_DEBUG, "# PlayRequests for empty sound: %i", numEmptyPlayRequests);
	LOG_L(L_DEBUG, "# Samples disrupted: %i", numAbortedPlays);
	LOG_L(L_DEBUG, "# SoundItems: %i", (int)sounds.size());
}

bool CSound::LoadSoundDefsImpl(const std::string& fileName, const std::string& modes)
{
	//! can be called from LuaUnsyncedCtrl too
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	LuaParser parser(fileName, modes, modes);
	parser.Execute();
	if (!parser.IsValid())
	{
		LOG_L(L_WARNING, "Could not load %s: %s",
				fileName.c_str(), parser.GetErrorLog().c_str());
		return false;
	}
	else
	{
		const LuaTable soundRoot = parser.GetRoot();
		const LuaTable soundItemTable = soundRoot.SubTable("SoundItems");
		if (!soundItemTable.IsValid())
		{
			LOG_L(L_WARNING, "CSound(): could not parse SoundItems table in %s", fileName.c_str());
			return false;
		}
		else
		{
			std::vector<std::string> keys;
			soundItemTable.GetKeys(keys);
			for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				std::string name(*it);

				soundItemDef bufmap;
				const LuaTable buf(soundItemTable.SubTable(name));
				buf.GetMap(bufmap);
				bufmap["name"] = name;
				soundItemDefMap::const_iterator sit = soundItemDefs.find(name);

				if (name == "default") {
					defaultItem = bufmap;
					defaultItem.erase("name"); //must be empty for default item
					defaultItem.erase("file");
					continue;
				}

				if (sit != soundItemDefs.end())
					LOG_L(L_WARNING, "Sound %s gets overwritten by %s", name.c_str(), fileName.c_str());

				if (!buf.KeyExists("file")) {
					// no file, drop
					LOG_L(L_WARNING, "Sound %s is missing file tag (ignoring)", name.c_str());
					continue;
				} else {
					soundItemDefs[name] = bufmap;
				}

				if (buf.KeyExists("preload")) {
					MakeItemFromDef(bufmap);
				}
			}
			LOG(" parsed %i sounds from %s", (int)keys.size(), fileName.c_str());
		}
	}

	//FIXME why do sounds w/o an own soundItemDef create (!=pointer) a new one from the defaultItem?
	for (soundItemDefMap::iterator it = soundItemDefs.begin(); it != soundItemDefs.end(); ++it) {
		soundItemDef& snddef = it->second;
		if (snddef.find("name") == snddef.end()) {
			// uses defaultItem! update it!
			const std::string file = snddef["file"];
			snddef = defaultItem;
			snddef["file"] = file;
		}
	}

	return true;
}

//! only used internally, locked in caller's scope
size_t CSound::LoadSoundBuffer(const std::string& path)
{
	const size_t id = SoundBuffer::GetId(path);

	if (id > 0) {
		return id; // file is loaded already
	} else {
		CFileHandler file(path);

		if (!file.FileExists()) {
			LOG_L(L_ERROR, "Unable to open audio file: %s", path.c_str());
			return 0;
		}

		std::vector<boost::uint8_t> buf(file.FileSize());
		file.Read(&buf[0], file.FileSize());

		boost::shared_ptr<SoundBuffer> buffer(new SoundBuffer());
		bool success = false;
		const std::string ending = file.GetFileExt();
		if (ending == "wav") {
			success = buffer->LoadWAV(path, buf);
		} else if (ending == "ogg") {
			success = buffer->LoadVorbis(path, buf);
		} else {
			LOG_L(L_WARNING, "CSound::LoadALBuffer: unknown audio format: %s",
					ending.c_str());
		}

		CheckError("CSound::LoadALBuffer");
		if (!success) {
			LOG_L(L_WARNING, "Failed to load file: %s", path.c_str());
			return 0;
		}

		return SoundBuffer::Insert(buffer);
	}
}

void CSound::NewFrame()
{
	Channels::General->UpdateFrame();
	Channels::Battle->UpdateFrame();
	Channels::UnitReply->UpdateFrame();
	Channels::UserInterface->UpdateFrame();
}


// try to get the maximum number of supported sounds, this is similar to code CSound::StartThread
// but should be more safe
int CSound::GetMaxMonoSources(ALCdevice* device, int maxSounds)
{
	ALCint size;
	alcGetIntegerv(device, ALC_ATTRIBUTES_SIZE, 1, &size);
	std::vector<ALCint> attrs(size);
	alcGetIntegerv(device, ALC_ALL_ATTRIBUTES, size, &attrs[0] );
	for (int i=0; i<attrs.size(); ++i){
		if (attrs[i] == ALC_MONO_SOURCES) {
			const int maxMonoSources = attrs.at(i + 1);
			if (maxMonoSources < maxSounds) {
				LOG_L(L_WARNING, "Hardware supports only %d Sound sources, MaxSounds=%d, using Hardware Limit", maxMonoSources, maxSounds);
			}
			return std::min(maxSounds, maxMonoSources);
		}
	}
	return maxSounds;
}

