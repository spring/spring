#include "SoundSource.h"

#include <climits>
#include <AL/alc.h>
#include <SDL_timer.h>

#include "SoundBuffer.h"
#include "SoundItem.h"
#include "ALShared.h"
#include "float3.h"
#include "LogOutput.h"

float SoundSource::globalPitch = 1.0;
float SoundSource::heightAdjustedRolloffModifier = 1.0;

SoundSource::SoundSource() : curPlaying(0)
{
	alGenSources(1, &id);
	if (!CheckError("SoundSource::SoundSource"))
	{
		id = 0;
	}
	else
	{
		alSourcef(id, AL_REFERENCE_DISTANCE, 200.0f);
		CheckError("SoundSource::SoundSource");
	}
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
	if (!curPlaying) {
		logOutput.Print("Warning: SoundSource::GetCurrentPriority() curPlaying is NULL (id %d)", id);
		return INT_MIN;
	}
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

void SoundSource::Play(SoundItem* item, const float3& pos, float3 velocity, float volume, bool relative)
{
	if (!item->PlayNow())
		return;
	Stop();
	curPlaying = item;
	alSourcei(id, AL_BUFFER, item->buffer->GetId());
	alSourcef(id, AL_GAIN, item->gain * volume);
	alSourcef(id, AL_PITCH, item->pitch * globalPitch);
	velocity *= item->dopplerScale;
	alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	if (item->loopTime > 0)
		alSourcei(id, AL_LOOPING, AL_TRUE);
	else
		alSourcei(id, AL_LOOPING, AL_FALSE);
	loopStop = SDL_GetTicks() + item->loopTime;
	if (relative || !item->in3D)
	{
		alSourcei(id, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(id, AL_POSITION, 0.0, 0.0, -1.0);
	}
	else
	{
		alSourcei(id, AL_SOURCE_RELATIVE, AL_FALSE);
		alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
		alSourcef(id, AL_ROLLOFF_FACTOR, item->rolloff * heightAdjustedRolloffModifier);
	}
	alSourcePlay(id);

	if (item->buffer->GetId() == 0)
		logOutput.Print("SoundSource::Play: Empty buffer for item %s (file %s)", item->name.c_str(), item->buffer->GetFilename().c_str());
	CheckError("SoundSource::Play");
}

void SoundSource::SetPitch(float newPitch)
{
	globalPitch = newPitch;
}
