/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
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
#include "IEffectChannel.h"
#include "IMusicChannel.h"
#include "ALShared.h"
#include "EFX.h"
#include "EFXPresets.h"

#include "LogOutput.h"
#include "TimeProfiler.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/myMath.h"
#include "System/Platform/errorhandler.h"

#include "float3.h"

CSound::CSound() : myPos(0.0, 0.0, 0.0), prevVelocity(0.0, 0.0, 0.0), numEmptyPlayRequests(0), numAbortedPlays(0), soundThread(NULL), soundThreadQuit(false)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	mute = false;
	appIsIconified = false;
	int maxSounds = configHandler->Get("MaxSounds", 128);
	pitchAdjust = configHandler->Get("PitchAdjust", false);

	masterVolume = configHandler->Get("snd_volmaster", 60) * 0.01f;
	Channels::General.SetVolume(configHandler->Get("snd_volgeneral", 100 ) * 0.01f);
	Channels::UnitReply.SetVolume(configHandler->Get("snd_volunitreply", 100 ) * 0.01f);
	Channels::UnitReply.SetMaxEmmits(1);
	Channels::Battle.SetVolume(configHandler->Get("snd_volbattle", 100 ) * 0.01f);
	Channels::UserInterface.SetVolume(configHandler->Get("snd_volui", 100 ) * 0.01f);
	Channels::BGMusic.SetVolume(configHandler->Get("snd_volmusic", 100 ) * 0.01f);

	SoundBuffer::Initialise();
	soundItemDef temp;
	temp["name"] = "EmptySource";
	SoundItem* empty = new SoundItem(SoundBuffer::GetById(0), temp);
	sounds.push_back(empty);

	if (maxSounds <= 0) {
		LogObject(LOG_SOUND) << "MaxSounds set to 0, sound is disabled";
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

	sounds.clear();
	SoundBuffer::Deinitialise();
}

bool CSound::HasSoundItem(const std::string& name)
{
	soundMapT::const_iterator it = soundMap.find(name);
	if (it != soundMap.end())
	{
		return true;
	}
	else
	{
		soundItemDefMap::const_iterator it = soundItemDefs.find(name);
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
		soundItemDefMap::const_iterator itemDefIt = soundItemDefs.find(name);
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
					LogObject(LOG_SOUND) << "CSound::GetSoundId: could not find sound: " << name;
					return 0;
				}
			}
		}
	}
}

CSoundSource* CSound::GetNextBestSource(bool lock)
{
	if (sources.empty())
		return NULL;
	sourceVecT::iterator bestPos = sources.begin();
	
	for (sourceVecT::iterator it = sources.begin(); it != sources.end(); ++it)
	{
		if (!it->IsPlaying())
		{
			return &(*it); //argh
		}
		else if (it->GetCurrentPriority() <= bestPos->GetCurrentPriority())
		{
			bestPos = it;
		}
	}
	return &(*bestPos);
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
		efx->sfxProperties->filter_properties_f[AL_LOWPASS_GAIN] = std::atof(value.c_str());
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

void CSound::PlaySample(size_t id, const float3& p, const float3& velocity, float volume, bool relative)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (sources.empty() || volume == 0.0f)
		return;

	if (id == 0)
	{
		numEmptyPlayRequests++;
		return;
	}

	if (p.distance(myPos) > sounds[id].MaxDistance())
	{
		if (!relative)
			return;
		else
			LogObject(LOG_SOUND) << "CSound::PlaySample: maxdist ignored for relative payback: " << sounds[id].Name();
	}

	CSoundSource* best = GetNextBestSource(false);
	if (best && (!best->IsPlaying() || (best->GetCurrentPriority() <= 0 && best->GetCurrentPriority() < sounds[id].GetPriority())))
	{
		if (best->IsPlaying())
			++numAbortedPlays;
		best->Play(&sounds[id], p, velocity, volume, relative);
	}
	CheckError("CSound::PlaySample");
}


