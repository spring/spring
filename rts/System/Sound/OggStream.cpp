#include "OggStream.h"

#include "FileSystem/FileHandler.h"
#include "LogOutput.h"
#include "ALShared.h"
#include "VorbisShared.h"

namespace
{
// 512KB buffer
const int BUFFER_SIZE = (4096 * 128);

size_t VorbisStreamRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	CFileHandler* buffer = (CFileHandler*)datasource;
	return buffer->Read(ptr, size * nmemb);
}

int	VorbisStreamClose(void* datasource)
{
	CFileHandler* buffer = (CFileHandler*)datasource;
	delete buffer;
	return 0;
}
}

COggStream::COggStream(ALuint _source)
{
	source = _source;
	vorbisInfo = 0x0;
	vorbisComment = 0x0;

	stopped = true;
	paused = false;
}

COggStream::~COggStream()
{
	if (!stopped) {
		ReleaseBuffers();
	}
}

// open an Ogg stream from a given file and start playing it
void COggStream::Play(const std::string& path, float volume)
{
	if (!stopped) {
		// we're already playing another stream
		return;
	}

	int result = 0;

	ov_callbacks vorbisCallbacks;
	vorbisCallbacks.read_func  = VorbisStreamRead;
	vorbisCallbacks.close_func = VorbisStreamClose;
	vorbisCallbacks.seek_func  = NULL;
	vorbisCallbacks.tell_func  = NULL;

	CFileHandler* buf = new CFileHandler(path);
	if ((result = ov_open_callbacks(buf, &oggStream, NULL, 0, vorbisCallbacks)) < 0) {
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

	alGenBuffers(2, buffers); CheckError("COggStream");

	if (!StartPlaying()) {
		ReleaseBuffers();
	} else {
		stopped = false;
		paused = false;
	}
	CheckError("COggStream");
}

float COggStream::GetPlayTime()
{
	return ((stopped)? 0.0 : ov_time_tell(&oggStream));
}

float COggStream::GetTotalTime()
{
	return ov_time_total(&oggStream,-1);
}


// display Ogg info and comments
void COggStream::DisplayInfo()
{
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
void COggStream::ReleaseBuffers()
{
	stopped = true;
	paused = false;

	EmptyBuffers();

	alDeleteBuffers(2, buffers); CheckError("COggStream");

	ov_clear(&oggStream);
}


// returns true if both buffers were
// filled with data from the stream
bool COggStream::StartPlaying()
{
	if (!DecodeStream(buffers[0]))
		return false;

	if (!DecodeStream(buffers[1]))
		return false;

	alSourceQueueBuffers(source, 2, buffers); CheckError("COggStream");
	alSourcePlay(source); CheckError("COggStream");

	return true;
}


// returns true if we're still playing
bool COggStream::IsPlaying()
{
	ALenum state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);

	return (state == AL_PLAYING);
}

// stops the currently playing stream
void COggStream::Stop()
{
	if (!stopped) {
		ReleaseBuffers();
	}
}

bool COggStream::TogglePause()
{
	if (!stopped) {
		paused = !paused;
	}
	return paused;
}


// pop the processed buffers from the queue,
// refill them, and push them back in line
bool COggStream::UpdateBuffers()
{
	int processed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	while (processed-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream");

		// false if we've reached end of stream
		active = DecodeStream(buffer);

		alSourceQueueBuffers(source, 1, &buffer); CheckError("COggStream");
	}

	return active;
}


void COggStream::Update()
{
	if (!stopped) {
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
			} else if (!IsPlaying()) {
				// EOS and all chunks processed by OpenALs
				ReleaseBuffers();
			}
		}
	}
}


// read decoded data from audio stream into PCM buffer
bool COggStream::DecodeStream(ALuint buffer)
{
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
	CheckError("COggStream");

	return true;
}


// dequeue any buffers pending on source
void COggStream::EmptyBuffers()
{
	int queued = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queued); CheckError("COggStream");

	while (queued-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream");
	}
}
