#include "OggStream.h"
#include "LogOutput.h"


COggStream::COggStream() {
	DS = 0;
	DSB = 0;
	oggFile = 0;
	vorbisInfo = 0;
	vorbisComment = 0;
	stopped = true;
	isLastSection = true;
	reachedEOS = true;
}


void COggStream::play(const std::string& path, float volume, const float3& position) {
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
	// display();

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
	desc.dwFlags        = 0;
	desc.lpwfxFormat    = &wfm;
	desc.dwReserved     = 0;

	desc.dwBufferBytes  = BUFSIZE * 2;
	DS->CreateSoundBuffer(&desc, &DSB, NULL);

	// fill the buffer
	DWORD pos = 0;
	int sec = 0;
	int ret = 1;
	DWORD size = BUFSIZE * 2;

	char* buf;
	// offset to lock start, lock size, address of first lock part, size of first part, 0, 0, flag
	DSB->Lock(0, size, (LPVOID*) &buf, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
	DSB->SetVolume(int(DSBVOLUME_MIN * volume));

	// read in the stream bits
	while (ret && pos < size) {
		ret = ov_read(&oggStream, buf + pos, size - pos, 0, 2, 1, &sec);
		pos += ret;
	}

	DSB->Unlock(buf, size, NULL, 0);
	DSB->Play(0, 0, DSBPLAY_LOOPING);

	curSection = 0;
	lastSection = 0;
	stopped = false;
	isLastSection = false;
	reachedEOS = false;
}


// stops the currently playing stream
// and cleans up the associated buffer
void COggStream::stop() {
	if (!stopped) {
		stopped = true;
		DSB->Stop();
		DSB->Release();
	}
}


void COggStream::update() {
	if (stopped) {
		return;
	}

	DWORD pos;
	DSB->GetCurrentPosition(&pos, NULL);
	curSection = (pos < BUFSIZE)? 0: 1;

	// buffer section changed?
	if (curSection != lastSection) {
		if (reachedEOS) {
			stop();
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


// display Ogg info and comments
void COggStream::display() {
	logOutput.Print("version:           %d", vorbisInfo->version);
	logOutput.Print("channels:          %d", vorbisInfo->channels);
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
