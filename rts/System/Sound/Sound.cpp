#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"

#include <cstdlib>
#include <cmath>
#include <AL/alc.h>
#include <boost/cstdint.hpp>

#include "SoundSource.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "AudioChannel.h"
#include "ALShared.h"

#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"
#include "Lua/LuaParser.h"
#include "Rendering/GL/myGL.h" // I HATE IT

CSound* sound = NULL;

CSound::CSound() : prevVelocity(0.0, 0.0, 0.0), numEmptyPlayRequests(0), updateCounter(0)
{
	mute = false;
	appIsIconified = false;
	int maxSounds = configHandler->Get("MaxSounds", 64) - 1; // 1 source is occupied by eventual music (handled by OggStream)
	pitchAdjust = configHandler->Get("PitchAdjust", true);

	masterVolume = configHandler->Get("snd_volmaster", 60) * 0.01f;
	Channels::General.SetVolume(configHandler->Get("snd_volgeneral", 100 ) * 0.01f);
	Channels::UnitReply.SetVolume(configHandler->Get("snd_volunitreply", 100 ) * 0.01f);
	Channels::UnitReply.SetMaxEmmits(1);
	Channels::Battle.SetVolume(configHandler->Get("snd_volbattle", 100 ) * 0.01f);
	Channels::UserInterface.SetVolume(configHandler->Get("snd_volui", 100 ) * 0.01f);

	if (maxSounds <= 0)
	{
		LogObject(LOG_SOUND) << "MaxSounds set to 0, sound is disabled";
	}
	else
	{
		//TODO: device choosing, like this:
		//const ALchar* deviceName = "ALSA Software on SB Live 5.1 [SB0220] [Multichannel Playback]";
		const ALchar* deviceName = NULL;
		ALCdevice *device = alcOpenDevice(deviceName);

		if (device == NULL)
		{
			LogObject(LOG_SOUND) <<  "Could not open a sounddevice, disabling sounds";
			CheckError("CSound::InitAL");
			return;
		} else
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

		LogObject(LOG_SOUND) << "OpenAL info:\n";
		LogObject(LOG_SOUND) << "  Vendor:     " << (const char*)alGetString(AL_VENDOR );
		LogObject(LOG_SOUND) << "  Version:    " << (const char*)alGetString(AL_VERSION);
		LogObject(LOG_SOUND) << "  Renderer:   " << (const char*)alGetString(AL_RENDERER);
		LogObject(LOG_SOUND) << "  AL Extensions: " << (const char*)alGetString(AL_EXTENSIONS);
		LogObject(LOG_SOUND) << "  ALC Extensions: " << (const char*)alcGetString(device, ALC_EXTENSIONS);
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

		// Generate sound sources
		for (int i = 0; i < maxSounds; i++) {
			sources.push_back(new SoundSource());
			if (!sources[i].IsValid())
			{
				sources.pop_back();
				maxSounds = i-1;
				LogObject(LOG_SOUND) << "Your hardware/driver can not handle more than " << maxSounds << " soundsources";
				break;
			}
		}

		// Set distance model (sound attenuation)
		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
		//alDopplerFactor(1.0);

		alListenerf(AL_GAIN, masterVolume);
	}

	SoundBuffer::Initialise();
	soundItemDef temp;
	temp["name"] = "EmptySource";
	SoundItem* empty = new SoundItem(SoundBuffer::GetById(0), temp);
	sounds.push_back(empty);

	LoadSoundDefs("gamedata/sounds.lua");

	configHandler->NotifyOnChange(this);
}

CSound::~CSound()
{
	if (!sources.empty())
	{
		sources.clear(); // delete all sources
		sounds.clear();
		SoundBuffer::Deinitialise();
		ALCcontext *curcontext = alcGetCurrentContext();
		ALCdevice *curdevice = alcGetContextsDevice(curcontext);
		alcMakeContextCurrent(NULL);
		alcDestroyContext(curcontext);
		alcCloseDevice(curdevice);
	}
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
	GML_RECMUTEX_LOCK(sound); // GetSoundId

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
		size_t newid = sounds.size();

		soundItemDefMap::iterator itemDefIt = soundItemDefs.find(name);
		if (itemDefIt != soundItemDefs.end())
		{
			//logOutput.Print("CSound::GetSoundId: %s points to %s", name.c_str(), it->second.c_str());
			sounds.push_back(new SoundItem(GetWaveBuffer(itemDefIt->second["file"], hardFail), itemDefIt->second));
			soundMap[name] = newid;
			return newid;
		}
		else
		{
			if (GetWaveId(name, hardFail) > 0) // maybe raw filename?
			{
				soundItemDef temp = defaultItem;
				temp["name"] = name;
				sounds.push_back(new SoundItem(GetWaveBuffer(name, hardFail), temp)); // use raw file with default values
				soundMap[name] = newid;
				return newid;
			}
			else
			{
				if (hardFail)
					ErrorMessageBox("Couldn't open wav file", name, 0);
				else
				{
					LogObject(LOG_SOUND) << "CSound::GetSoundId: could not find sound: " << name;
					return 0;
				}
			}
		}
	}

	return 0;
}

void CSound::PitchAdjust(const float newPitch)
{
	if (pitchAdjust)
		SoundSource::SetPitch(newPitch);
}

