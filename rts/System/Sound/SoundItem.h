#ifndef SOUNDITEM_H
#define SOUNDITEM_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class SoundBuffer;

/**
 * @brief A class representing a sound which can be played
 * 
 * This can be played by SoundSource.
 * Each soundsource has exactly one SoundBuffer it wraps around, while one buffer can be shared among multiple Items.
 * You can adjust various playing parameters within this class, sou you can have 1 buffer and multiple SoundItems
 * which differ in pitch, volume etc.
 */
class SoundItem
{
	friend class SoundSource;
public:
	SoundItem(boost::shared_ptr<SoundBuffer> buffer, const std::map<std::string, std::string>& items);

	bool PlayNow();
	void StopPlay();
	float MaxDistance() const
	{
		return maxDist;
	};
	const std::string& Name() const
	{
		return name;
	};
	const int GetPriority() const
	{
		return priority;
	};

private:
	boost::shared_ptr<SoundBuffer> buffer;
	/// unique identifier (if no name is specified, this will be the filename
	std::string name;

	/// volume gain, applied to this sound
	float gain;
	//float gainFWHM;
	/// sound pitch (multiplied with globalPitch from SoundSource when played)
	float pitch;
	//float pitchFWHM;
	float dopplerScale;

	float maxDist;
	float rolloff;
	int priority;

	unsigned maxConcurrent;
	unsigned currentlyPlaying;

	unsigned loopTime;

	bool in3D;
};

#endif
