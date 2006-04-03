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
	CSound() {}
	virtual ~CSound() {}

	virtual unsigned int GetWaveId(const std::string& path) = 0;
	virtual void Update() = 0;
	virtual void PlaySound(int id, float volume=1) = 0;
	virtual void PlaySound(int id,CWorldObject* p,float volume=1) = 0;
	virtual void PlaySound(int id,const float3& p,float volume=1) = 0;
	virtual void SetVolume (float vol) = 0; // 1 = full volume
	
	void PlayUnitReply(int id, CUnit* p, float volume=1, bool squashDupes=false);
	void PlayUnitActivate(int id, CUnit* p, float volume=1);
	void NewFrame();
private:
	std::set<unsigned int> repliesPlayed;
};

extern CSound* sound; 

#endif
