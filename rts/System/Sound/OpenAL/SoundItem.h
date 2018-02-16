/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDITEM_H
#define SOUNDITEM_H

#include <string>
#include <memory>

#include "System/UnorderedMap.hpp"

class SoundBuffer;

/**
 * @brief A class representing a sound which can be played
 *
 * This can be played by CSoundSource.
 * Each soundsource has exactly one SoundBuffer it wraps around, while one buffer can be shared among multiple Items.
 * You can adjust various playing parameters within this class, sou you can have 1 buffer and multiple SoundItems
 * which differ in pitch, volume etc.
 */
class SoundItem
{
	friend class CSoundSource;
public:
	SoundItem(size_t bufferID, const spring::unordered_map<std::string, std::string>& items);

	bool PlayNow();
	void StopPlay();

	size_t GetSoundBufferID() const { return soundBufferID; }

	float MaxDistance() const { return maxDist; }
	const std::string& Name() const { return name; }
	const int GetPriority() const { return priority; }

	float GetGain() const;
	float GetPitch() const;

private:
	size_t soundBufferID = 0;

	/// unique identifier (if no name is specified, this will be the filename)
	std::string name;

	/// volume gain, applied to this sound
	float gain = 1.0f;
	float gainMod = 0.0f;
	/// sound pitch (multiplied with globalPitch from CSoundSource when played)
	float pitch = 1.0f;
	float pitchMod = 0.0f;
	float dopplerScale = 1.0f;

	float maxDist = 0.0f;
	float rolloff = 1.0f;

	int priority = 0;

	unsigned maxConcurrent = 16;
	unsigned currentlyPlaying = 0;
	unsigned loopTime = 0;

	bool in3D = true;
};

#endif
