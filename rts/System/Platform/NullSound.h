#ifndef NULL_SOUND_H
#define NULL_SOUND_H

#include "../Sound.h"

// Null sound system
class CNullSound : public CSound
{
public:
	CNullSound() { return; }
	~CNullSound() { return; } 

	unsigned int GetWaveId(const std::string& path) { return 0; }
	void Update() { return; }
	void PlaySample(int id, float volume=1) { return; }
	void PlaySample(int id,const float3& p,float volume=1) { return; }
	void SetVolume (float vol) { return; }
};

#endif // NULL_SOUND_H
