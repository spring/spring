#ifndef OGGSTREAM_H
#define OGGSTREAM_H

#ifndef __GNUC__
#include "StdAfx.h"
#endif

#include "DxSound.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <string>

// 512KB buffer
#define BUFSIZE (4096 * 128)

class COggStream {
	public:
		COggStream();

		inline void SetDSoundObject(LPDIRECTSOUND _DS) { DS = _DS; }

		void Play(const std::string& path, float volume, const float3& position);
		void Stop();
		void TogglePause();
		void Update();
		unsigned int GetPlayTime() const { return ((stopped)? 0 : secsPlayed); }
		unsigned int GetTime();
		void SetVolume(float, bool b = false);

	private:
		void UpdateTimer();
		void DisplayInfo();

		LPDIRECTSOUND DS;
		LPDIRECTSOUNDBUFFER DSB;

		FILE* oggFile;
		OggVorbis_File oggStream;
		vorbis_info* vorbisInfo;
		vorbis_comment* vorbisComment;

		int curSection;
		int lastSection;
		bool isLastSection;
		bool reachedEOS;

		unsigned int secsPlayed;
		unsigned int lastTick;

		bool stopped;
		bool paused;
};

#endif
