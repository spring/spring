#include <SDL.h>

#include "LogOutput.h"
#include "OggStream.h"

// 512KB buffer
const int BUFFER_SIZE = (4096 * 128);

COggStream::COggStream() {
	source = 0;
	oggFile = 0x0;
	vorbisInfo = 0x0;
	vorbisComment = 0x0;

	secsPlayed = 0;
	lastTick = 0;

	stopped = true;
	paused = false;
}

// open an Ogg stream from a given file and start playing it
void COggStream::Play(const std::string& path, float volume) {
	if (!stopped) {
		// we're already playing another stream
		return;
	}

	int result = 0;

	if (!(oggFile = fopen(path.c_str(), "rb"))) {
		logOutput.Print("Could not open Ogg file.");
		return;
	}

	if ((result = ov_open(oggFile, &oggStream, NULL, 0)) < 0) {
		fclose(oggFile);
		logOutput.Print("Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
		return;
	}

	vorbisInfo = ov_info(&oggStream, -1);
	vorbisComment = ov_comment(&oggStream, -1);
	// DisplayInfo();

	if (vorbisInfo->channels == 1) {
		format = AL_FORMAT_MONO16;
	} else {
		format = AL_FORMAT_STEREO16;
	}

	alGenBuffers(2, buffers); CheckErrors();
	alGenSources(1, &source); CheckErrors();

	SetVolume(volume, true);

	alSource3f(source, AL_POSITION,        0.0, 0.0, 0.0);
	alSource3f(source, AL_VELOCITY,        0.0f,  0.0f,  0.0f );
	alSource3f(source, AL_DIRECTION,       0.0f,  0.0f,  0.0f );
	alSourcef( source, AL_ROLLOFF_FACTOR,  0.0f               );
	alSourcei( source, AL_SOURCE_RELATIVE, false              );

	if (!StartPlaying()) {
		ReleaseBuffers();
	} else {
		secsPlayed = 0;
		lastTick = SDL_GetTicks();
		stopped = false;
		paused = false;
	}
}


void COggStream::SetVolume(float volume, bool b) {
	if (!stopped || b) {
		volume = std::max(0.0f, std::min(volume, 1.0f));

		alSourcef(source, AL_GAIN, volume);
	}
}


unsigned int COggStream::GetTotalTime() {
	return ov_time_total(&oggStream,-1);
}


// display Ogg info and comments
void COggStream::DisplayInfo() {
	logOutput.Print("version:           %d", vorbisInfo->version);
	logOutput.Print("channels:          %d", vorbisInfo->channels);
	logOutput.Print("time (sec):        %lf", ov_time_total(&oggStream,-1));
	logOutput.Print("rate (Hz):         %ld", vorbisInfo->rate);
	logOutput.Print("bitrate (upper):   %ld", vorbisInfo->bitrate_upper);
	logOutput.Print("bitrate (nominal): %ld", vorbisInfo->bitrate_nominal);
	logOutput.Print("bitrate (lower):   %ld", vorbisInfo->bitrate_lower);
	logOutput.Print("bitrate (window):  %ld", vorbisInfo->bitrate_window);
	logOutput.Print("vendor:            %s", vorbisComment->vendor);

	for (int i = 0; i < vorbisComment->comments; i++) {
		logOutput.Print("%s", vorbisComment->user_comments[i]);
	}
}


// clean up the OpenAL resources
void COggStream::ReleaseBuffers() {
	stopped = true;
	paused = false;

	alSourceStop(source); EmptyBuffers();

	alDeleteBuffers(2, buffers); CheckErrors();
	alDeleteSources(1, &source); CheckErrors();

	ov_clear(&oggStream);
}



// returns true if both buffers were
// filled with data from the stream
bool COggStream::StartPlaying() {
	if (!DecodeStream(buffers[0]))
		return false;

	if (!DecodeStream(buffers[1]))
		return false;

	alSourceQueueBuffers(source, 2, buffers);
	alSourcePlay(source);

	return true;
}



// returns true if we're still playing
bool COggStream::IsPlaying() {
	ALenum state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);

	return (state == AL_PLAYING);
}

// stops the currently playing stream
void COggStream::Stop() {
	if (!stopped) {
		ReleaseBuffers();
	}
}

void COggStream::TogglePause() {
	if (!stopped) {
		paused = !paused;

		if (paused) {
			alSourcePause(source);
		} else {
			alSourcePlay(source);
		}
	}
}




// pop the processed buffers from the queue,
// refill them, and push them back in line
bool COggStream::UpdateBuffers() {
	int processed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	while (processed-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckErrors();

		// false if we've reached end of stream
		active = DecodeStream(buffer);

		alSourceQueueBuffers(source, 1, &buffer); CheckErrors();
	}

	return active;
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
			if (UpdateBuffers()) {
				if (!IsPlaying()) {
					// source state changed
					if (!StartPlaying()) {
						// stream stopped
						ReleaseBuffers();
					} else {
						// stream interrupted
						ReleaseBuffers();
					}
				}
			} else {
				// EOS, nothing left to do
				ReleaseBuffers();
			}
		}
	}
}



// read decoded data from audio stream into PCM buffer
bool COggStream::DecodeStream(ALuint buffer) {
	char pcm[BUFFER_SIZE];
	int size = 0;
	int section = 0;
	int result = 0;

	while (size < BUFFER_SIZE) {
		result = ov_read(&oggStream, pcm + size, BUFFER_SIZE - size, 0, 2, 1, &section);

		if (result > 0) {
			size += result;
		} else {
			if (result < 0) {
				logOutput.Print("Error reading Ogg stream (%s)", ErrorString(result).c_str());
			} else {
				break;
			}
		}
	}

	if (size == 0) {
		return false;
	}

	alBufferData(buffer, format, pcm, size, vorbisInfo->rate);
	CheckErrors();

	return true;
}



// dequeue any buffers pending on source
void COggStream::EmptyBuffers() {
	int queued = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

	while (queued-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer);
		CheckErrors();
	}
}



// check for any OpenAL errors
void COggStream::CheckErrors() {
	int error = alGetError();

	if (error != AL_NO_ERROR) {
		logOutput.Print("OpenAL error occurred, code: %d", error);
	}
}



std::string COggStream::ErrorString(int code) {
	switch (code) {
		case OV_EREAD:
			return std::string("Read from media.");
		case OV_ENOTVORBIS:
			return std::string("Not Vorbis data.");
		case OV_EVERSION:
			return std::string("Vorbis version mismatch.");
		case OV_EBADHEADER:
			return std::string("Invalid Vorbis header.");
		case OV_EFAULT:
			return std::string("Internal logic fault (bug or heap/stack corruption.");
		default:
			return std::string("Unknown Ogg error.");
	}
}
