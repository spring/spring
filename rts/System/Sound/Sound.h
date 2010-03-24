/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

#include <set>
#include <string>
#include <map>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "float3.h"

class SoundSource;
class SoundBuffer;
class SoundItem;


// Sound system interface
class CSound
{
	friend class EffectChannel;
public:
	CSound();
	~CSound();

	bool HasSoundItem(const std::string& name);
	size_t GetSoundId(const std::string& name, bool hardFail = true);

	/// returns a free soundsource if available, the one with the lowest priority otherwise
	SoundSource* GetNextBestSource(bool lock = true);

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
	void StartThread(int maxSounds);
	void Update();

	typedef std::map<std::string, std::string> soundItemDef;
	typedef std::map<std::string, soundItemDef> soundItemDefMap;

	size_t MakeItemFromDef(const soundItemDef& itemDef);

	void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	size_t LoadSoundBuffer(const std::string& filename, bool hardFail);

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
	unsigned numAbortedPlays;

	soundItemDef defaultItem;
	soundItemDefMap soundItemDefs;

	boost::thread* soundThread;
	boost::mutex soundMutex;
};

extern CSound* sound;

#endif // SOUNDSYSTEM_INTERFACE_H
