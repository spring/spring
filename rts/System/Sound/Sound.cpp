#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"

#include <AL/alc.h>

#include "SoundSource.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "OggStream.h"
#include "GlobalUnsynced.h"
#include "Platform/errorhandler.h"
#include "Lua/LuaParser.h"

// Ogg-Vorbis audio stream object
COggStream oggStream;

CSound* sound = NULL;

bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		logOutput << msg << ": " << (char*)alGetString(e) << "\n";
		return false;
	}
	return true;
}

CSound::CSound() : numEmptyPlayRequests(0), parser("gamedata/sounds.lua", SPRING_VFS_MOD, SPRING_VFS_ZIP)
{
	mute = false;
	int maxSounds = configHandler.Get("MaxSounds", 16) - 1; // 1 source is occupied by eventual music (handled by OggStream)
	globalVolume = configHandler.Get("SoundVolume", 60) * 0.01f;
	unitReplyVolume = configHandler.Get("UnitReplyVolume", 80 ) * 0.01f;

	if (maxSounds <= 0)
	{
		logOutput.Print("MaxSounds set to 0, sound is disabled");
	}
	else
	{
		ALCdevice *device = alcOpenDevice(NULL);
		logOutput.Print("OpenAL: using device: %s\n", (const char*)alcGetString(device, ALC_DEVICE_SPECIFIER));
		if (device == NULL)
		{
			logOutput.Print("Could not open a sounddevice, disabling sounds");
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
				logOutput.Print("Could not create OpenAL audio context");
				return;
			}
		}

		logOutput.Print("OpenAL: %s\n", (const char*)alGetString(AL_VERSION));
		logOutput.Print("OpenAL: %s",   (const char*)alGetString(AL_EXTENSIONS));

		// Generate sound sources
		sources.resize(maxSounds);

		// Set distance model (sound attenuation)
		alDistanceModel (AL_INVERSE_DISTANCE);
		alDopplerFactor(0.1);
	}

	buffers.resize(1); // empty ("zero") buffer
	
	soundItemDef temp;
	temp["name"] = "EmptySource";
	SoundItem* empty = new SoundItem(buffers[0], temp);
	sounds.push_back(empty);
	posScale.x = 0.02f;
	posScale.y = 0.0005f;
	posScale.z = 0.02f;

	parser.Execute();
	if (!parser.IsValid()) {
		logOutput.Print("Could not load gamedata/sounds.lua:");
		logOutput.Print(parser.GetErrorLog());
	}
	else
	{
		soundRoot = parser.GetRoot();
		soundItemTable = soundRoot.SubTable("SoundItems");
	}
}

CSound::~CSound()
{
	if (!sources.empty())
	{
		sources.clear(); // delete all sources
		sounds.clear();
		buffers.clear(); // delete all buffers
		ALCcontext *curcontext = alcGetCurrentContext();
		ALCdevice *curdevice = alcGetContextsDevice(curcontext);
		alcMakeContextCurrent(NULL);
		alcDestroyContext(curcontext);
		alcCloseDevice(curdevice);
	}
}

size_t CSound::GetSoundId(const std::string& name, bool hardFail)
{
	GML_RECMUTEX_LOCK(sound);
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

		const LuaTable soundItem = soundItemTable.SubTable(name);
		if (soundItem.IsValid())
		{
			soundItemDef temp;
			soundItem.GetMap(temp);
			temp["name"] = name;
			soundItemDef::const_iterator it = temp.find("file");
			if (it == temp.end())
			{
				if (hardFail)
					ErrorMessageBox("SoundItem does not have file specified ", name, 0);
				else
				{
					logOutput.Print("SoundItem %s has no file tag", name.c_str());
					return 0;
				}
			}
			else
			{
				//logOutput.Print("CSound::GetSoundId: %s points to %s", name.c_str(), it->second.c_str());
				sounds.push_back(new SoundItem(GetWaveBuffer(it->second, hardFail), temp));
				soundMap[name] = newid;
				return newid;
			}
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
					logOutput.Print("CSound::GetSoundId: could not find sound %s", name.c_str());
					return 0;
				}
			}
		}
	}
}

void CSound::PlayStream(const std::string& path, float volume, const float3& pos, bool loop)
{
	GML_RECMUTEX_LOCK(sound);
	oggStream.Play(path, pos * posScale, volume);
}

void CSound::StopStream()
{
	GML_RECMUTEX_LOCK(sound); // StopStream
	oggStream.Stop();
}

void CSound::PauseStream()
{
	GML_RECMUTEX_LOCK(sound); // PauseStream
	oggStream.TogglePause();
}

unsigned int CSound::GetStreamTime()
{
	GML_RECMUTEX_LOCK(sound); // GetStreamTime
	return oggStream.GetTotalTime();
}

unsigned int CSound::GetStreamPlayTime()
{
	GML_RECMUTEX_LOCK(sound); // GetStreamPlayTime
	return oggStream.GetPlayTime();
}

void CSound::SetStreamVolume(float v)
{
	GML_RECMUTEX_LOCK(sound); // SetStreamVolume
	oggStream.SetVolume(v);
}

void CSound::PitchAdjust(const float newPitch)
{
	SoundSource::SetPitch(newPitch);
}

bool CSound::Mute()
{
	mute = !mute;
	if (mute)
		alListenerf(AL_GAIN, 0.0);
	else
		alListenerf(AL_GAIN, globalVolume);
	return mute;
}

bool CSound::IsMuted() const
{
	return mute;
}

