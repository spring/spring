#ifndef SOUNDITEM_H
#define SOUNDITEM_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class SoundBuffer;

/**
 * @brief A class representing a soudn which can be played
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

private:
	boost::shared_ptr<SoundBuffer> buffer;
	/// unique identifier (if no name is specified, this will be the filename
	std::string name;

	/// volume gain, applied to this sound
	float gain;
	//float gainFWHM;
	/// sound pitch (multiplied with globalPitch fro mSoudnSource when played)
	float pitch;
	//float pitchFWHM;
	int priority;
	unsigned maxConcurrent;
	bool in3D;
};

#endif
