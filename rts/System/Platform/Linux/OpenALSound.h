#ifndef OPENAL_SOUND_H
#define OPENAL_SOUND_H

// CSound interface definition
#include "Sound.h"

#include <map>
#include <AL/al.h>
#include "SDL_types.h"

class COpenALSound: public CSound
{
public:
	ALuint GetWaveId(const std::string& path, bool hardFail);
	void Update();
	void PlaySample(int id, float volume);
	void PlaySample(int id, const float3& p,float volume);
	void PlaySample(int id, const float3& p, const float3& velocity, float volume);

	void PlayStream(const std::string& path, float volume = 1.0f,
					const float3& pos = ZeroVector, bool loop = false);
	void StopStream();
	void PauseStream();
	unsigned int GetStreamTime();
	unsigned int GetStreamPlayTime();
	void SetStreamVolume(float);

	void SetVolume(float v);

	COpenALSound();
	virtual ~COpenALSound();

private:
	bool ReadWAV(const char* name, Uint8* buf, int size, ALuint albuffer);

private:
	ALuint LoadALBuffer(const std::string& path);
	void PlaySample(int id, const float3 &p, const float3& velocity, float volume, bool relative);

	int maxSounds;
	int cur;
	float globalVolume;
	bool hardFail;

	void UpdateListener();
	void Enqueue(ALuint src);
	
	std::map<std::string, ALuint> soundMap; // filename, index into Buffers
	float3 posScale;
	float3 prevPos;
	ALuint* Sources;
};


#endif /* SOUND_H */