void CSound::SetVolume(float v)
{
	globalVolume = v;
}

void CSound::PlaySample(size_t id, float volume)
{
	PlaySample(id, ZeroVector, ZeroVector, volume, true);
}


void CSound::PlaySample(size_t id, const float3& p, float volume)
{
	PlaySample(id, p, ZeroVector, volume, false);
}

void CSound::PlaySample(size_t id, const float3& p, const float3& velocity, float volume)
{
	PlaySample(id, p, velocity, volume, false);
}

void CSound::PlaySample(size_t id, CUnit* u,float volume)
{
	PlaySample(id, u->pos, u->speed, volume);
}

void CSound::PlaySample(size_t id, CWorldObject* p,float volume)
{
	PlaySample(id,p->pos,volume);
}

void CSound::PlayUnitActivate(size_t id, CUnit* p, float volume)
{
	PlaySample (id, p, volume);
}

void CSound::PlayUnitReply(size_t id, CUnit * p, float volume, bool squashDupes)
{
	GML_RECMUTEX_LOCK(sound); // PlayUnitReply

	if (squashDupes) {
		/* HACK Squash multiple command acknowledgements in the same frame, so
		   we aren't deafened by the construction horde, or the metalmaker
		   horde. */

		/* If we've already played the sound this frame, don't play it again. */
		if (repliesPlayed.find(id) != repliesPlayed.end()) {
			return;
		}
		
		repliesPlayed.insert(id);
	}

	PlaySample(id, volume * unitReplyVolume);
}

void CSound::PlaySample(size_t id, const float3& p, const float3& velocity, float volume, bool relative)
{
	GML_RECMUTEX_LOCK(sound); // PlaySample

	if (sources.empty() || mute || volume == 0.0f || globalVolume == 0.0f)
		return;

	if (id == 0)
	{
		numEmptyPlayRequests++;
		return;
	}
	
	if (p.distance(myPos) > sounds[id].MaxDistance())
		return;

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
			if (sources[pos].GetCurrentPriority() < minPriority)
			{
				found1Free = true;
				minPriority = sources[pos].GetCurrentPriority();
				minPos = pos;
			}
		}
	}

	if (found1Free)
		sources[minPos].Play(&sounds[id], p * posScale, velocity, volume);
	CheckError("CSound::PlaySample");
}

void CSound::Update()
{
	GML_RECMUTEX_LOCK(sound); // Update

	oggStream.Update();
	if (sources.empty())
		return;

	CheckError("CSound::Update");
	UpdateListener();
	for (sourceVecT::iterator it = sources.begin(); it != sources.end(); ++it)
		it->Update();
}

void CSound::UpdateListener()
{
	if (sources.empty())
		return;
	assert(camera);
	myPos = camera->pos * posScale;
	//TODO: move somewhere camera related and make accessible for everyone
	const float3 velocity = (myPos - prevPos)/gu->lastFrameTime/7.0;
	prevPos = myPos;
	alListener3f(AL_POSITION, myPos.x, myPos.y, myPos.z);
	alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	ALfloat ListenerOri[] = {camera->forward.x, camera->forward.y, camera->forward.z, camera->up.x, camera->up.y, camera->up.z};
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alListenerf(AL_GAIN, globalVolume);
	CheckError("CSound::UpdateListener");
}

void CSound::PrintDebugInfo()
{
	logOutput.Print("OpenAL Sound System:");
	logOutput.Print("# SoundSources: %lu", sources.size());
	logOutput.Print("# SoundBuffers: %lu", buffers.size());
	logOutput.Print("# SoundItems: %lu", sounds.size());
	logOutput.Print("# PlayRequests for empty sound: %u", numEmptyPlayRequests);
	/*for (soundMapT::const_iterator it = soundMap.begin(); it != soundMap.end(); ++it)
		logOutput.Print("Name: %s Id: %zu", it->first.c_str(), it->second);*/
}

size_t CSound::LoadALBuffer(const std::string& path, bool strict)
{
	CFileHandler file(path);

	std::vector<uint8_t> buf;
	if (file.FileExists()) {
		buf.resize(file.FileSize());
		file.Read(&buf[0], file.FileSize());
	} else {
		if (strict) {
			handleerror(0, "Couldn't open wav file", path.c_str(),0);
		}
		return 0;
	}

	boost::shared_ptr<SoundBuffer> buffer(new SoundBuffer());
	const bool success = buffer->LoadWAV(path, buf, strict);

	CheckError("CSound::LoadALBuffer");
	if (!success)
	{
		return 0;
	}
	else
	{
		size_t bufId = buffers.size();
		buffers.push_back(buffer);
		bufferMap[path] = bufId;
		return bufId;
	}
}


size_t CSound::GetWaveId(const std::string& path, bool hardFail)
{
	GML_RECMUTEX_LOCK(sound); // GetWaveId
	if (sources.empty())
		return 0;
	bufferMapT::const_iterator it = bufferMap.find(path);
	if (it != bufferMap.end()) {
		return it->second;
	}

	const size_t buffer = LoadALBuffer(path, hardFail);
	return buffer;
}

boost::shared_ptr<SoundBuffer> CSound::GetWaveBuffer(const std::string& path, bool hardFail)
{
	return buffers[GetWaveId(path, hardFail)];
}

void CSound::NewFrame()
{
	GML_RECMUTEX_LOCK(sound); // NewFrame

	repliesPlayed.clear();
}


void CSound::SetUnitReplyVolume (float vol)
{
	unitReplyVolume = vol;
}
