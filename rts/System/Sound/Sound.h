#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>
#include <map>
#include <boost/ptr_container/ptr_vector.hpp>

#include "float3.h"

class CWorldObject;
class CUnit;
class SoundSource;
class SoundBuffer;

// Sound system interface
class CSound
{
public:
	CSound();
	~CSound();

	size_t GetWaveId(const std::string& path, bool hardFail = true);

	void Update();
	void NewFrame();

	void PlaySample(size_t id, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f);

	void PlaySample(size_t id, CUnit* u, float volume = 1.0f);
	void PlaySample(size_t id, CWorldObject* p, float volume = 1.0f);
	void PlayUnitReply(size_t id, CUnit* p, float volume = 1.0f, bool squashDupes = false);
	void PlayUnitActivate(size_t id, CUnit* p, float volume = 1.0f);

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
	size_t LoadALBuffer(const std::string& path);
	void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	void UpdateListener();
	
	size_t cur;
	float globalVolume;
	bool hardFail;
	
	typedef std::map<std::string, size_t> bufferMap;
	typedef boost::ptr_vector<SoundBuffer> bufferVec;
	bufferMap soundMap; // filename, index into Buffers
	bufferVec buffers;

	float3 posScale;
	float3 prevPos;
	typedef boost::ptr_vector<SoundSource> sourceVec;
	sourceVec sources;
	std::set<unsigned int> repliesPlayed;
	float unitReplyVolume;
	bool mute;
};

extern CSound* sound;

#endif
