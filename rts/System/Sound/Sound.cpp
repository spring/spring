/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Sound.h"

#include <cstdlib>
#include <cmath>
#include <alc.h>
#include <boost/cstdint.hpp>

#include "SoundChannels.h"
#include "SoundLog.h"
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
#include "System/Platform/errorhandler.h"
#include "System/Platform/Watchdog.h"

#include "System/float3.h"

CONFIG(int, MaxSounds).defaultValue(128);
CONFIG(bool, PitchAdjust).defaultValue(false);
CONFIG(int, snd_volmaster).defaultValue(60);
CONFIG(int, snd_volgeneral).defaultValue(100);
CONFIG(int, snd_volunitreply).defaultValue(100);
CONFIG(int, snd_volbattle).defaultValue(100);
CONFIG(int, snd_volui).defaultValue(100);
CONFIG(int, snd_volmusic).defaultValue(100);
CONFIG(std::string, snd_device).defaultValue("");

boost::recursive_mutex soundMutex;


CSound::CSound()
	: myPos(0., 0., 0.)
	, prevVelocity(0., 0., 0.)
	, soundThread(NULL)
	, soundThreadQuit(false)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	mute = false;
	appIsIconified = false;
	int maxSounds = configHandler->GetInt("MaxSounds");
	pitchAdjust = configHandler->GetBool("PitchAdjust");

	masterVolume = configHandler->GetInt("snd_volmaster") * 0.01f;
	Channels::General.SetVolume(configHandler->GetInt("snd_volgeneral") * 0.01f);
	Channels::UnitReply.SetVolume(configHandler->GetInt("snd_volunitreply") * 0.01f);
	Channels::UnitReply.SetMaxConcurrent(1);
	Channels::UnitReply.SetMaxEmmits(1);
	Channels::Battle.SetVolume(configHandler->GetInt("snd_volbattle") * 0.01f);
	Channels::UserInterface.SetVolume(configHandler->GetInt("snd_volui") * 0.01f);
	Channels::BGMusic.SetVolume(configHandler->GetInt("snd_volmusic") * 0.01f);

	SoundBuffer::Initialise();
	soundItemDef temp;
	temp["name"] = "EmptySource";
	sounds.push_back(NULL);

	if (maxSounds <= 0) {
		LOG_L(L_WARNING, "MaxSounds set to 0, sound is disabled");
	} else {
		soundThread = new boost::thread(boost::bind(&CSound::StartThread, this, maxSounds));
	}

	configHandler->NotifyOnChange(this);
}

CSound::~CSound()
{
	soundThreadQuit = true;

	if (soundThread) {
		soundThread->join();
		delete soundThread;
		soundThread = NULL;
	}

	for (soundVecT::iterator it = sounds.begin(); it != sounds.end(); ++it)
		delete *it;
	sounds.clear();
	SoundBuffer::Deinitialise();
}

bool CSound::HasSoundItem(const std::string& name) const
{
	soundMapT::const_iterator it = soundMap.find(name);
	if (it != soundMap.end())
	{
		return true;
	}
	else
	{
		soundItemDefMap::const_iterator it = soundItemDefs.find(StringToLower(name));
		if (it != soundItemDefs.end())
			return true;
		else
			return false;
	}
}

size_t CSound::GetSoundId(const std::string& name, bool hardFail)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (sources.empty())
		return 0;

	soundMapT::const_iterator it = soundMap.find(name);
	if (it != soundMap.end())
	{
		// sounditem found
		return it->second;
	}
	else
	{
		soundItemDefMap::const_iterator itemDefIt = soundItemDefs.find(StringToLower(name));
		if (itemDefIt != soundItemDefs.end())
		{
			return MakeItemFromDef(itemDefIt->second);
		}
		else
		{
			if (LoadSoundBuffer(name, hardFail) > 0) // maybe raw filename?
			{
				soundItemDef temp = defaultItem;
				temp["file"] = name;
				return MakeItemFromDef(temp);
			}
			else
			{
				if (hardFail)
				{
					ErrorMessageBox("Couldn't open wav file", name, 0);
					return 0;
				}
				else
				{
					LOG_L(L_WARNING, "CSound::GetSoundId: could not find sound: %s", name.c_str());
					return 0;
				}
			}
		}
	}
}

