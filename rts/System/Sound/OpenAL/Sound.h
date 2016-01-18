/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SOUND_H_
#define _SOUND_H_

#include "System/Sound/ISound.h"

#include <set>
#include <string>
#include <map>
#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "System/float3.h"

#include "SoundItem.h"

class CSoundSource;
class SoundBuffer;
class SoundItem;
struct ALCdevice_struct;
typedef struct ALCdevice_struct ALCdevice;

namespace boost {
	class thread;
}


/// Default sound system implementation (OpenAL)
class CSound : public ISound
{
public:
	CSound();
	virtual ~CSound();

	virtual bool HasSoundItem(const std::string& name) const;
	virtual size_t GetSoundId(const std::string& name);
	SoundItem* GetSoundItem(size_t id) const;

	virtual CSoundSource* GetNextBestSource(bool lock = true);

	virtual void UpdateListener(const float3& campos, const float3& camdir, const float3& camup);
	virtual void NewFrame();

	/// @see ConfigHandler::ConfigNotifyCallback
	virtual void ConfigNotify(const std::string& key, const std::string& value);
	virtual void PitchAdjust(const float newPitch);

	virtual bool Mute();
	virtual bool IsMuted() const;

	virtual void Iconified(bool state);

	virtual void PrintDebugInfo();

	bool SoundThreadQuit() const { return soundThreadQuit; }
	bool CanLoadSoundDefs() const { return canLoadDefs; }

	bool LoadSoundDefsImpl(const std::string& fileName, const std::string& modes);
	const float3& GetListenerPos() const { return myPos; }

private:
	typedef std::map<std::string, std::string> soundItemDef;
	typedef std::map<std::string, soundItemDef> soundItemDefMap;

private:
	void StartThread(int maxSounds);
	void Update();
	int GetMaxMonoSources(ALCdevice* device, int maxSounds);
	void UpdateListenerReal();

	size_t MakeItemFromDef(const soundItemDef& itemDef);

	size_t LoadSoundBuffer(const std::string& filename);

private:
	float masterVolume;

	bool mute;

	/// we do not play if minimized / iconified
	bool appIsIconified;
	bool pitchAdjust;

	typedef std::map<std::string, size_t> soundMapT;
	typedef std::vector<SoundItem*> soundVecT;
	soundMapT soundMap;
	soundVecT sounds;

	/// unscaled
	float3 myPos;
	float3 camDir;
	float3 camUp;
	float3 prevVelocity;
	bool listenerNeedsUpdate;

	typedef boost::ptr_vector<CSoundSource> sourceVecT;
	sourceVecT sources;

	soundItemDef defaultItem;
	soundItemDefMap soundItemDefs;

	boost::thread* soundThread;

	volatile bool soundThreadQuit;
	volatile bool canLoadDefs;
};

#endif // _SOUND_H_
