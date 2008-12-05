// Sound.cpp: implementation of the COpenALSound class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Sound.h"

#include <AL/alc.h>

#include "LogOutput.h"
#include "ConfigHandler.h"
#include "Game/Camera.h"
#include "Sim/Objects/WorldObject.h"
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"
#include "SDL_types.h"
#include "Platform/byteorder.h"
#include "OpenALSound.h"
#include "OggStream.h"
#include "mmgr.h"
#include "Exceptions.h"

// Ogg-Vorbis audio stream object
COggStream oggStream;


COpenALSound::COpenALSound()
{
	Sources = NULL;

	maxSounds = configHandler.Get("MaxSounds", 16);
	if (maxSounds <= 0) {
		throw content_error("Internal error, (maxSounds <= 0) in COpenALSound");
	}

	globalVolume = 1.0f;

	cur = 0;

	ALCdevice *device = alcOpenDevice(NULL);
	if (device == NULL) {
		throw content_error("Could not create OpenAL audio device");
	} else {
		ALCcontext *context = alcCreateContext(device, NULL);
		if (context != NULL) {
			alcMakeContextCurrent(context);
		} else {
			alcCloseDevice(device);
			throw content_error("Could not create OpenAL audio context");
		}
	}

	logOutput.Print("OpenAL: %s\n", (const char*)alGetString(AL_VERSION));
	logOutput.Print("OpenAL: %s",   (const char*)alGetString(AL_EXTENSIONS));

	// Generate sound sources
	Sources = SAFE_NEW ALuint[maxSounds];
	for (int a=0;a<maxSounds;a++) {
		Sources[a]=0;
	}

	// Set distance model (sound attenuation)
	alDistanceModel (AL_INVERSE_DISTANCE);

	posScale.x = 0.02f;
	posScale.y = 0.0005f;
	posScale.z = 0.02f;
}


COpenALSound::~COpenALSound()
{
	for (int i = 0; i < maxSounds; i++) {
		alSourceStop(Sources[i]);
		alDeleteSources(1,&Sources[i]);
	}
	delete[] Sources;
	std::map<std::string, ALuint>::iterator it;
	for (it = soundMap.begin(); it != soundMap.end(); ++it) {
		alDeleteBuffers(1, &it->second);
	}
	ALCcontext *curcontext = alcGetCurrentContext();
	//ALCdevice *curdevice = alcGetContextsDevice(curcontext);
	alcSuspendContext(curcontext);
	/*
	 * FIXME
	 * Technically you're supposed to detach and destroy the
	 * current context with these two lines, but it deadlocks.
	 * As a not-quite-as-clean shortcut, if we skip this step
	 * and just close the device, OpenAL theoretically
	 * destroys the context associated with that device.
	 *
	 * alcMakeContextCurrent(NULL);
	 * alcDestroyContext(curcontext);
	 */
	/*
	 * FIXME
	 * Technically you're supposed to close the device, but
	 * the OpenAL sound thread crashes if we do this manually.
	 * The device seems to be closed automagically anyway.
	 *
	 *  alcCloseDevice(curdevice);
	 */
}


static bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR) {
		logOutput << msg << ": " << (char*)alGetString(e) << "\n";
		return false;
	}
	return true;
}


void COpenALSound::PlayStream(const std::string& path, float volume, const float3& pos, bool loop)
{
	GML_RECMUTEX_LOCK(sound); // PlayStream
/*
	if (volume <= 0.0f) {
		return;
	}

	ALuint source;
	alGenSources(1, &source);

	// it seems OpenAL running on Windows is giving problems when too many
	// sources are allocated at a time, in which case it still generates errors.
	if (alGetError () != AL_NO_ERROR) {
		return;
	}
//	if (!CheckError("error generating OpenAL sound source"))
//		return;

	if (Sources[cur]) {
		// The Linux port of OpenAL generates an "Illegal call" error
		// if we delete a playing source, so we must stop it first. -- tvo
		alSourceStop(Sources[cur]);
		alDeleteSources(1, &Sources[cur]);
	}
	Sources[cur++] = source;
	if (cur == maxSounds) {
		cur = 0;
	}

	alSourcei(source, AL_BUFFER, 0); // FIXME id);
	alSourcef(source, AL_PITCH, 1.0f);
	alSourcef(source, AL_GAIN, volume);

	const bool relative = true; // FIXME

	float3 p(0.0f, 0.0f, 0.0f);;
	if (pos != NULL) {
		p = (*pos) * posScale;
	}
	alSource3f(source, AL_POSITION, p.x,  p.y,  p.z);
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	alSourcei(source, AL_LOOPING, false);
	alSourcei(source, AL_SOURCE_RELATIVE, pos == NULL);
	alSourcePlay(source);
	CheckError("COpenALSound::PlaySample");
*/

	oggStream.Play(path, pos * posScale, volume);
}

