#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#ifdef OGGSTREAM_PLAYBACK

#include "StdAfx.h"
#include "DxSound.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <string>

// 256KB buffer
#define BUFSIZE (4096 * 64)

class COggStream {
	public:
		COggStream();

		inline void setDSoundObject(LPDIRECTSOUND _DS) { DS = _DS; }

		void play(const std::string& path, float volume, const float3& position);
		void stop();
		void update();
		void display();

	private:
		LPDIRECTSOUND DS;
		LPDIRECTSOUNDBUFFER DSB;

		FILE* oggFile;
		OggVorbis_File oggStream;
		vorbis_info* vorbisInfo;
		vorbis_comment* vorbisComment;

		int curSection;
		int lastSection;
		bool stopped;
};

#endif

#endif
