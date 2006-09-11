#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include "float3.h"
#include <set>

class CWorldObject;
class CUnit;

// Sound system interface
class CSound
{
public:
	CSound() { unitReplyVolume=1.0f; }
	virtual ~CSound();
	
	static CSound* GetSoundSystem();

	virtual unsigned int GetWaveId(const std::string& path) = 0;
	virtual void Update() = 0;
	virtual void PlaySample(int id, float volume=1) = 0;
	virtual void PlaySample(int id,const float3& p,float volume=1) = 0;
	virtual void SetVolume (float vol) = 0; // 1 = full volume
	
	void PlaySample(int id,CWorldObject* p,float volume=1.0f);
	void PlayUnitReply(int id, CUnit* p, float volume=1.0f, bool squashDupes=false);
	void PlayUnitActivate(int id, CUnit* p, float volume=1.0f);
	void NewFrame();

	void SetUnitReplyVolume (float vol); // also affected by global volume ( SetVolume() )
private:
	std::set<unsigned int> repliesPlayed;
	float unitReplyVolume;
};

extern CSound* sound; 

#endif
