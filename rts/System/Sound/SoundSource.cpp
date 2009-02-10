#include "SoundSource.h"

#include <AL/alc.h>
#include <SDL_timer.h>

#include "Sound.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "float3.h"
#include "LogOutput.h"

float SoundSource::globalPitch = 1.0;
namespace
{
bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		LogObject() << msg << ": " << (char*)alGetString(e) << "\n";
		return false;
	}
	return true;
}
}

SoundSource::SoundSource() : curPlaying(0)
{
	alGenSources(1, &id);
	CheckError("SoundSource::SoundSource");
}

SoundSource::~SoundSource()
{
	alDeleteSources(1, &id);
	CheckError("SoundSource::~SoundSource");
}

void SoundSource::Update()
{
	if (curPlaying && (!IsPlaying() || ((curPlaying->loopTime > 0) && (loopStop < SDL_GetTicks()))))
		Stop();
}

int SoundSource::GetCurrentPriority() const
{
	assert(curPlaying);
	return curPlaying->priority;
}

bool SoundSource::IsPlaying() const
{
	ALint state;
	alGetSourcei(id, AL_SOURCE_STATE, &state);
	CheckError("SoundSource::IsPlaying");
	if (state == AL_PLAYING)
		return true;
	else
	{
		return false;
	}
}

void SoundSource::Stop()
{
	alSourceStop(id);
	if (curPlaying)
	{
		curPlaying->StopPlay();
		curPlaying = 0;
	}
	CheckError("SoundSource::Stop");
}

void SoundSource::Play(SoundItem* item, const float3& pos, const float3& velocity, float volume)
{
	if (!item->PlayNow())
		return;
	Stop();
	curPlaying = item;
	alSourcei(id, AL_BUFFER, item->buffer->GetId());
	alSourcef(id, AL_GAIN, item->gain * volume);
	alSourcef(id, AL_PITCH, item->pitch * globalPitch);
	alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
	alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	alSourcei(id, AL_LOOPING, (item->loopTime > 0));
	loopStop = SDL_GetTicks() + item->loopTime;
	alSourcei(id, AL_SOURCE_RELATIVE, item->in3D);
	alSourcePlay(id);
	if (item->buffer->GetId() == 0)
		logOutput.Print("SoundSource::Play: Empty buffer for item %s (file %s)", item->name.c_str(), item->buffer->GetFilename().c_str());
	CheckError("SoundSource::Play");
}

void SoundSource::SetPitch(float newPitch)
{
	globalPitch = newPitch;
}