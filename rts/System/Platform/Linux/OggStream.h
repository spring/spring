#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#include "StdAfx.h"

#include <AL/al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include <string>

// 512KB buffer
#define BUFFER_SIZE (4096 * 128)

class COggStream {
	public:
		COggStream();

		void Play(const std::string& path, const float3& pos, float volume);
		void Stop();
		void TogglePause();
		void Update();

    private:
		void Release();
		void DisplayInfo();
		bool IsPlaying();
		bool StartPlaying();

		bool Stream(ALuint buffer);
		void EmptyBuffers();
		bool UpdateBuffers();
		void CheckErrors();
		std::string ErrorString(int code);

		FILE* oggFile;
		OggVorbis_File oggStream;
		vorbis_info* vorbisInfo;
		vorbis_comment* vorbisComment;

		ALuint buffers[2];
		ALuint source;
		ALenum format;

		unsigned int secsPlayed;
		unsigned int lastTick;
		bool stopped;
		bool paused;
};


#endif