void CSound::StartThread(int maxSounds)
{
	{
		boost::recursive_mutex::scoped_lock lck(soundMutex);

		// NULL -> default device
		const ALchar* deviceName = NULL;
		std::string configDeviceName = "";

		// we do not want to set a default for snd_device,
		// so we do it like this ...
		if (configHandler->IsSet("snd_device"))
		{
			configDeviceName = configHandler->GetString("snd_device", "YOU_SHOULD_NOT_EVER_SEE_THIS");
			deviceName = configDeviceName.c_str();
		}

		ALCdevice* device = alcOpenDevice(deviceName);

		if ((device == NULL) && (deviceName != NULL))
		{
			LogObject(LOG_SOUND) <<  "Could not open the sound device \""
					<< deviceName << "\", trying the default device ...";
			configDeviceName = "";
			deviceName = NULL;
			device = alcOpenDevice(deviceName);
		}

		if (device == NULL)
		{
			LogObject(LOG_SOUND) <<  "Could not open a sound device, disabling sounds";
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
				LogObject(LOG_SOUND) << "Could not create OpenAL audio context";
				return;
			}
		}

		const bool airAbsorptionSupported = alcIsExtensionPresent(device, "ALC_EXT_EFX");

		LogObject(LOG_SOUND) << "OpenAL info:\n";
		if(alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		{
			LogObject(LOG_SOUND) << "  Available Devices:";
			const char *s = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
			while (*s != '\0')
			{
				LogObject(LOG_SOUND) << "              " << s;
				while (*s++ != '\0')
					;
			}
			LogObject(LOG_SOUND) << "  Device:     " << alcGetString(device, ALC_DEVICE_SPECIFIER);
		}
		LogObject(LOG_SOUND) << "  Vendor:     " << (const char*)alGetString(AL_VENDOR );
		LogObject(LOG_SOUND) << "  Version:    " << (const char*)alGetString(AL_VERSION);
		LogObject(LOG_SOUND) << "  Renderer:   " << (const char*)alGetString(AL_RENDERER);
		LogObject(LOG_SOUND) << "  AL Extensions:  " << (const char*)alGetString(AL_EXTENSIONS);
		LogObject(LOG_SOUND) << "  ALC Extensions: " << (const char*)alcGetString(device, ALC_EXTENSIONS);

		// Init EFX
		efx = new CEFX(device);

		// Generate sound sources
		for (int i = 0; i < maxSounds; i++)
		{
			CSoundSource* thenewone = new CSoundSource();
			if (thenewone->IsValid()) {
				sources.push_back(thenewone);
			} else {
				maxSounds = std::max(i-1,0);
				LogObject(LOG_SOUND) << "Your hardware/driver can not handle more than " << maxSounds << " soundsources";
				delete thenewone;
				break;
			}
		}

		// Set distance model (sound attenuation)
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		alDopplerFactor(0.2f);

		alListenerf(AL_GAIN, masterVolume);
	}
	configHandler->Set("MaxSounds", maxSounds);

	while (!soundThreadQuit) {
		boost::this_thread::sleep(boost::posix_time::millisec(50)); //! 20Hz
		Update();
	}

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
	const size_t newid = sounds.size();
	soundItemDef::const_iterator it = itemDef.find("file");
	boost::shared_ptr<SoundBuffer> buffer = SoundBuffer::GetById(LoadSoundBuffer(it->second, false));
	if (buffer)
	{
		SoundItem* buf = new SoundItem(buffer, itemDef);
		sounds.push_back(buf);
		soundMap[buf->Name()] = newid;
		return newid;
	}
	else
		return 0;
}

void CSound::UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);
	if (sources.empty())
		return;

	const float3 prevPos = myPos;
	myPos = campos;
	float3 myPosInMeters = myPos * GetElmoInMeters();
	alListener3f(AL_POSITION, myPosInMeters.x, myPosInMeters.y, myPosInMeters.z);

	//! reduce the rolloff when the camera is height above the ground (so we still hear something in tab mode or far zoom)
	const float camHeight = std::max(0.f, campos.y - ground->GetHeightAboveWater(campos.x, campos.z));
	const float newmod = std::min(600.f / camHeight, 1.f);
	CSoundSource::SetHeightRolloffModifer(newmod);
	efx->SetHeightRolloffModifer(newmod);

	//! Result were bad with listener related doppler effects.
	//! The user experience the camera/listener not as world interacting object.
	//! So changing sounds on camera movements were irritating, esp. because zooming with the mouse wheel
	//! often is faster than the speed of sound, causing very high frequencies.
	//! Note: soundsource related doppler effects are not deactivated by this! Flying cannon shoots still change their frequencies.
	//! Note2: by not updating the listener velocity soundsource related velocities are calculated wrong,
	//! so even if the camera is moving with a cannon shoot the frequency gets changed.
	/*const float3 velocity = (myPos - prevPos) / (lastFrameTime);
	float3 velocityAvg = velocity * 0.6f + prevVelocity * 0.4f;
	prevVelocity = velocityAvg;
	velocityAvg *= GetElmoInMeters();
	velocityAvg.y *= 0.001f; //! scale vertical axis separatly (zoom with mousewheel is faster than speed of sound!)
	velocityAvg *= 0.15f;
	alListener3f(AL_VELOCITY, velocityAvg.x, velocityAvg.y, velocityAvg.z);*/

	ALfloat ListenerOri[] = {camdir.x, camdir.y, camdir.z, camup.x, camup.y, camup.z};
	alListenerfv(AL_ORIENTATION, ListenerOri);
	CheckError("CSound::UpdateListener");
}

