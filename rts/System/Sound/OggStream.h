#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#include <AL/al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <string>

class COggStream
{
public:
	COggStream();

	void Play(const std::string& path, float volume);
	void Stop();
	void TogglePause();
	void Update();
	float GetPlayTime();
	float GetTotalTime();
	void SetVolume(float, bool b = false);

private:
	void DisplayInfo();
	bool IsPlaying();
	bool StartPlaying();

	bool DecodeStream(ALuint buffer);
	void EmptyBuffers();
	void ReleaseBuffers();
	bool UpdateBuffers();

	OggVorbis_File oggStream;
	vorbis_info* vorbisInfo;
	vorbis_comment* vorbisComment;

	ALuint buffers[2];
	ALuint source;
	ALenum format;

	bool stopped;
	bool paused;
};


#endif
