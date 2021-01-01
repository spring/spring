/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <al.h>

#include <string>
#include <vector>
#include <cinttypes>

#include "System/UnorderedMap.hpp"
#include "System/Misc/NonCopyable.h"

/**
 * @brief A buffer holding a sound
 *
 * One of these will be created for each {wav,ogg} sound-file loaded.
 * All buffers are generated on demand and released when a game ends,
 * and can be shared among multiple SoundItem instances.
 */
class SoundBuffer : spring::noncopyable
{
public:
	/// Construct an "empty" buffer
	/// can be played, but you won't hear anything
	SoundBuffer() = default;
	SoundBuffer(SoundBuffer&& sb) { *this = std::move(sb); }
	~SoundBuffer() { Release(); }

	SoundBuffer& operator = (SoundBuffer&& sb) {
		filename = std::move(sb.filename);
		id = sb.id;
		sb.id = 0;
		channels = sb.channels;
		length = sb.length;
		return *this;
	}

	bool LoadWAV(const std::string& file, const std::vector<std::uint8_t>& buffer);
	bool LoadVorbis(const std::string& file, const std::vector<std::uint8_t>& buffer);
	bool Release();

	const std::string& GetFilename() const { return filename; }

	ALuint GetId() const { return id; }
	ALuint GetChannels() const { return channels; }
	ALfloat GetLength() const { return length; }

	int BufferSize() const;

	static void Initialise() {
		buffers.reserve(256);
		buffers.emplace_back(); // empty ("zero") buffer
	}
	static void Deinitialise() {
		bufferMap.clear();
		buffers.clear();
	}

	static size_t GetId(const std::string& name);
	static SoundBuffer& GetById(const size_t id);

	static size_t Count() { return buffers.size(); }
	static size_t AllocedSize();

	static size_t Insert(SoundBuffer&& buffer);

private:
	bool AlGenBuffer(const std::string& file, ALenum format, const std::uint8_t* data, size_t datalength, int rate);

	std::string filename;

	ALuint id = 0;
	ALuint channels = 0;
	ALfloat length = 0.0f;

	typedef spring::unsynced_map<std::string, size_t> bufferMapT;
	typedef std::vector<SoundBuffer> bufferVecT;

	static bufferMapT bufferMap; // filename, index into ::buffers
	static bufferVecT buffers;
};

#endif

