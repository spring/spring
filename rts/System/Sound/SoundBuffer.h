#ifndef SOUNDBUFFER_H
#define SOUNDBUFFER_H

#include <AL/al.h>
#include <boost/noncopyable.hpp>
#include <string>
#include <vector>

class SoundBuffer : boost::noncopyable
{
public:
	SoundBuffer();
	~SoundBuffer();

	bool LoadWAV(const std::string& file, std::vector<uint8_t> buffer, bool strict);

	const std::string& GetFilename() const
	{
		return filename;
	};

	ALuint GetId() const
	{
		return id;
	};

private:
	std::string filename;
	ALuint id;
};


#endif