void CSound::ConfigNotify(const std::string& key, const std::string& value)
{
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
	GML_RECMUTEX_LOCK(sound); // PlaySample

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

	bool found1Free = false;
	int minPriority = 1;
	size_t minPos = 0;

	for (size_t pos = 0; pos != sources.size(); ++pos)
	{
		if (!sources[pos].IsPlaying())
		{
			minPos = pos;
			found1Free = true;
			break;
		}
		else
		{
			if (sources[pos].GetCurrentPriority() < minPriority && sources[pos].GetCurrentPriority() <=  sounds[id].GetPriority())
			{
				found1Free = true;
				minPriority = sources[pos].GetCurrentPriority();
				minPos = pos;
			}
		}
	}

	if (found1Free)
		sources[minPos].Play(&sounds[id], p, velocity, volume, relative);
	CheckError("CSound::PlaySample");
}

void CSound::Update()
{
	updateCounter++;

	GML_RECMUTEX_LOCK(sound); // Update

	if (sources.empty())
		return;

	Channels::General.UpdateFrame();
	Channels::Battle.UpdateFrame();
	Channels::UnitReply.UpdateFrame();
	Channels::UserInterface.UpdateFrame();

	if (updateCounter % 2)
	{
		for (sourceVecT::iterator it = sources.begin(); it != sources.end(); ++it)
			it->Update();
	}
	CheckError("CSound::Update");
}

void CSound::UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime)
{
	GML_RECMUTEX_LOCK(sound); // UpdateListener

	if (sources.empty())
		return;
	const float3 prevPos = myPos;
	myPos = campos;
	alListener3f(AL_POSITION, myPos.x, myPos.y, myPos.z);

	SoundSource::SetHeightRolloffModifer(std::min(5.*600./campos.y, 5.0));
	//TODO: reactivate when it does nto go crazy on camera "teleportation" or fast movement,
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
	LogObject(LOG_SOUND) << "OpenAL Sound System:";
	LogObject(LOG_SOUND) << "# SoundSources: " << sources.size();
	LogObject(LOG_SOUND) << "# SoundBuffers: " << SoundBuffer::Count();

	LogObject(LOG_SOUND) << "# reserved for buffers: " << (SoundBuffer::AllocedSize()/1024) << " kB";
	LogObject(LOG_SOUND) << "# PlayRequests for empty sound: " << numEmptyPlayRequests;
	LogObject(LOG_SOUND) << "# SoundItems: " << sounds.size();
}

bool CSound::LoadSoundDefs(const std::string& filename)
{
	LuaParser parser(filename, SPRING_VFS_MOD, SPRING_VFS_ZIP);
	parser.SetLowerKeys(false);
	parser.SetLowerCppKeys(false);
	parser.Execute();
	if (!parser.IsValid())
	{
		LogObject(LOG_SOUND) << "Could not load " << filename << ": " << parser.GetErrorLog();
		return false;
	}
	else
	{
		const LuaTable soundRoot = parser.GetRoot();
		const LuaTable soundItemTable = soundRoot.SubTable("SoundItems");
		if (!soundItemTable.IsValid())
		{
			LogObject(LOG_SOUND) << "CSound(): could not parse SoundItems table in " << filename;
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
					LogObject(LOG_SOUND) << "CSound(): two SoundItems have the same name: " << name;

				soundItemDef::const_iterator inspec = bufmap.find("file");
				if (inspec == bufmap.end())	// no file, drop
					LogObject(LOG_SOUND) << "CSound(): SoundItem has no file tag: " << name;
				else
					soundItemDefs[name] = bufmap;

				if (buf.KeyExists("preload"))
				{
					LogObject(LOG_SOUND) << "CSound(): preloading " << name;
					const size_t newid = sounds.size();
					sounds.push_back(new SoundItem(GetWaveBuffer(bufmap["file"], true), bufmap));
					soundMap[name] = newid;
				}
			}
			LogObject(LOG_SOUND) << "CSound(): Sucessfully parsed " << keys.size() << " SoundItems from " << filename;
		}
	}
	return true;
}

size_t CSound::LoadALBuffer(const std::string& path, bool strict)
{
	assert(path.length() > 3);
	CFileHandler file(path);

	std::vector<boost::uint8_t> buf;
	if (file.FileExists()) {
		buf.resize(file.FileSize());
		file.Read(&buf[0], file.FileSize());
	} else {
		if (strict) {
			handleerror(0, "Couldn't open audio file", path.c_str(),0);
		}
		else
			LogObject(LOG_SOUND) << "Unable to open audio file: " << path;
		return 0;
	}

	boost::shared_ptr<SoundBuffer> buffer(new SoundBuffer());
	bool success = false;
	std::string ending = path.substr(path.length()-3);
	std::transform(ending.begin(), ending.end(), ending.begin(), (int (*)(int))tolower);
	if (ending == "wav")
		success = buffer->LoadWAV(path, buf, strict);
	else if (ending == "ogg")
		success = buffer->LoadVorbis(path, buf, strict);
	else
	{
		LogObject(LOG_SOUND) << "CSound::LoadALBuffer: unknown audio format: " << ending;
	}

	CheckError("CSound::LoadALBuffer");
	if (!success)
	{
		return 0;
	}
	else
	{
		return SoundBuffer::Insert(buffer);
	}
}


size_t CSound::GetWaveId(const std::string& path, bool hardFail)
{
	GML_RECMUTEX_LOCK(sound); // GetWaveId

	if (sources.empty())
		return 0;

	const size_t id = SoundBuffer::GetId(path);
	return (id == 0) ? LoadALBuffer(path, hardFail) : id;
}

boost::shared_ptr<SoundBuffer> CSound::GetWaveBuffer(const std::string& path, bool hardFail)
{
	return SoundBuffer::GetById(GetWaveId(path, hardFail));
}

void CSound::NewFrame()
{
}
