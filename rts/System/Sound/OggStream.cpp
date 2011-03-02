/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "OggStream.h"

#include <SDL.h>

#include "FileSystem/FileHandler.h"
#include "LogOutput.h"
#include "ALShared.h"
#include "VorbisShared.h"

namespace VorbisCallbacks {
	size_t VorbisStreamRead(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		CFileHandler* buffer = (CFileHandler*)datasource;
		return buffer->Read(ptr, size * nmemb);
	}

	int VorbisStreamClose(void* datasource)
	{
		CFileHandler* buffer = (CFileHandler*)datasource;
		delete buffer;
		return 0;
	}

	int VorbisStreamSeek(void* datasource, ogg_int64_t offset, int whence)
	{
		CFileHandler* buffer = (CFileHandler*)datasource;
		if (whence == SEEK_SET)
		{
			buffer->Seek(offset, std::ios_base::beg);
		}
		else if (whence == SEEK_CUR)
		{
			buffer->Seek(offset, std::ios_base::cur);
		}
		else if (whence == SEEK_END)
		{
			buffer->Seek(offset, std::ios_base::end);
		}

		return 0;
	}

	long VorbisStreamTell(void* datasource)
	{
		CFileHandler* buffer = (CFileHandler*)datasource;
		return buffer->GetPos();
	}

}



COggStream::COggStream(ALuint _source)
	: vorbisInfo(NULL)
	, source(_source)
	, format(AL_FORMAT_MONO16)
	, stopped(true)
	, paused(false)
	, msecsPlayed(0)
	, lastTick(0)
{
	for (unsigned i = 0; i < NUM_BUFFERS; ++i) {
		buffers[i] = 0;
	}
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

	vorbisTags.clear();
	int result = 0;

	ov_callbacks vorbisCallbacks;
		vorbisCallbacks.read_func  = VorbisCallbacks::VorbisStreamRead;
		vorbisCallbacks.close_func = VorbisCallbacks::VorbisStreamClose;
		vorbisCallbacks.seek_func  = VorbisCallbacks::VorbisStreamSeek;
		vorbisCallbacks.tell_func  = VorbisCallbacks::VorbisStreamTell;

	CFileHandler* buf = new CFileHandler(path);
	if ((result = ov_open_callbacks(buf, &oggStream, NULL, 0, vorbisCallbacks)) < 0) {
		logOutput.Print("Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
		return;
	}


	vorbisInfo = ov_info(&oggStream, -1);
	{
		vorbis_comment* vorbisComment;
		vorbisComment = ov_comment(&oggStream, -1);
		vorbisTags.resize(vorbisComment->comments);

		for (unsigned i = 0; i < vorbisComment->comments; ++i) {
			vorbisTags[i] = std::string(vorbisComment->user_comments[i], vorbisComment->comment_lengths[i]);
		}

		vendor = std::string(vorbisComment->vendor);
		// DisplayInfo();
	}

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

float COggStream::GetPlayTime() const
{
	return (msecsPlayed / 1000.0f);
}

float COggStream::GetTotalTime()
{
	return ov_time_total(&oggStream, -1);
}

const COggStream::TagVector& COggStream::VorbisTags() const
{
	return vorbisTags;
}

bool COggStream::Valid() const
{
	return (vorbisInfo != 0);
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
	logOutput.Print("vendor:            %s", vendor.c_str());

	for (TagVector::const_iterator it = vorbisTags.begin(); it != vorbisTags.end(); ++it) {
		logOutput.Print("%s", it->c_str());
	}
}


// clean up the OpenAL resources
void COggStream::ReleaseBuffers()
{
	stopped = true;
	paused = false;

	EmptyBuffers();

	alDeleteBuffers(2, buffers);
	CheckError("COggStream");

	ov_clear(&oggStream);
}


// returns true if both buffers were
// filled with data from the stream
bool COggStream::StartPlaying()
{
	msecsPlayed = 0;
	lastTick = SDL_GetTicks();

	if (!DecodeStream(buffers[0])) { return false; }
	if (!DecodeStream(buffers[1])) { return false; }

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
	if (stopped) {
		return;
	}

	ReleaseBuffers();
	msecsPlayed = 0;
	vorbisInfo = NULL;
	lastTick = SDL_GetTicks();
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
	int buffersProcessed = 0;
	bool active = true;

	alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffersProcessed);

	while (buffersProcessed-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream");

		// false if we've reached end of stream
		active = DecodeStream(buffer);
		if (active)
			alSourceQueueBuffers(source, 1, &buffer); CheckError("COggStream");
	}

	return active;
}


void COggStream::Update()
{
	if (stopped) {
		return;
	}

	unsigned tick = SDL_GetTicks();

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

		msecsPlayed += (tick - lastTick);
	}

	lastTick = tick;
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
	int queuedBuffers = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queuedBuffers); CheckError("COggStream");

	while (queuedBuffers-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream");
	}
}
