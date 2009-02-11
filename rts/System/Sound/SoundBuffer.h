#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <AL/al.h>
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>

/**
 * @brief A buffer holding a sound
 * 
 * One of this will be created for each wav-file used.
 * They are loaded on demand and unloaded when game ends.
 * They can be shared among multiple SoundItem
 */
class SoundBuffer : boost::noncopyable
{
public:
	/// Construct an "empty" buffer
	/// can be played, but you won't hear anything
	SoundBuffer();
	~SoundBuffer();

	bool LoadWAV(const std::string& file, std::vector<uint8_t> buffer, bool strict);
	bool LoadVorbis(const std::string& file, std::vector<uint8_t> buffer, bool strict);

	const std::string& GetFilename() const
	{
		return filename;
	};

	ALuint GetId() const
	{
		return id;
	};
	
	int BufferSize() const;

private:
	void AlGenBuffer(const std::string& file, ALenum format, const uint8_t* data, size_t datalength, int rate);

	std::string filename;
	ALuint id;
};


#endif
