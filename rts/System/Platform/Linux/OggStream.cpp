#include "LogOutput.h"
#include "OggStream.h"

COggStream::COggStream() {
	source = 0;
	oggFile = 0;
	vorbisInfo = 0;
	vorbisComment = 0;
	stopped = true;
}

// open an Ogg stream from a given file and start playing it
void COggStream::play(const std::string& path, const float3& pos, float volume) {
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
		logOutput.Print("Could not open Ogg stream (reason: %s).", errorString(result).c_str());
		return;
	}

	vorbisInfo = ov_info(&oggStream, -1);
	vorbisComment = ov_comment(&oggStream, -1);
	// display();

	if (vorbisInfo->channels == 1) {
		format = AL_FORMAT_MONO16;
	} else {
		format = AL_FORMAT_STEREO16;
	}

	alGenBuffers(2, buffers);
	check();
	alGenSources(1, &source);
	check();

	alSource3f(source, AL_POSITION,        pos.x, pos.y, pos.z);
	alSource3f(source, AL_VELOCITY,        0.0f,  0.0f,  0.0f );
	alSource3f(source, AL_DIRECTION,       0.0f,  0.0f,  0.0f );
	alSourcef( source, AL_ROLLOFF_FACTOR,  0.0f               );
	alSourcef( source, AL_GAIN,            volume             );
	alSourcei( source, AL_SOURCE_RELATIVE, false              );

	if (!playback()) {
		release();
	} else {
		stopped = false;
	}
}


// display Ogg info and comments
void COggStream::display() {
	logOutput.Print("version:           %d", vorbisInfo->version);
	logOutput.Print("channels:          %d", vorbisInfo->channels);
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
void COggStream::release() {
	alSourceStop(source);
	empty();

	alDeleteBuffers(2, buffers);
	check();

	alDeleteSources(1, &source);
	check();

	ov_clear(&oggStream);
}



// returns true if both buffers were
// filled with data from the stream
bool COggStream::playback() {
	if (!stream(buffers[0]))
		return false;

	if (!stream(buffers[1]))
		return false;

	alSourceQueueBuffers(source, 2, buffers);
	alSourcePlay(source);

	return true;
}



// returns true if we're still playing
bool COggStream::playing() {
	ALenum state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);

	return (state == AL_PLAYING);
}

// stops the currently playing stream
void COggStream::stop() {
	if (playing()) {
		stopped = true;
		release();
	}
}


// pop the processed buffers from the queue,
// refill them, and push them back in line
bool COggStream::updateBuffers() {
	int processed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	while (processed-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer);
		check();

		// false if we've reached end of stream
		active = stream(buffer);

		alSourceQueueBuffers(source, 1, &buffer);
		check();
	}

	return active;
}


void COggStream::update() {
	if (!stopped) {
		if (updateBuffers()) {
			if (!playing()) {
				// source state changed
				if (!playback()) {
					// stream stopped
					stopped = true;
					release();
				} else {
					// stream interrupted
					stopped = true;
					release();
				}
			}
		} else {
			// EOS, nothing left to do
			stopped = true;
			release();
		}
	}
}



// read decoded data from audio stream into PCM buffer
bool COggStream::stream(ALuint buffer) {
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
				logOutput.Print("Error reading Ogg stream (%s)", errorString(result).c_str());
			} else {
				break;
			}
		}
	}

	if (size == 0) {
		return false;
	}

	alBufferData(buffer, format, pcm, size, vorbisInfo->rate);
	check();

	return true;
}



// dequeue any buffers pending on source
void COggStream::empty() {
	int queued = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

	while (queued-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer);
		check();
	}
}



// check for any OpenAL errors
void COggStream::check() {
	int error = alGetError();

	if (error != AL_NO_ERROR) {
		logOutput.Print("OpenAL error occurred, code: %d", error);
	}
}



std::string COggStream::errorString(int code) {
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
