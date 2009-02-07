#include "SoundSource.h"

#include <AL/alc.h>

#include "Sound.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "float3.h"
#include "LogOutput.h"

float SoundSource::globalPitch = 0.0;
namespace
{
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
}

SoundSource::SoundSource()
{
	alGenSources(1, &id);
	CheckError("SoundSource::SoundSource");
}

SoundSource::~SoundSource()
{
	alDeleteSources(1, &id);
	CheckError("SoundSource::~SoundSource");
}

bool SoundSource::IsPlaying() const
{
	ALint state;
	alGetSourcei(id, AL_SOURCE_STATE, &state);
	CheckError("SoundSource::IsPlaying");
	if (state == AL_PLAYING)
		return true;
	else
		return false;
}

void SoundSource::Stop()
{
	alSourceStop(id);
	CheckError("SoundSource::Stop");
}

void SoundSource::Play(const SoundItem& item, const float3& pos, const float3& velocity, float volume)
{
	if (IsPlaying())
		Stop();
	alSourcei(id, AL_BUFFER, item.buffer->GetId());
	alSourcef(id, AL_GAIN, item.gain * volume);
	SetPitch(item.pitch * globalPitch);
	alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
	alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	alSourcei(id, AL_LOOPING, false);
	alSourcei(id, AL_SOURCE_RELATIVE, item.in3D);
	alSourcePlay(id);
	CheckError("SoundSource::Play");
}

void SoundSource::SetPitch(float newPitch)
{
	globalPitch = newPitch;
}