#ifndef SOUNDSOURCE_H
#define SOUNDSOURCE_H

#include <AL/al.h>
#include <boost/noncopyable.hpp>

class float3;

class SoundSource : boost::noncopyable
{
public:
	SoundSource();
	~SoundSource();

	bool IsPlaying() const;
	void Stop();
	void Play(ALuint buffer, const float3& pos, const float3& velocity, float volume, bool relative);
	void SetPitch(float newPitch) const;

private:
	ALuint id;
};

#endif