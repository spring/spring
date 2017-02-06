/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "OggStream.h"

#include <string.h> //memset
#include "System/FileSystem/FileHandler.h"
#include "System/Sound/SoundLog.h"
#include "ALShared.h"
#include "VorbisShared.h"


namespace VorbisCallbacks {
	size_t VorbisStreamRead(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		CFileHandler* buffer = static_cast<CFileHandler*>(datasource);
		return buffer->Read(ptr, size * nmemb);
	}

	int VorbisStreamClose(void* datasource)
	{
		CFileHandler* buffer = static_cast<CFileHandler*>(datasource);
		delete buffer;
		return 0;
	}

	int VorbisStreamSeek(void* datasource, ogg_int64_t offset, int whence)
	{
		CFileHandler* buffer = static_cast<CFileHandler*>(datasource);
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
		CFileHandler* buffer = static_cast<CFileHandler*>(datasource);
		return buffer->GetPos();
	}

}



COggStream::COggStream(ALuint _source)
	: vorbisInfo(nullptr)
	, source(_source)
	, format(AL_FORMAT_MONO16)
	, stopped(true)
	, paused(false)
{
	for (unsigned i = 0; i < NUM_BUFFERS; ++i) {
		buffers[i] = 0;
	}
	for (unsigned i = 0; i < BUFFER_SIZE; ++i) {
		pcmDecodeBuffer[i] = 0;
	}
}

COggStream::~COggStream()
{
	Stop();
}

// open an Ogg stream from a given file and start playing it
void COggStream::Play(const std::string& path, float volume)
{
	if (!stopped) {
		// we're already playing another stream
		return;
	}

	vorbisTags.clear();

	ov_callbacks vorbisCallbacks;
		vorbisCallbacks.read_func  = VorbisCallbacks::VorbisStreamRead;
		vorbisCallbacks.close_func = VorbisCallbacks::VorbisStreamClose;
		vorbisCallbacks.seek_func  = VorbisCallbacks::VorbisStreamSeek;
		vorbisCallbacks.tell_func  = VorbisCallbacks::VorbisStreamTell;

	CFileHandler* buf = new CFileHandler(path);
	const int result = ov_open_callbacks(buf, &oggStream, nullptr, 0, vorbisCallbacks);
	if (result < 0) {
		LOG_L(L_WARNING, "Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
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

	alGenBuffers(2, buffers); CheckError("COggStream::Play");

	if (!StartPlaying()) {
		ReleaseBuffers();
	} else {
		stopped = false;
		paused = false;
	}

	CheckError("COggStream::Play");
}

float COggStream::GetPlayTime() const
{
	return msecsPlayed.toSecsf();
}

float COggStream::GetTotalTime()
{
	return ov_time_total(&oggStream, -1);
}

bool COggStream::Valid() const
{
	return (vorbisInfo != 0);
}

bool COggStream::IsFinished()
{
	return !Valid() || (GetPlayTime() >= GetTotalTime());
}

const COggStream::TagVector& COggStream::VorbisTags() const
{
	return vorbisTags;
}

// display Ogg info and comments
void COggStream::DisplayInfo()
{
	LOG("version:           %d", vorbisInfo->version);
	LOG("channels:          %d", vorbisInfo->channels);
	LOG("time (sec):        %lf", ov_time_total(&oggStream,-1));
	LOG("rate (Hz):         %ld", vorbisInfo->rate);
	LOG("bitrate (upper):   %ld", vorbisInfo->bitrate_upper);
	LOG("bitrate (nominal): %ld", vorbisInfo->bitrate_nominal);
	LOG("bitrate (lower):   %ld", vorbisInfo->bitrate_lower);
	LOG("bitrate (window):  %ld", vorbisInfo->bitrate_window);
	LOG("vendor:            %s", vendor.c_str());

	for (TagVector::const_iterator it = vorbisTags.begin(); it != vorbisTags.end(); ++it) {
		LOG("%s", it->c_str());
	}
}


// clean up the OpenAL resources
void COggStream::ReleaseBuffers()
{
	stopped = true;
	paused = false;

	EmptyBuffers();

	alDeleteBuffers(2, buffers);
	CheckError("COggStream::ReleaseBuffers");

	ov_clear(&oggStream);
}


// returns true if both buffers were
// filled with data from the stream
bool COggStream::StartPlaying()
{
	msecsPlayed = spring_nulltime;
	lastTick = spring_gettime();

	if (!DecodeStream(buffers[0])) { return false; }
	if (!DecodeStream(buffers[1])) { return false; }

	alSourceQueueBuffers(source, 2, buffers); CheckError("COggStream::StartPlaying");
	alSourcePlay(source); CheckError("COggStream::StartPlaying");

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
	if (stopped)
		return;

	ReleaseBuffers();
	msecsPlayed = spring_nulltime;
	vorbisInfo = nullptr;
	lastTick = spring_gettime();
}

bool COggStream::TogglePause()
{
	if (!stopped)
		paused = !paused;

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
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream::UpdateBuffers");

		// false if we've reached end of stream
		active = DecodeStream(buffer);
		if (active) {
			alSourceQueueBuffers(source, 1, &buffer); CheckError("COggStream::UpdateBuffers");
		}
	}
	CheckError("COggStream::UpdateBuffers");

	return active;
}


void COggStream::Update()
{
	if (stopped)
		return;

	spring_time tick = spring_gettime();

	if (!paused) {
		UpdateBuffers();

		if (!IsPlaying()) {
			ReleaseBuffers();
		}

		msecsPlayed += (tick - lastTick);
	}

	lastTick = tick;
}


// read decoded data from audio stream into PCM buffer
bool COggStream::DecodeStream(ALuint buffer)
{
	memset(pcmDecodeBuffer, 0, BUFFER_SIZE);

	int size = 0;
	int section = 0;
	int result = 0;

	while (size < BUFFER_SIZE) {
		result = ov_read(&oggStream, pcmDecodeBuffer + size, BUFFER_SIZE - size, 0, 2, 1, &section);

		if (result > 0) {
			size += result;
		} else {
			if (result < 0) {
				LOG_L(L_WARNING, "Error reading Ogg stream (%s)", ErrorString(result).c_str());
			} else {
				break;
			}
		}
	}

	if (size == 0)
		return false;

	alBufferData(buffer, format, pcmDecodeBuffer, size, vorbisInfo->rate);
	CheckError("COggStream::DecodeStream");

	return true;
}


// dequeue any buffers pending on source
void COggStream::EmptyBuffers()
{
	int queuedBuffers = 0;
	alGetSourcei(source, AL_BUFFERS_QUEUED, &queuedBuffers); CheckError("COggStream::EmptyBuffers");

	while (queuedBuffers-- > 0) {
		ALuint buffer;
		alSourceUnqueueBuffers(source, 1, &buffer); CheckError("COggStream::EmptyBuffers");
	}
}

