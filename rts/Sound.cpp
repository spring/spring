// Sound.cpp: implementation of the CSound class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Sound.h"

#include <math.h>
#include "InfoConsole.h"
#include "RegHandler.h"
#include "Camera.h"
#include "WorldObject.h"
#include "FileHandler.h"
//#include "mmgr.h"

/*
 * OpenGL's float coordinates are expressed
 * with a full cartesian system, while
 * OpenAL's coordinates are expressed entirely
 * in the range (0.0,1.0].  So the OpenGL coordinates
 * need to be scaled down so they don't all seem to come
 * from far away
 */
#define SCALEVERTEX(v)	((v)/10)

CSound* sound;

CSound::CSound()
{
	noSound = false;
	alutInit(NULL,0);
	alGetError();
}

CSound::~CSound()
{
	LoadedFiles.clear();
	for (std::vector<ALuint>::iterator it = Buffers.begin(); it != Buffers.end(); ++it)
		alDeleteBuffers(1,&(*it));
	Buffers.clear();
	if (noSound)
		return;
	alutExit();
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
	ALfloat SourcePos[] = {SCALEVERTEX(p.x),SCALEVERTEX(p.y),SCALEVERTEX(p.z)};
	ALfloat SourceVel[] = {0.0,0.0,0.0};
	alGenSources(1,&source);
	alSourcei(source, AL_BUFFER, id);
	alSourcef(source, AL_PITCH, 1.0 );
	alSourcef(source, AL_GAIN, volume );
	alSourcefv(source, AL_POSITION, SourcePos);
	alSourcefv(source, AL_VELOCITY, SourceVel);
	alSourcei(source, AL_LOOPING, false);
	alSourcePlay(source);
	alDeleteSources(1,&source);
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
	ALfloat ListenerPos[] = {SCALEVERTEX(camera->pos.x),SCALEVERTEX(camera->pos.y),SCALEVERTEX(camera->pos.z)};
	ALfloat ListenerVel[] = {0.0,0.0,0.0};
	ALfloat ListenerOri[] = {SCALEVERTEX(camera->forward.x),SCALEVERTEX(camera->forward.y),SCALEVERTEX(camera->forward.z),SCALEVERTEX(camera->up.x),SCALEVERTEX(camera->up.y),SCALEVERTEX(camera->up.z)};
	alListenerfv(AL_POSITION,ListenerPos);
	alListenerfv(AL_VELOCITY,ListenerVel);
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
		MessageBox(0,"Couldnt open wav file",path,0);
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
	for (vector<string>::iterator it = LoadedFiles.begin(); it != LoadedFiles.end(); ++it,count++) {
		if (*it == path)
			return Buffers[count];
	}
	buffer = LoadALBuffer(path);
	Buffers.push_back(buffer);
	LoadedFiles.push_back(path);
	return buffer;
}
