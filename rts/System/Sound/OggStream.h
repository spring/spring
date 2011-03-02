/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <string>
#include <vector>

class COggStream
{
public:
	typedef std::vector<std::string> TagVector;
	COggStream(ALuint _source);
	~COggStream();

	void Play(const std::string& path, float volume);
	void Stop();
	bool TogglePause();
	void Update();

	float GetPlayTime() const;
	float GetTotalTime();
	const TagVector& VorbisTags() const;
	bool Valid() const;

private:
	void DisplayInfo();
	bool IsPlaying();
	bool StartPlaying();

	bool DecodeStream(ALuint buffer);
	void EmptyBuffers();
	void ReleaseBuffers();

	/**
	@brief Decode next part of the stream and queue it for playing
	@return wheter it is the end of the stream (check for IsPlaying() wheter the complete stream was played)
	*/
	bool UpdateBuffers();

	OggVorbis_File oggStream;
	vorbis_info* vorbisInfo;

	static const unsigned int BUFFER_SIZE = (4096 * 128); // 512KB
	static const unsigned int NUM_BUFFERS = 2;

	ALuint buffers[NUM_BUFFERS];
	ALuint source;
	ALenum format;

	bool stopped;
	bool paused;

	unsigned msecsPlayed;
	unsigned lastTick;
	
	std::vector<std::string> vorbisTags;
	std::string vendor;
};


#endif
