#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>

#include "System/float3.h" // for zerovector

class CWorldObject;
class CUnit;
class float3;

// Sound system interface
class CSound
{
public:
	CSound() { unitReplyVolume = 1.0f; }
	virtual ~CSound();

	static CSound* GetSoundSystem();

	virtual unsigned int GetWaveId(const std::string& path, bool hardFail = true) = 0;
	virtual void Update() = 0;
	virtual void PlaySample(int id, float volume = 1.0f) = 0;
	virtual void PlaySample(int id, const float3& p, float volume = 1.0f) = 0;

	virtual void PlayStream(const std::string& path, float volume = 1.0f,
							const float3& pos = ZeroVector, bool loop = false) = 0;
	virtual void StopStream() = 0;
	virtual void PauseStream() = 0;
	virtual unsigned int GetStreamTime() = 0;
	virtual void SetStreamVolume(float) = 0;

	virtual void SetVolume(float vol) = 0; // 1 = full volume

	void PlaySample(int id, CWorldObject* p, float volume = 1.0f);
	void PlayUnitReply(int id, CUnit* p, float volume = 1.0f, bool squashDupes = false);
	void PlayUnitActivate(int id, CUnit* p, float volume = 1.0f);
	void NewFrame();

	void SetUnitReplyVolume(float vol); // also affected by global volume (SetVolume())

private:
	std::set<unsigned int> repliesPlayed;
	float unitReplyVolume;
};

extern CSound* sound;

#endif
