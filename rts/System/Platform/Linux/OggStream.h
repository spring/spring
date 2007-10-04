#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#ifdef OGGSTREAM_PLAYBACK

#include "StdAfx.h"

#include <iostream>
#include <string>

#include <AL/al.h>
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

// 256KB buffer
#define BUFFER_SIZE (4096 * 64)

class COggStream {
	public:
		COggStream();

		void play(const std::string& path, const float3& pos, float volume);
		void release();
		void display();
		bool playback();
		bool playing();
		void stop();
		bool updateBuffers();
		void update();

    protected:
		bool stream(ALuint buffer);
		void empty();
		void check();
		std::string errorString(int code);

	private:
		FILE*			oggFile;
		OggVorbis_File	oggStream;
		vorbis_info*	vorbisInfo;
		vorbis_comment*	vorbisComment;

		ALuint buffers[2];
		ALuint source;
		ALenum format;

		bool stopped;
};


#endif

#endif
