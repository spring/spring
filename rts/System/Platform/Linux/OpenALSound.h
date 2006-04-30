#ifndef OPENAL_SOUND_H
#define OPENAL_SOUND_H

// CSound interface definition
#include "Sound.h"

#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

using namespace std;
class COpenALSound : public CSound
{
public:
	ALuint GetWaveId(const string& path);
	void Update();
	void PlaySound(int id, float volume);
	void PlaySound(int id,const float3& p,float volume);
	void SetVolume(float v);

	COpenALSound();
	virtual ~COpenALSound();
private:
	ALuint LoadALBuffer(const string& path);
	void PlaySound(int id, const float3 &p, float volume, bool relative);

	bool noSound;
	int maxSounds;
	int cur;
	float globalVolume;

	void UpdateListener();
	void Enqueue(ALuint src);
	vector<string> LoadedFiles;
	vector<ALuint> Buffers;
	float3 posScale;
	ALuint *Sources;
};


#endif /* SOUND_H */
