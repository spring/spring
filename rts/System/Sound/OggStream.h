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
	COggStream(ALuint source);
	~COggStream();

	void Play(const std::string& path, float volume);
	void Stop();
	bool TogglePause();
	void Update();
	float GetPlayTime();
	float GetTotalTime();
	const TagVector& VorbisTags();
	bool Valid() const;

private:
	void DisplayInfo();
	bool IsPlaying();
	bool StartPlaying();

	bool DecodeStream(ALuint buffer);
	void EmptyBuffers();
	void ReleaseBuffers();
	
	/**
	@brief Decode next part of the stream and queu it for playing
	@return wheter it is the end of the stream (check for IsPlaying() wheter the complete stream was played)
	*/
	bool UpdateBuffers();

	OggVorbis_File oggStream;
	vorbis_info* vorbisInfo;

	ALuint buffers[2];
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
