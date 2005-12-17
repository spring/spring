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
//#include "mmgr.h"

CSound* sound;

CSound::CSound()
{
	maxSounds = configHandler.GetInt("MaxSounds",16);
	noSound = false;
	cur = 0;
	device = alcOpenDevice(NULL);
	if (device != NULL) {
		context = alcCreateContext(device, NULL);
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
	Sources = new ALuint[maxSounds];
}

CSound::~CSound()
{
	LoadedFiles.clear();
	alDeleteSources(maxSounds,Sources);
	if (!noSound)
		delete[] Sources;
	for (std::vector<ALuint>::iterator it = Buffers.begin(); it != Buffers.end(); it++)
		alDeleteBuffers(1,&(*it));
	Buffers.clear();
	if (noSound)
		return;
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
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
	alSourcei(source, AL_BUFFER, id);
	alSourcef(source, AL_PITCH, 1.0 );
	alSourcef(source, AL_GAIN, volume );
	alSource3f(source, AL_POSITION, p.x/(gs->mapx*SQUARE_SIZE),p.y/(gs->mapy*SQUARE_SIZE),p.z/(p.maxzpos));
	alSource3f(source, AL_VELOCITY, 0.0f,0.0f,0.0f);
	alSourcei(source, AL_LOOPING, false);
	Enqueue(source);
	alSourcePlay(source);
}

void CSound::Enqueue(ALuint src)
{
	alDeleteSources(1,&Sources[cur]);
	Sources[cur++] = src;
	if (cur == maxSounds)
		cur = 0;
}

void CSound::Update()
{
	if (noSound)
		return;
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
}

ALuint CSound::LoadALBuffer(string path)
{
	if (noSound)
		return 0;
	Uint8 *buf;
	ALuint buffer;
	alGenBuffers(1,&buffer);
	CFileHandler file("Sounds/"+path);
	if(file.FileExists()){
		buf = new Uint8[file.FileSize()];
		file.Read(buf, file.FileSize());
	} else {
		handleerror(0, "Couldnt open wav file",path.c_str(),0);
		return 0;
	}
	alBufferData(buffer,AL_FORMAT_STEREO8,buf,file.FileSize(),11025);
	delete[] buf;
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
