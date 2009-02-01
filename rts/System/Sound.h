#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>
#include <map>
#include <AL/al.h>

#include "float3.h" // for zerovector

class CWorldObject;
class CUnit;
class float3;

// Sound system interface
class CSound
{
public:
	CSound();
	~CSound();

	ALuint GetWaveId(const std::string& path, bool hardFail = true);

	void Update();
	void NewFrame();

	void PlaySample(int id, float volume = 1.0f);
	void PlaySample(int id, const float3& p, float volume = 1.0f);
	void PlaySample(int id, const float3& p, const float3& velocity, float volume = 1.0f);

	void PlaySample(int id, CUnit* u, float volume = 1.0f);
	void PlaySample(int id, CWorldObject* p, float volume = 1.0f);
	void PlayUnitReply(int id, CUnit* p, float volume = 1.0f, bool squashDupes = false);
	void PlayUnitActivate(int id, CUnit* p, float volume = 1.0f);

	void PlayStream(const std::string& path, float volume = 1.0f, const float3& pos = ZeroVector, bool loop = false);
	void StopStream();
	void PauseStream();
	unsigned int GetStreamTime();
	unsigned int GetStreamPlayTime();
	void SetStreamVolume(float);

	void PitchAdjust(const float newPitch);

	bool Mute();
	bool IsMuted() const;

	void SetVolume(float vol); // 1 = full volume
	void SetUnitReplyVolume(float vol); // also affected by global volume (SetVolume())

private:
	bool ReadWAV(const char* name, Uint8* buf, int size, ALuint albuffer);
	ALuint LoadALBuffer(const std::string& path);
	void PlaySample(int id, const float3 &p, const float3& velocity, float volume, bool relative);

	void UpdateListener();
	
	size_t cur;
	float globalVolume;
	bool hardFail;
	
	std::map<std::string, ALuint> soundMap; // filename, index into Buffers
	float3 posScale;
	float3 prevPos;
	typedef std::vector<ALuint> sourceVec;
	std::vector<ALuint> sources;
	std::set<unsigned int> repliesPlayed;
	float unitReplyVolume;
	bool mute;
};

extern CSound* sound;

#endif
