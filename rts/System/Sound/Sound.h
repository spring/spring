/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SOUND_H_
#define _SOUND_H_

#include "ISound.h"

#include <set>
#include <string>
#include <map>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "float3.h"

class SoundSource;
class SoundBuffer;
class SoundItem;


/// Default sound system implementation (OpenAL)
class CSound : public ISound
{
public:
	CSound();
	virtual ~CSound();

	virtual bool HasSoundItem(const std::string& name);
	virtual size_t GetSoundId(const std::string& name, bool hardFail = true);

	virtual SoundSource* GetNextBestSource(bool lock = true);

	virtual void UpdateListener(const float3& campos, const float3& camdir, const float3& camup, float lastFrameTime);
	virtual void NewFrame();

	/// @see ConfigHandler::ConfigNotifyCallback
	virtual void ConfigNotify(const std::string& key, const std::string& value);
	virtual void PitchAdjust(const float newPitch);

	virtual bool Mute();
	virtual bool IsMuted() const;

	virtual void Iconified(bool state);

	virtual void PrintDebugInfo();
	virtual bool LoadSoundDefs(const std::string& fileName);

private:
	void StartThread(int maxSounds);
	void Update();

	typedef std::map<std::string, std::string> soundItemDef;
	typedef std::map<std::string, soundItemDef> soundItemDefMap;

	size_t MakeItemFromDef(const soundItemDef& itemDef);

	friend class EffectChannel;
	// this is used by EffectChannel in AudioChannel.cpp
	virtual void PlaySample(size_t id, const float3 &p, const float3& velocity, float volume, bool relative);

	size_t LoadSoundBuffer(const std::string& filename, bool hardFail);

	float masterVolume;
	bool mute;
	/// we do not play if minimized / iconified
	bool appIsIconified;
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
	boost::recursive_mutex soundMutex;

	volatile bool soundThreadQuit;
};

#endif // _SOUND_H_