void CSound::PrintDebugInfo()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	LogObject(LOG_SOUND) << "OpenAL Sound System:";
	LogObject(LOG_SOUND) << "# SoundSources: " << sources.size();
	LogObject(LOG_SOUND) << "# SoundBuffers: " << SoundBuffer::Count();

	LogObject(LOG_SOUND) << "# reserved for buffers: " << (SoundBuffer::AllocedSize()/1024) << " kB";
	LogObject(LOG_SOUND) << "# PlayRequests for empty sound: " << numEmptyPlayRequests;
	LogObject(LOG_SOUND) << "# Samples disrupted: " << numAbortedPlays;
	LogObject(LOG_SOUND) << "# SoundItems: " << sounds.size();
}

bool CSound::LoadSoundDefs(const std::string& fileName)
{
	//! can be called from LuaUnsyncedCtrl too
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	LuaParser parser(fileName, SPRING_VFS_MOD, SPRING_VFS_ZIP);
	parser.SetLowerKeys(false);
	parser.SetLowerCppKeys(false);
	parser.Execute();
	if (!parser.IsValid())
	{
		LogObject(LOG_SOUND) << "Could not load " << fileName << ": " << parser.GetErrorLog();
		return false;
	}
	else
	{
		const LuaTable soundRoot = parser.GetRoot();
		const LuaTable soundItemTable = soundRoot.SubTable("SoundItems");
		if (!soundItemTable.IsValid())
		{
			LogObject(LOG_SOUND) << "CSound(): could not parse SoundItems table in " << fileName;
			return false;
		}
		else
		{
			std::vector<std::string> keys;
			soundItemTable.GetKeys(keys);
			for (std::vector<std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				const std::string name(*it);
				soundItemDef bufmap;
				const LuaTable buf(soundItemTable.SubTable(*it));
				buf.GetMap(bufmap);
				bufmap["name"] = name;
				soundItemDefMap::const_iterator sit = soundItemDefs.find(name);
				if (sit != soundItemDefs.end())
					LogObject(LOG_SOUND) << "Sound " << name << " gets overwritten by " << fileName;

				soundItemDef::const_iterator inspec = bufmap.find("file");
				if (inspec == bufmap.end())	// no file, drop
					LogObject(LOG_SOUND) << "Sound " << name << " is missing file tag (ignoring)";
				else
					soundItemDefs[name] = bufmap;

				if (buf.KeyExists("preload"))
				{
					MakeItemFromDef(bufmap);
				}
			}
			LogObject(LOG_SOUND) << " parsed " << keys.size() << " sounds from " << fileName;
		}
	}
	return true;
}

//! only used internally, locked in caller's scope
size_t CSound::LoadSoundBuffer(const std::string& path, bool hardFail)
{
	const size_t id = SoundBuffer::GetId(path);
	
	if (id > 0)
	{
		return id; // file is loaded already
	}
	else
	{
		CFileHandler file(path);

		if (!file.FileExists())
		{
			if (hardFail) {
				handleerror(0, "Couldn't open audio file", path.c_str(),0);
			}
			else
				LogObject(LOG_SOUND) << "Unable to open audio file: " << path;
			return 0;
		}
		else
		{
			
		}

		std::vector<boost::uint8_t> buf(file.FileSize());
		file.Read(&buf[0], file.FileSize());

		boost::shared_ptr<SoundBuffer> buffer(new SoundBuffer());
		bool success = false;
		const std::string ending = file.GetFileExt();
		if (ending == "wav")
			success = buffer->LoadWAV(path, buf, hardFail);
		else if (ending == "ogg")
			success = buffer->LoadVorbis(path, buf, hardFail);
		else
		{
			LogObject(LOG_SOUND) << "CSound::LoadALBuffer: unknown audio format: " << ending;
		}

		CheckError("CSound::LoadALBuffer");
		if (!success) {
			LogObject(LOG_SOUND) << "Failed to load file: " << path;
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
