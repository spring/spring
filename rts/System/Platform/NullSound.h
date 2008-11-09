#ifndef NULL_SOUND_H
#define NULL_SOUND_H

#include "Sound.h"

// Null sound system
class CNullSound: public CSound
{
public:
	CNullSound() { return; }
	~CNullSound() { return; }

	unsigned int GetWaveId(const std::string& path, bool hardFail) { return 0; }
	void Update() { return; }
	void PlaySample(int id, float volume = 1.0f) { return; }
	void PlaySample(int id, const float3& p, float volume = 1.0f) { return; }

	void PlayStream(const std::string& path, float volume,
					const float3& pos, bool loop)  { return; }
	void StopStream() { return; }
	void PauseStream() { return; }
	unsigned int GetStreamTime() { return 0; }
	unsigned int GetStreamPlayTime() { return 0; }
	void SetStreamVolume(float) { return; }

	void SetVolume(float vol) { return; }
};

#endif // NULL_SOUND_H
