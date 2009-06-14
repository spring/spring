#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <al.h>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <vector>
#include <boost/cstdint.hpp>

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

	bool LoadWAV(const std::string& file, std::vector<boost::uint8_t> buffer, bool strict);
	bool LoadVorbis(const std::string& file, std::vector<boost::uint8_t> buffer, bool strict);

	const std::string& GetFilename() const
	{
		return filename;
	};

	ALuint GetId() const
	{
		return id;
	};
	
	int BufferSize() const;
	
	static void Initialise();
	static void Deinitialise();
	
	static size_t GetId(const std::string& name);
	static boost::shared_ptr<SoundBuffer> GetById(const size_t id);
	
	static size_t Count();
	static size_t AllocedSize();
	
	static size_t Insert(boost::shared_ptr<SoundBuffer> buffer);
private:
	bool AlGenBuffer(const std::string& file, ALenum format, const boost::uint8_t* data, size_t datalength, int rate);

	std::string filename;
	ALuint id;
	
	typedef std::map<std::string, size_t> bufferMapT;
	typedef std::vector< boost::shared_ptr<SoundBuffer> > bufferVecT;
	static bufferMapT bufferMap; // filename, index into Buffers
	static bufferVecT buffers;
};


#endif
