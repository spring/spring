#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>
#include <map>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>

#include "float3.h"

class SoundSource;
class SoundBuffer;
class SoundItem;


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

	void ConfigNotify(const std::string& key, const std::string& value);
	void PitchAdjust(const float newPitch);

	bool Mute();
	bool IsMuted() const;

	void Iconified(bool state);

	void PrintDebugInfo();
	bool LoadSoundDefs(const std::string& filename);

private:

	size_t LoadALBuffer(const std::string& path, bool strict);
	void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	size_t GetWaveId(const std::string& path, bool hardFail);
	boost::shared_ptr<SoundBuffer> GetWaveBuffer(const std::string& path, bool hardFail = true);

	float masterVolume;
	bool mute;
	bool appIsIconified; // do not play when minimized / iconified
	bool pitchAdjust;

	typedef std::map<std::string, size_t> soundMapT;
	typedef boost::ptr_vector<SoundItem> soundVecT;
	soundMapT soundMap;
	soundVecT sounds;

	/// unscaled
	float3 myPos;
	float3 prevVelocity;

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
