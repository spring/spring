#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"

#include <AL/alc.h>

#include "SoundSource.h"
#include "SoundBuffer.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "OggStream.h"
#include "GlobalUnsynced.h"
#include "Platform/errorhandler.h"

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

CSound::CSound()
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

	cur = 0;
	buffers.resize(1); // empty ("zero") buffer
	posScale.x = 0.02f;
	posScale.y = 0.0005f;
	posScale.z = 0.02f;
}

CSound::~CSound()
{
	if (!sources.empty())
	{
		sources.clear(); // delete all sources
		soundMap.clear();
		buffers.clear(); // delete all buffers
		ALCcontext *curcontext = alcGetCurrentContext();
		ALCdevice *curdevice = alcGetContextsDevice(curcontext);
		alcMakeContextCurrent(NULL);
		alcDestroyContext(curcontext);
		alcCloseDevice(curdevice);
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
	for (sourceVec::const_iterator it = sources.begin(); it != sources.end(); ++it)
	{
		it->SetPitch(newPitch);
	}
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

	if (sources.empty() || mute || volume == 0.0f || globalVolume == 0.0f || id == 0)
		return;

	if (sources[cur].IsPlaying())
	{
		for (size_t pos = 0; pos != sources.size(); ++pos)
		{
			if (!sources[pos].IsPlaying())
			{
				cur = pos;
				break;
			}
		}
	}
	
	sources[cur++].Play(buffers[id], p * posScale, velocity, volume, relative);
	if (cur == sources.size())
		cur = 0;
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
}

void CSound::UpdateListener()
{
	if (sources.empty())
		return;
	assert(camera);
	float3 pos = camera->pos * posScale;
	//TODO: move somewhere camera related and make accessible for everyone
	const float3 velocity = (pos - prevPos)/gu->lastFrameTime/7.0;
	prevPos = pos;
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	ALfloat ListenerOri[] = {camera->forward.x, camera->forward.y, camera->forward.z, camera->up.x, camera->up.y, camera->up.z};
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alListenerf(AL_GAIN, globalVolume);
	CheckError("CSound::UpdateListener");
}


size_t CSound::LoadALBuffer(const std::string& path)
{
	CFileHandler file(path);

	std::vector<uint8_t> buf;
	if (file.FileExists()) {
		buf.resize(file.FileSize());
		file.Read(&buf[0], file.FileSize());
	} else {
		if (hardFail) {
			handleerror(0, "Couldn't open wav file", path.c_str(),0);
		}
		return 0;
	}

	SoundBuffer* buffer = new SoundBuffer();
	const bool success = buffer->LoadWAV(path, buf, hardFail);

	CheckError("CSound::LoadALBuffer");
	if (!success)
	{
		delete buffer;
		return 0;
	}
	else
	{
		size_t bufId = buffers.size();
		buffers.push_back(buffer);
		soundMap[path] = bufId;
		return bufId;
	}
}


size_t CSound::GetWaveId(const std::string& path, bool _hardFail)
{
	GML_RECMUTEX_LOCK(sound); // GetWaveId
	if (sources.empty())
		return 0;
	bufferMap::const_iterator it = soundMap.find(path);
	if (it != soundMap.end()) {
		return it->second;
	}

	hardFail = _hardFail;
	const size_t buffer = LoadALBuffer(path);
	return buffer;
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
