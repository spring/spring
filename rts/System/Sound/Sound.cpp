/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"

#include <cstdlib>
#include <cmath>
#include <alc.h>
#include <boost/cstdint.hpp>

#include "SoundLog.h"
#include "SoundSource.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "IEffectChannel.h"
#include "IMusicChannel.h"
#include "ALShared.h"

#include "LogOutput.h"
#include "TimeProfiler.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"
#include "Lua/LuaParser.h"

#include "float3.h"

CSound::CSound() : prevVelocity(0.0, 0.0, 0.0), numEmptyPlayRequests(0), numAbortedPlays(0), soundThread(NULL), soundThreadQuit(false)
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
		soundThread = 0;
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

SoundSource* CSound::GetNextBestSource(bool lock)
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
		SoundSource::SetPitch(newPitch);
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
	else if (key == "snd_airAbsorption")
	{
		const float airAbsorption = std::atof(value.c_str());
		SoundSource::SetAirAbsorption(airAbsorption);
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

	SoundSource* best = GetNextBestSource(false);
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
		LogObject(LOG_SOUND) << "  Vendor:     " << (const char*)alGetString(AL_VENDOR );
		LogObject(LOG_SOUND) << "  Version:    " << (const char*)alGetString(AL_VERSION);
		LogObject(LOG_SOUND) << "  Renderer:   " << (const char*)alGetString(AL_RENDERER);
		LogObject(LOG_SOUND) << "  AL Extensions: " << (const char*)alGetString(AL_EXTENSIONS);
		LogObject(LOG_SOUND) << "  ALC Extensions: " << (const char*)alcGetString(device, ALC_EXTENSIONS);
		LogObject(LOG_SOUND) << "                  ALC_EXT_EFX found (required for air absorption): " << (airAbsorptionSupported ? "yes" : "no");
		if(alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		{
			LogObject(LOG_SOUND) << "  Device:     " << alcGetString(device, ALC_DEVICE_SPECIFIER);
			const char *s = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
			LogObject(LOG_SOUND) << "  Available Devices:  ";
			while (*s != '\0')
			{
				LogObject(LOG_SOUND) << "                      " << s;
				while (*s++ != '\0')
					;
			}
		}

		SoundSource::SetAirAbsorptionSupported(airAbsorptionSupported);
		if (airAbsorptionSupported) {
			const float airAbsorption = configHandler->Get("snd_airAbsorption", 0.0f);
			SoundSource::SetAirAbsorption(airAbsorption);
		}

		// Generate sound sources
		for (int i = 0; i < maxSounds; i++)
		{
			SoundSource* thenewone = new SoundSource();
			if (thenewone->IsValid())
			{
				sources.push_back(thenewone);
			}
			else
			{
				maxSounds = std::max(i-1,0);
				LogObject(LOG_SOUND) << "Your hardware/driver can not handle more than " << maxSounds << " soundsources";
				delete thenewone;
				break;
			}
		}

		// Set distance model (sound attenuation)
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		//alDopplerFactor(1.0);

		alListenerf(AL_GAIN, masterVolume);
	}
	configHandler->Set("MaxSounds", maxSounds);

	while (!soundThreadQuit) {
		boost::this_thread::sleep(boost::posix_time::millisec(50));
		Update();
	}

	sources.clear(); // delete all sources
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
	alListener3f(AL_POSITION, myPos.x, myPos.y, myPos.z);

	SoundSource::SetHeightRolloffModifer(std::min(5.*600./std::max(campos.y, 200.f), 5.0));
	//TODO: reactivate when it does not go crazy on camera "teleportation" or fast movement,
	// like when clicked on minimap
	//const float3 velocity = (myPos - prevPos)*(10.0/myPos.y)/(lastFrameTime);
	//const float3 velocityAvg = (velocity+prevVelocity)/2;
	//alListener3f(AL_VELOCITY, velocityAvg.x, velocityAvg.y , velocityAvg.z);
	//prevVelocity = velocity;

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
		if (!success)
		{
			LogObject(LOG_SOUND) << "Failed to load file: " << path;
			return 0;
		}
		else
		{
			return SoundBuffer::Insert(buffer);
		}
	}
}

void CSound::NewFrame()
{
	Channels::General.UpdateFrame();
	Channels::Battle.UpdateFrame();
	Channels::UnitReply.UpdateFrame();
	Channels::UserInterface.UpdateFrame();
}
