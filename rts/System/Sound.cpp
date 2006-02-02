// Sound.cpp: implementation of the CSound class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Sound.h"

#include <math.h>
#include "Game/UI/InfoConsole.h"
#include "Platform/ConfigHandler.h"
#include "Game/Camera.h"
#include "Sim/Objects/WorldObject.h"
#include "FileSystem/FileHandler.h"
#include "Platform/errorhandler.h"
#include "SDL_types.h"
#include "Platform/byteorder.h"
#include "mmgr.h"

CSound* sound;

CSound::CSound()
{
	maxSounds = configHandler.GetInt("MaxSounds",16);
	noSound = false;
	cur = 0;
	ALCdevice *device = alcOpenDevice(NULL);
	if (device != NULL) {
		ALCcontext *context = alcCreateContext(device, NULL);
		if (context != NULL)
			alcMakeContextCurrent(context);
		else {
			handleerror(0, "Could not create audio context","OpenAL error",MBF_OK);
			noSound = true;
			alcCloseDevice(device);
			return;
		}
	} else {
		handleerror(0,"Could not create audio device","OpenAL error",MBF_OK);
		noSound = true;
		return;
	}

	// Generate sound sources
	Sources = new ALuint[maxSounds];
	for (int a=0;a<maxSounds;a++) Sources[a]=0;
}

CSound::~CSound()
{
	if (noSound)
		return;
	LoadedFiles.clear();
	alDeleteSources(maxSounds,Sources);
	delete[] Sources;
	while (!Buffers.empty()) {
		alDeleteBuffers(1,&Buffers.back());
		Buffers.pop_back();
	}
	ALCcontext *curcontext = alcGetCurrentContext();
	ALCdevice *curdevice = alcGetContextsDevice(curcontext);
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
	alcCloseDevice(curdevice);
}

static bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR) {
		(*info) << msg << ": " << (char*)alGetString(e) << "\n";
		return false;
	}
	return true;
}

void CSound::PlaySound(int id, float volume)
{
	if (noSound || !camera)
		return;
	PlaySound(id,camera->pos,volume);
}

void CSound::PlaySound(int id,CWorldObject* p,float volume)
{
	if (noSound)
		return;
	PlaySound(id,p->pos,volume);
}

void CSound::PlaySound(int id,const float3& p,float volume)
{
	if (noSound)
		return;
	ALuint source;
	alGenSources(1,&source);

	if (!CheckError("error generating OpenAL sound source"))
		return;

	if (Sources[cur]) {
		/* The Linux port of OpenAL generates an "Illegal call" error if we
		delete a playing source, so we must stop it first. -- tvo */
		alSourceStop(Sources[cur]);
		alDeleteSources(1,&Sources[cur]);
	}
	Sources[cur++] = source;
	if (cur == maxSounds)
		cur = 0;

	alSourcei(source, AL_BUFFER, id);
	alSourcef(source, AL_PITCH, 1.0f );
	alSourcef(source, AL_GAIN, volume );
	alSource3f(source, AL_POSITION, p.x/(gs->mapx*SQUARE_SIZE),p.y/(gs->mapy*SQUARE_SIZE),p.z/(p.maxzpos));
	alSource3f(source, AL_VELOCITY, 0.0f,0.0f,0.0f);
	alSourcei(source, AL_LOOPING, false);
	alSourcePlay(source);
	CheckError("CSound::PlaySound");
}

void CSound::Update()
{
	if (noSound)
		return;

	for (int a=0;a<maxSounds;a++) {
		if (Sources[a]) {
			ALint state;
			alGetSourcei(Sources[a],AL_SOURCE_STATE, &state);
			if (state == AL_STOPPED) {
				alDeleteSources(1,&Sources[a]);
				Sources[a]=0;
			}
		}
	}
	CheckError("CSound::Update");

	UpdateListener();
}

void CSound::UpdateListener()
{
	if (noSound || !camera)
		return;
	alListener3f(AL_POSITION,camera->pos.x/(gs->mapx*SQUARE_SIZE),camera->pos.y/(gs->mapy*SQUARE_SIZE),camera->pos.z/(camera->pos.maxzpos));
	alListener3f(AL_VELOCITY,0.0,0.0,0.0);
	ALfloat ListenerOri[] = {camera->forward.x/(gs->mapx*SQUARE_SIZE),camera->forward.y/(gs->mapy*SQUARE_SIZE),camera->forward.z/(camera->pos.maxzpos),camera->up.x/(gs->mapx*SQUARE_SIZE),camera->up.y/(gs->mapy*SQUARE_SIZE),camera->up.z/(camera->pos.maxzpos)};
	alListenerfv(AL_ORIENTATION,ListenerOri);
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

static bool ReadWAV (const char *name, Uint8 *buf, int size, ALuint albuffer)
{
	WAVHeader *header = (WAVHeader *)buf;

	if (memcmp (header->riff, "RIFF",4) || memcmp (header->wavefmt, "WAVEfmt", 7)) {
		handleerror(0, "ReadWAV: invalid header.", name, 0);
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
		handleerror(0,"ReadWAV: invalid format tag.", name, 0);
		return false;
	}

	ALenum format;
	if (header->channels == 1) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_MONO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_MONO16;
		else {
			handleerror(0,"ReadWAV: invalid number of bits per sample (mono).",name,0);
			return false;
		}
	}
	else if(header->channels == 2) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_STEREO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_STEREO16;
		else {
			handleerror(0,"ReadWAV: invalid number of bits per sample (stereo).", name,0);
			return false;
		}
	}
	else {
		handleerror(0,"ReadWAV (%s): invalid number of channels.", name,0);
		return false;
	}

	alBufferData(albuffer,format,buf+sizeof(WAVHeader),header->datalen > size-sizeof(WAVHeader) ? size-sizeof(WAVHeader) : header->datalen,header->SamplesPerSec);
	return CheckError("ReadWAV");
}


ALuint CSound::LoadALBuffer(string path)
{
	if (noSound)
		return 0;
	Uint8 *buf;
	ALuint buffer;
	alGenBuffers(1,&buffer);
	if (!CheckError("error generating OpenAL sound buffer"))
		return 0;
	CFileHandler file("Sounds/"+path);
	if(file.FileExists()){
		buf = new Uint8[file.FileSize()];
		file.Read(buf, file.FileSize());
	} else {
		handleerror(0, "Couldnt open wav file",path.c_str(),0);
		alDeleteBuffers(1, &buffer);
		return 0;
	}
	bool success=ReadWAV (path.c_str(), buf, file.FileSize(), buffer);
	delete[] buf;

	if (!success) {
		alDeleteBuffers(1, &buffer);
		return 0;
	}
	return buffer;
}

ALuint CSound::GetWaveId(string path)
{
	if (noSound)
		return 0;
	int count = 0;
	ALuint buffer;
	for (vector<string>::iterator it = LoadedFiles.begin(); it != LoadedFiles.end(); it++,count++) {
		if (*it == path)
			return Buffers.at(count);
	}
	buffer = LoadALBuffer(path);
	Buffers.push_back(buffer);
	LoadedFiles.push_back(path);
	return buffer;
}
