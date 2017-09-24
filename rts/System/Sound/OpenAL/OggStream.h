/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OGG_STREAM_H
#define OGG_STREAM_H

#include "System/Misc/SpringTime.h"

#include <al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <string>
#include <vector>


class COggStream
{
public:
	COggStream(ALuint _source = 0);
	~COggStream() { Stop(); }

	void Play(const std::string& path, float volume);
	void Stop();
	void Update();

	float GetPlayTime() const { return (msecsPlayed.toSecsf()); }
	float GetTotalTime();

	bool TogglePause();
	bool Valid() const { return (source != 0 && vorbisInfo != nullptr); }
	bool IsFinished() { return !Valid() || (GetPlayTime() >= GetTotalTime()); }

	const std::vector<std::string>& VorbisTags() const { return vorbisTags; }

private:
	void DisplayInfo();
	bool IsPlaying();
	bool StartPlaying();

	bool DecodeStream(ALuint buffer);
	void EmptyBuffers();
	void ReleaseBuffers();

	/**
	 * @brief Decode next part of the stream and queue it for playing
	 * @return whether it is the end of the stream
	 *   (check for IsPlaying() whether the complete stream was played)
	 */
	bool UpdateBuffers();

	OggVorbis_File ovFile;
	vorbis_info* vorbisInfo;

	static constexpr unsigned int BUFFER_SIZE = 512 * 1024; // 512KB
	static constexpr unsigned int NUM_BUFFERS = 2;

	char pcmDecodeBuffer[BUFFER_SIZE];

	ALuint buffers[NUM_BUFFERS];
	ALuint source;
	ALenum format;

	bool stopped;
	bool paused;

	spring_time msecsPlayed;
	spring_time lastTick;

	std::vector<std::string> vorbisTags;
	std::string vendor;
};

#endif // OGG_STREAM_H