void COpenALSound::StopStream() {
	GML_RECMUTEX_LOCK(sound); // StopStream

	oggStream.Stop();
}

void COpenALSound::PauseStream() {
	GML_RECMUTEX_LOCK(sound); // PauseStream

	oggStream.TogglePause();
}

unsigned int COpenALSound::GetStreamTime() {
	GML_RECMUTEX_LOCK(sound); // GetStreamTime

	return oggStream.GetTotalTime();
}

unsigned int COpenALSound::GetStreamPlayTime() {
	GML_RECMUTEX_LOCK(sound); // GetStreamPlayTime

	return oggStream.GetPlayTime();
}

void COpenALSound::SetStreamVolume(float v) {
	GML_RECMUTEX_LOCK(sound); // SetStreamVolume

	oggStream.SetVolume(v);
}



void COpenALSound::SetVolume(float v)
{
	globalVolume = v;
}


void COpenALSound::PlaySample(int id, float volume)
{
	if (!camera) {
		return;
	}
	PlaySample(id, float3(0, 0, 0), volume, true);
}


void COpenALSound::PlaySample(int id, const float3& p, float volume)
{
	PlaySample(id, p, volume, false);
}


void COpenALSound::PlaySample(int id, const float3& p, float volume, bool relative)
{
	GML_RECMUTEX_LOCK(sound); // PlaySample

	assert(volume >= 0.0f);

	if (volume == 0.0f || globalVolume == 0.0f) return;

	ALuint source;
	alGenSources(1, &source);

	/* it seems OpenAL running on Windows is giving problems when too many sources are allocated at a time,
	in which case it still generates errors. */
	if (alGetError() != AL_NO_ERROR)
		return;

	if (Sources[cur]) {
		/* The Linux port of OpenAL generates an "Illegal call" error if we
		delete a playing source, so we must stop it first. -- tvo */
		alSourceStop(Sources[cur]);
		alDeleteSources(1, &Sources[cur]);
	}
	Sources[cur++] = source;
	if (cur == maxSounds)
		cur = 0;

	alSourcei(source, AL_BUFFER, id);
	alSourcef(source, AL_PITCH, 1.0f);
	alSourcef(source, AL_GAIN, volume);

	float3 pos = p * posScale;
	alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
	alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	alSourcei(source, AL_LOOPING, false);
	alSourcei(source, AL_SOURCE_RELATIVE, relative);
	alSourcePlay(source);
	CheckError("COpenALSound::PlaySample");
}



void COpenALSound::Update()
{
	GML_RECMUTEX_LOCK(sound); // Update

	oggStream.Update();

	for (int a = 0; a < maxSounds; a++) {
		if (Sources[a]) {
			ALint state;
			alGetSourcei(Sources[a], AL_SOURCE_STATE, &state);

			if (state == AL_STOPPED) {
				alDeleteSources(1, &Sources[a]);
				Sources[a] = 0;
			}
		}
	}

	CheckError("COpenALSound::Update");
	UpdateListener();
}



void COpenALSound::UpdateListener()
{
	if (!camera) {
		return;
	}
	float3 pos = camera->pos * posScale;
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	ALfloat ListenerOri[] = {camera->forward.x, camera->forward.y, camera->forward.z, camera->up.x, camera->up.y, camera->up.z};
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alListenerf(AL_GAIN, globalVolume);
	CheckError("COpenALSound::UpdateListener");
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


bool COpenALSound::ReadWAV(const char* name, Uint8* buf, int size, ALuint albuffer)
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


ALuint COpenALSound::LoadALBuffer(const std::string& path)
{
	Uint8* buf = 0;
	ALuint buffer;
	alGenBuffers(1, &buffer);

	if (!CheckError("error generating OpenAL sound buffer"))
		return 0;

	CFileHandler file(path);

	if (file.FileExists()) {
		buf = SAFE_NEW Uint8[file.FileSize()];
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


ALuint COpenALSound::GetWaveId(const std::string& path, bool _hardFail)
{
	GML_RECMUTEX_LOCK(sound); // GetWaveId

	std::map<std::string, ALuint>::const_iterator it = soundMap.find(path);
	if (it != soundMap.end()) {
		return it->second;
	}

	hardFail = _hardFail;
	const ALuint buffer = LoadALBuffer(path);
	soundMap[path] = buffer;
	return buffer;
}
