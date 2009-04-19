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

class AudioChannel
{
public:
	AudioChannel();
	
	void Enable(bool newState);
	bool IsEnabled();
	
	void SetVolume(float newVolume)
	{
		volume = newVolume;
	};
	float GetVolume()
	{
		return volume;
	};

	void PlaySample(size_t id, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, float volume = 1.0f);
	void PlaySample(size_t id, const float3& p, const float3& velocity, float volume = 1.0f);

	void PlaySample(size_t id, CUnit* u, float volume = 1.0f);
	void PlaySample(size_t id, CWorldObject* p, float volume = 1.0f);

private:
	float volume;
	
	bool enabled;
};

namespace Channels
{
extern AudioChannel General;
extern AudioChannel Battle;
extern AudioChannel UnitReply;
extern AudioChannel UserInterface;
}

// Sound system interface
class CSound
{
	friend class AudioChannel;
public:
	CSound();
	~CSound();

	bool HasSoundItem(const std::string& name);
	size_t GetSoundId(const std::string& name, bool hardFail = true);

	void Update();
	void UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime);
	void NewFrame();

	void PitchAdjust(const float newPitch);

	bool Mute();
	bool IsMuted() const;

	void SetVolume(float vol); // 1 = full volume

	void PrintDebugInfo();

private:
	void LoadSoundDefs(const std::string& filename);

	size_t LoadALBuffer(const std::string& path, bool strict);
	void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	size_t GetWaveId(const std::string& path, bool hardFail);
	boost::shared_ptr<SoundBuffer> GetWaveBuffer(const std::string& path, bool hardFail = true);

	float masterVolume;
	bool mute;

	typedef std::map<std::string, size_t> soundMapT;
	typedef boost::ptr_vector<SoundItem> soundVecT;
	soundMapT soundMap;
	soundVecT sounds;

	/// some scale, so camera above will still hear sounds from far below
	float3 posScale;
	/// unscaled
	float3 myPos;

	typedef boost::ptr_vector<SoundSource> sourceVecT;
	sourceVecT sources;
	
	unsigned numEmptyPlayRequests;
	unsigned updateCounter;

	typedef std::map<std::string, std::string> soundItemDef;
	typedef std::map<std::string, soundItemDef> soundItemDefMap;
	soundItemDef defaultItem;
	soundItemDefMap soundItemDefs;
};

extern CSound* sound;

#endif
