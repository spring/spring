/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <al.h>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <cinttypes>

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

	bool LoadWAV(const std::string& file, std::vector<std::uint8_t> buffer);
	bool LoadVorbis(const std::string& file, std::vector<std::uint8_t> buffer);

	const std::string& GetFilename() const
	{
		return filename;
	};

	ALuint GetId() const
	{
		return id;
	};

	ALuint GetChannels() const
	{
		return channels;
	};

	ALfloat GetLength() const
	{
		return length;
	};

	int BufferSize() const;

	static void Initialise();
	static void Deinitialise();

	static size_t GetId(const std::string& name);
	static std::shared_ptr<SoundBuffer> GetById(const size_t id);

	static size_t Count();
	static size_t AllocedSize();

	static size_t Insert(std::shared_ptr<SoundBuffer> buffer);
private:
	bool AlGenBuffer(const std::string& file, ALenum format, const std::uint8_t* data, size_t datalength, int rate);

	std::string filename;
	ALuint id;
	ALuint channels;
	ALfloat length;

	typedef std::map<std::string, size_t> bufferMapT;
	typedef std::vector< std::shared_ptr<SoundBuffer> > bufferVecT;
	static bufferMapT bufferMap; // filename, index into Buffers
	static bufferVecT buffers;
};


#endif
