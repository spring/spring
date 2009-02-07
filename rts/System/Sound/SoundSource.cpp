#include "SoundSource.h"

#include <AL/alc.h>

#include "SoundBuffer.h"
#include "float3.h"
#include "LogOutput.h"

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

void SoundSource::Play(const SoundBuffer& buffer, const float3& pos, const float3& velocity, float volume, bool relative)
{
	if (IsPlaying())
		Stop();
	alSourcei(id, AL_BUFFER, buffer.GetId());
	alSourcef(id, AL_GAIN, volume);

	alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
	alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	alSourcei(id, AL_LOOPING, false);
	alSourcei(id, AL_SOURCE_RELATIVE, relative);
	alSourcePlay(id);
	CheckError("SoundSource::Play");
}

void SoundSource::SetPitch(float newPitch) const
{
	alSourcef(id, AL_PITCH, newPitch);
	CheckError("SoundSource::SetPitch");
}