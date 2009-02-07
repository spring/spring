#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>
#include <map>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

#include "float3.h"

class CWorldObject;
class CUnit;
class SoundSource;
class SoundBuffer;
class SoundItem;

// Sound system interface
class CSound
{
public:
	CSound();
	~CSound();

	size_t GetSoundId(const std::string& name, bool hardFail = true);

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
	size_t LoadALBuffer(const std::string& path, bool strict);
	void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	size_t GetWaveId(const std::string& path, bool hardFail);
	boost::shared_ptr<SoundBuffer> GetWaveBuffer(const std::string& path, bool hardFail = true);

	void UpdateListener();

	size_t cur;
	float globalVolume;

	typedef std::map<std::string, size_t> bufferMapT;
	typedef std::vector< boost::shared_ptr<SoundBuffer> > bufferVecT;
	bufferMapT bufferMap; // filename, index into Buffers
	bufferVecT buffers;

	typedef std::map<std::string, size_t> soundMapT;
	typedef boost::ptr_vector<SoundItem> soundVecT;
	soundMapT soundMap;
	soundVecT sounds;

	float3 posScale;
	float3 prevPos;
	float3 myPos;

	typedef boost::ptr_vector<SoundSource> sourceVecT;
	sourceVecT sources;
	std::set<unsigned int> repliesPlayed;
	float unitReplyVolume;
	bool mute;

	typedef std::map<std::string, std::string> soundItemDef;
	typedef std::map< std::string, soundItemDef > soundItemMap;
	soundItemMap soundItemDefs;
	soundItemDef defaultItem;
};

extern CSound* sound;

#endif
