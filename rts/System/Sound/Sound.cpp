#include "StdAfx.h"
#include "mmgr.h"
#include "Sound.h"

#include <AL/alc.h>

#include "SoundSource.h"
#include "Sim/Units/Unit.h"
#include "Game/Camera.h"
#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Exceptions.h"
#include "FileSystem/FileHandler.h"
#include "OggStream.h"
#include "GlobalUnsynced.h"
#include "Platform/errorhandler.h"
#include "Platform/byteorder.h"

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

	posScale.x = 0.02f;
	posScale.y = 0.0005f;
	posScale.z = 0.02f;
}

CSound::~CSound()
{
	if (!sources.empty())
	{
		sources.resize(0); // delete all sources
		std::map<std::string, ALuint>::iterator it;
		for (it = soundMap.begin(); it != soundMap.end(); ++it) {
			alDeleteBuffers(1, &it->second);
		}
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

void CSound::PlaySample(int id, float volume)
{
	PlaySample(id, ZeroVector, ZeroVector, volume, true);
}


void CSound::PlaySample(int id, const float3& p, float volume)
{
	PlaySample(id, p, ZeroVector, volume, false);
}

void CSound::PlaySample(int id, const float3& p, const float3& velocity, float volume)
{
	PlaySample(id, p, velocity, volume, false);
}

void CSound::PlaySample(int id, CUnit* u,float volume)
{
	PlaySample(id, u->pos, u->speed, volume);
}

void CSound::PlaySample(int id, CWorldObject* p,float volume)
{
	PlaySample(id,p->pos,volume);
}

void CSound::PlayUnitActivate(int id, CUnit* p, float volume)
{
	PlaySample (id, p, volume);
}

void CSound::PlayUnitReply(int id, CUnit * p, float volume, bool squashDupes)
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

void CSound::PlaySample(int id, const float3& p, const float3& velocity, float volume, bool relative)
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
	
	sources[cur++].Play(id, p * posScale, velocity, volume, relative);
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


#pragma pack(push, 1)
// Header copied from WavLib by Michael McTernan
struct WAVHeader
{
	Uint8 riff[4];         // "RIFF"
	Sint32 totalLength;
	Uint8 wavefmt[8];      // WAVEfmt "
	Sint32 length;         // Remaining length 4 bytes
	Sint16 format_tag;
	Sint16 channels;       // Mono=1 Stereo=2
	Sint32 SamplesPerSec;
	Sint32 AvgBytesPerSec;
	Sint16 BlockAlign;
	Sint16 BitsPerSample;
	Uint8 data[4];         // "data"
	Sint32 datalen;        // Raw data length 4 bytes
};
#pragma pack(pop)

bool CSound::ReadWAV(const char* name, Uint8* buf, int size, ALuint albuffer)
{
	WAVHeader* header = (WAVHeader*) buf;

	if (memcmp(header->riff, "RIFF", 4) || memcmp(header->wavefmt, "WAVEfmt", 7)) {
		if (hardFail) {
			handleerror(0, "ReadWAV: invalid header.", name, 0);
		}
		return false;
	}

#define hswabword(c) header->c = swabword(header->c)
#define hswabdword(c) header->c = swabdword(header->c)
	hswabword(format_tag);
	hswabword(channels);
	hswabword(BlockAlign);
	hswabword(BitsPerSample);

	hswabdword(totalLength);
	hswabdword(length);
	hswabdword(SamplesPerSec);
	hswabdword(AvgBytesPerSec);
	hswabdword(datalen);
#undef hswabword
#undef hswabdword

	if (header->format_tag != 1) { // Microsoft PCM format?
		if (hardFail) {
			handleerror(0,"ReadWAV: invalid format tag.", name, 0);
		}
		return false;
	}

	ALenum format;
	if (header->channels == 1) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_MONO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_MONO16;
		else {
			if (hardFail) {
				handleerror(0,"ReadWAV: invalid number of bits per sample (mono).", name, 0);
			}
			return false;
		}
	}
	else if (header->channels == 2) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_STEREO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_STEREO16;
		else {
			if (hardFail) {
				handleerror(0,"ReadWAV: invalid number of bits per sample (stereo).", name, 0);
			}
			return false;
		}
	}
	else {
		if (hardFail) {
			handleerror(0, "ReadWAV (%s): invalid number of channels.", name, 0);
		}
		return false;
	}

	if (header->datalen > size - sizeof(WAVHeader)) {
//		logOutput.Print("\n");
		logOutput.Print("OpenAL: %s has data length %d greater than actual data length %ld\n",
						name, header->datalen, size - sizeof(WAVHeader));
//		logOutput.Print("OpenAL: size %d\n", size);
//		logOutput.Print("OpenAL: sizeof(WAVHeader) %d\n", sizeof(WAVHeader));
//		logOutput.Print("OpenAL: format_tag %d\n", header->format_tag);
//		logOutput.Print("OpenAL: channels %d\n", header->channels);
//		logOutput.Print("OpenAL: BlockAlign %d\n", header->BlockAlign);
//		logOutput.Print("OpenAL: BitsPerSample %d\n", header->BitsPerSample);
//		logOutput.Print("OpenAL: totalLength %d\n", header->totalLength);
//		logOutput.Print("OpenAL: length %d\n", header->length);
//		logOutput.Print("OpenAL: SamplesPerSec %d\n", header->SamplesPerSec);
//		logOutput.Print("OpenAL: AvgBytesPerSec %d\n", header->AvgBytesPerSec);

		// FIXME: setting datalen to size - sizeof(WAVHeader) only
		// works for some files that have a garbage datalen field
		// in their header, others cause SEGV's inside alBufferData()
		// (eg. ionbeam.wav in XTA 9.2) -- Kloot
		// header->datalen = size - sizeof(WAVHeader);
		header->datalen = 1;
	}

	alBufferData(albuffer, format, buf + sizeof(WAVHeader), header->datalen, header->SamplesPerSec);
	return CheckError("ReadWAV");
}


ALuint CSound::LoadALBuffer(const std::string& path)
{
	Uint8* buf = 0;
	ALuint buffer;
	alGenBuffers(1, &buffer);

	if (!CheckError("error generating OpenAL sound buffer"))
		return 0;

	CFileHandler file(path);

	if (file.FileExists()) {
		buf = new Uint8[file.FileSize()];
		file.Read(buf, file.FileSize());
	} else {
		if (hardFail) {
			handleerror(0, "Couldn't open wav file", path.c_str(),0);
		}
		alDeleteBuffers(1, &buffer);
		return 0;
	}

	const bool success = ReadWAV(path.c_str(), buf, file.FileSize(), buffer);
	delete[] buf;

	if (!success) {
		alDeleteBuffers(1, &buffer);
		return 0;
	}

	return buffer;
}


ALuint CSound::GetWaveId(const std::string& path, bool _hardFail)
{
	GML_RECMUTEX_LOCK(sound); // GetWaveId
	if (sources.empty())
		return 0;
	std::map<std::string, ALuint>::const_iterator it = soundMap.find(path);
	if (it != soundMap.end()) {
		return it->second;
	}

	hardFail = _hardFail;
	const ALuint buffer = LoadALBuffer(path);
	soundMap[path] = buffer;
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
