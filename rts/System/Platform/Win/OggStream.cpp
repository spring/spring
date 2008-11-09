#include <SDL.h>

#include "Platform/Linux/OggStream.h"
#include "LogOutput.h"


COggStream::COggStream() {
	DS = 0x0;
	DSB = 0x0;
	oggFile = 0x0;
	vorbisInfo = 0x0;
	vorbisComment = 0x0;

	secsPlayed = 0;
	lastTick = 0;

	stopped = true;
	paused = false;
	isLastSection = true;
	reachedEOS = true;
}


void COggStream::Play(const std::string& path, float volume, const float3& position) {
	if (!stopped) {
		return;
	}

	int result = 0;

	if (!(oggFile = fopen(path.c_str(), "rb"))) {
		logOutput.Print("Could not open Ogg file.");
		return;
	}

	if ((result = ov_open(oggFile, &oggStream, NULL, 0)) < 0) {
		fclose(oggFile);
		logOutput.Print("Could not open Ogg stream.");
		return;
	}

	vorbisInfo = ov_info(&oggStream, -1);
	vorbisComment = ov_comment(&oggStream, -1);
	// DisplayInfo();

	// set the wave format
	WAVEFORMATEX wfm;
	memset(&wfm, 0, sizeof(wfm));

	// Ogg-Vorbis is always 16 bit
	wfm.cbSize			= sizeof(wfm);
	wfm.nChannels		= vorbisInfo->channels;
	wfm.wBitsPerSample	= 16;
	wfm.nSamplesPerSec	= vorbisInfo->rate;
	wfm.nAvgBytesPerSec	= wfm.nSamplesPerSec * wfm.nChannels * 2;
	wfm.nBlockAlign		= 2 * wfm.nChannels;
	wfm.wFormatTag		= 1;


	// set up the stream buffer
	DSBUFFERDESC desc;

	desc.dwSize         = sizeof(desc);
	desc.dwFlags        = DSBCAPS_CTRLVOLUME;
	desc.lpwfxFormat    = &wfm;
	desc.dwReserved     = 0;

	desc.dwBufferBytes  = BUFSIZE * 2;
	DS->CreateSoundBuffer(&desc, &DSB, NULL);

	// fill the buffer
	DWORD pos = 0;
	int sec = 0;
	int ret = 1;
	DWORD size = BUFSIZE * 2;

	SetVolume(volume, true);

	char* buf;
	// offset to lock start, lock size, address of first lock part, size of first part, 0, 0, flag
	DSB->Lock(0, size, (LPVOID*) &buf, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);


	// read in the stream bits
	while (ret && pos < size) {
		ret = ov_read(&oggStream, buf + pos, size - pos, 0, 2, 1, &sec);
		pos += ret;
	}

	DSB->Unlock(buf, size, NULL, 0);
	DSB->Play(0, 0, DSBPLAY_LOOPING);

	curSection = 0;
	lastSection = 0;

	secsPlayed = 0;
	lastTick = SDL_GetTicks();

	stopped = false;
	paused = false;

	isLastSection = false;
	reachedEOS = false;
}


// stops the currently playing stream
// and cleans up the associated buffer
void COggStream::Stop() {
	if (!stopped) {
		stopped = true;
		DSB->Stop();
		DSB->Release();
	}
}

void COggStream::SetVolume(float volume, bool b) {
	if (!stopped || b) {
		// clamp the volume to the interval [0, 1]
		float v = std::max(0.0f, std::min(volume, 1.0f));

		// SetVolume() wants the volume level specified in hundredths of
		// decibels between DSBVOLUME_MIN [-10.000] and DSBVOLUME_MAX [0]
		// but we assume <volume> is between 0 and 1, linearly convert it
		//
		// 0.0 --> -10000
		// 0.1 -->  -9000
		// ..............
		// 1.0 -->      0
		long db = (long)(1.0f - v) * -10000;

		DSB->SetVolume(db);
	}
}

void COggStream::TogglePause() {
	if (!stopped) {
		paused = !paused;

		if (paused) {
			DSB->Stop();
		} else {
			DSB->Play(0, 0, DSBPLAY_LOOPING);
		}
	}
}

unsigned int COggStream::GetTotalTime() {
	return ov_time_total(&oggStream,-1);
}

void COggStream::UpdateTimer() {
	unsigned int tick = SDL_GetTicks();

	if (paused) {
		lastTick = tick;
	}

	if ((tick - lastTick) >= 1000) {
		secsPlayed += (tick - lastTick) / 1000;
		lastTick = tick;
	}
}

void COggStream::Update() {
	if (!stopped) {
		UpdateTimer();

		if (!paused) {
			DWORD pos;
			DSB->GetCurrentPosition(&pos, NULL);
			curSection = (pos < BUFSIZE)? 0: 1;

			// buffer section changed?
			if (curSection != lastSection) {
				if (reachedEOS) {
					Stop();
					return;
				}
				if (isLastSection) {
					reachedEOS = true;
				}

				DWORD size = BUFSIZE;
				char* buf;

				// fill the section we switched from
				DSB->Lock(lastSection * BUFSIZE, size, (LPVOID*) &buf, &size, NULL, NULL, 0);

				DWORD pos = 0;
				int sec = 0;
				int ret = 1;

				while (ret && pos < size) {
					ret = ov_read(&oggStream, buf + pos, size - pos, 0, 2, 1, &sec);
					pos += ret;
				}

				if (!ret) {
					// EOS reached, zero rest of buffer
					while (pos < size) {
						*(buf + pos++) = 0;
					}

					// only this buffer section to go
					isLastSection = true;
				}

				DSB->Unlock(buf, size, NULL, 0);
				lastSection = curSection;
			}
		}
	}
}


// display Ogg info and comments
void COggStream::DisplayInfo() {
	logOutput.Print("version:           %d", vorbisInfo->version);
	logOutput.Print("channels:          %d", vorbisInfo->channels);
	logOutput.Print("time (sec):        %d", ov_time_total(&oggStream,-1));
	logOutput.Print("rate (Hz):         %d", vorbisInfo->rate);
	logOutput.Print("bitrate (upper):   %d", vorbisInfo->bitrate_upper);
	logOutput.Print("bitrate (nominal): %d", vorbisInfo->bitrate_nominal);
	logOutput.Print("bitrate (lower):   %d", vorbisInfo->bitrate_lower);
	logOutput.Print("bitrate (window):  %d", vorbisInfo->bitrate_window);
	logOutput.Print("vendor:            %s", vorbisComment->vendor);

	for (int i = 0; i < vorbisComment->comments; i++) {
		logOutput.Print("%s", vorbisComment->user_comments[i]);
	}
}
