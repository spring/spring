#ifndef OPENAL_SOUND_H
#define OPENAL_SOUND_H

// CSound interface definition
#include "Sound.h"

#include <map>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include "SDL_types.h"

#include "OggStream.h"


using namespace std;

class COpenALSound : public CSound
{
public:
	ALuint GetWaveId(const string& path, bool hardFail);
	void Update();
	void PlaySample(int id, float volume);
	void PlaySample(int id,const float3& p,float volume);
	void PlayStream(const std::string& path, float volume = 1.0f,
					const float3& pos = ZeroVector, bool loop = false);
	void StopStream();
	void SetVolume(float v);

	COpenALSound();
	virtual ~COpenALSound();

private:
	bool ReadWAV(const char *name, Uint8 *buf, int size, ALuint albuffer);

private:
	ALuint LoadALBuffer(const string& path);
	void PlaySample(int id, const float3 &p, float volume, bool relative);

	int maxSounds;
	int cur;
	float globalVolume;
	bool hardFail;

	void UpdateListener();
	void Enqueue(ALuint src);
	
	map<string, ALuint> soundMap; // filename, index into Buffers
	float3 posScale;
	ALuint* Sources;
};


#endif /* SOUND_H */