SoundItem* CSound::GetSoundItem(size_t id) const {
	//! id==0 is a special id and invalid
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
		return NULL;

	CSoundSource* bestPos = NULL;
	for (sourceVecT::iterator it = sources.begin(); it != sources.end(); ++it)
	{
		if (!it->IsPlaying())
		{
			return &(*it);
		}
		else if (it->GetCurrentPriority() <= (bestPos ? bestPos->GetCurrentPriority() : INT_MAX))
		{
			bestPos = &(*it);
		}
	}
	return bestPos;
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
		Channels::General.SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volunitreply")
	{
		Channels::UnitReply.SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volbattle")
	{
		Channels::Battle.SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volui")
	{
		Channels::UserInterface.SetVolume(std::atoi(value.c_str()) * 0.01f);
	}
	else if (key == "snd_volmusic")
	{
		Channels::BGMusic.SetVolume(std::atoi(value.c_str()) * 0.01f);
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
		alListenerf(AL_GAIN, 0.0);
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
		if (configHandler->IsSet("snd_device"))
		{
			configDeviceName = configHandler->GetString("snd_device");
			deviceName = configDeviceName.c_str();
		}

		ALCdevice* device = alcOpenDevice(deviceName);

		if ((device == NULL) && (deviceName != NULL))
		{
			LOG_L(L_WARNING,
					"Could not open the sound device \"%s\", trying the default device ...",
					deviceName);
			configDeviceName = "";
			deviceName = NULL;
			device = alcOpenDevice(deviceName);
		}

		if (device == NULL)
		{
			LOG_L(L_ERROR, "Could not open a sound device, disabling sounds");
			CheckError("CSound::InitAL");
			return;
		}
		else
		{
			ALCcontext *context = alcCreateContext(device, NULL);
			if (context != NULL)
			{
				alcMakeContextCurrent(context);
				CheckError("CSound::CreateContext");
			}
			else
			{
				alcCloseDevice(device);
				LOG_L(L_ERROR, "Could not create OpenAL audio context");
				return;
			}
		}

		LOG("OpenAL info:");
		if(alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		{
			LOG("  Available Devices:");
			const char* deviceSpecifier = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
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
		for (int i = 0; i < maxSounds; i++)
		{
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
	configHandler->Set("MaxSounds", maxSounds);

	Threading::SetThreadName("audio");
	Watchdog::RegisterThread(WDT_AUDIO);

	while (!soundThreadQuit) {
		boost::this_thread::sleep(boost::posix_time::millisec(50)); //! 20Hz
		Watchdog::ClearTimer(WDT_AUDIO);
		Update();
	}

	Watchdog::DeregisterThread(WDT_AUDIO);

	sources.clear(); // delete all sources
	delete efx; // must happen after sources and before context
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
}

size_t CSound::MakeItemFromDef(const soundItemDef& itemDef)
{
	//! MakeItemFromDef is private. Only caller is LoadSoundDefs and it sets the mutex itself.
	//boost::recursive_mutex::scoped_lock lck(soundMutex);
	const size_t newid = sounds.size();
	soundItemDef::const_iterator it = itemDef.find("file");
	if (it == itemDef.end())
		return 0;

	boost::shared_ptr<SoundBuffer> buffer = SoundBuffer::GetById(LoadSoundBuffer(it->second, false));
	
	if (!buffer)
		return 0;
		
	SoundItem* buf = new SoundItem(buffer, itemDef);
	sounds.push_back(buf);
	soundMap[buf->Name()] = newid;
	return newid;
}

void CSound::UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	if (sources.empty())
		return;

	myPos = campos;
	const float3 myPosInMeters = myPos * ELMOS_TO_METERS;
	alListener3f(AL_POSITION, myPosInMeters.x, myPosInMeters.y, myPosInMeters.z);

	//! reduce the rolloff when the camera is high above the ground (so we still hear something in tab mode or far zoom)
	//! for altitudes up to and including 600 elmos, the rolloff is always clamped to 1
	const float camHeight = std::max(1.0f, campos.y - ground->GetHeightAboveWater(campos.x, campos.z));
	const float newMod = std::min(600.0f / camHeight, 1.0f);

	CSoundSource::SetHeightRolloffModifer(newMod);
	efx->SetHeightRolloffModifer(newMod);

	//! Result were bad with listener related doppler effects.
	//! The user experiences the camera/listener not as a world-interacting object.
	//! So changing sounds on camera movements were irritating, esp. because zooming with the mouse wheel
	//! often is faster than the speed of sound, causing very high frequencies.
	//! Note: soundsource related doppler effects are not deactivated by this! Flying cannon shoots still change their frequencies.
	//! Note2: by not updating the listener velocity soundsource related velocities are calculated wrong,
	//! so even if the camera is moving with a cannon shoot the frequency gets changed.
	/*
	const float3 velocity = (myPos - prevPos) / (lastFrameTime);
	float3 velocityAvg = velocity * 0.6f + prevVelocity * 0.4f;
	prevVelocity = velocityAvg;
	velocityAvg *= ELMOS_TO_METERS;
	velocityAvg.y *= 0.001f; //! scale vertical axis separatly (zoom with mousewheel is faster than speed of sound!)
	velocityAvg *= 0.15f;
	alListener3f(AL_VELOCITY, velocityAvg.x, velocityAvg.y, velocityAvg.z);
	*/

	ALfloat ListenerOri[] = {camdir.x, camdir.y, camdir.z, camup.x, camup.y, camup.z};
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

bool CSound::LoadSoundDefs(const std::string& fileName)
{
	//! can be called from LuaUnsyncedCtrl too
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	LuaParser parser(fileName, SPRING_VFS_MOD, SPRING_VFS_ZIP);
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
size_t CSound::LoadSoundBuffer(const std::string& path, bool hardFail)
{
	const size_t id = SoundBuffer::GetId(path);

	if (id > 0) {
		return id; // file is loaded already
	} else {
		CFileHandler file(path);

		if (!file.FileExists()) {
			if (hardFail) {
				handleerror(0, "Could not open audio file", path, 0);
			} else {
				LOG_L(L_WARNING, "Unable to open audio file: %s", path.c_str());
			}
			return 0;
		}

		std::vector<boost::uint8_t> buf(file.FileSize());
		file.Read(&buf[0], file.FileSize());

		boost::shared_ptr<SoundBuffer> buffer(new SoundBuffer());
		bool success = false;
		const std::string ending = file.GetFileExt();
		if (ending == "wav") {
			success = buffer->LoadWAV(path, buf, hardFail);
		} else if (ending == "ogg") {
			success = buffer->LoadVorbis(path, buf, hardFail);
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
	Channels::General.UpdateFrame();
	Channels::Battle.UpdateFrame();
	Channels::UnitReply.UpdateFrame();
	Channels::UserInterface.UpdateFrame();
}
