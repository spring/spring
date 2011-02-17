/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SoundSource.h"

#include <climits>
#include <alc.h>
#include <SDL_timer.h>

#include "SoundLog.h"
#include "SoundBuffer.h"
#include "SoundItem.h"
#include "OggStream.h"
#include "ALShared.h"
#include "float3.h"
#include "Util.h"
#include "LogOutput.h"
#include "myMath.h"

float SoundSource::globalPitch = 1.0;
float SoundSource::heightAdjustedRolloffModifier = 1.0;
float SoundSource::referenceDistance = 200.0f;
float SoundSource::airAbsorption = AL_MIN_AIR_ABSORPTION_FACTOR;
bool SoundSource::airAbsorptionSupported = false;

struct SoundSource::StreamControl
{
	StreamControl(ALuint id) : stream(id), current(NULL), next(NULL), stopRequest(false) {};
	COggStream stream;

	struct TrackItem
	{
		std::string file;
		float volume;
		bool enqueue;
	};

	TrackItem* current;
	TrackItem* next;

	bool stopRequest;
};

SoundSource::SoundSource() : curPlaying(NULL), curStream(NULL)
{
	alGenSources(1, &id);
	if (!CheckError("SoundSource::SoundSource"))
	{
		id = 0;
	}
	else
	{
		alSourcef(id, AL_REFERENCE_DISTANCE, referenceDistance);
		CheckError("SoundSource::SoundSource");
	}
}

SoundSource::~SoundSource()
{
	alDeleteSources(1, &id);
	CheckError("SoundSource::~SoundSource");
}

void SoundSource::Update()
{
	if (curStream)
	{
		boost::mutex::scoped_lock lock(streamMutex);
		if (curStream->stopRequest)
		{
			Stop();
			delete curStream->current;
			delete curStream->next;
			delete curStream;
			curStream = NULL;
			CheckError("SoundSource::Update()");
		}
		else
		{
			bool finished = curStream->stream.Valid() ? curStream->stream.GetPlayTime() >= curStream->stream.GetTotalTime() : true;
			if (curStream->next != NULL && (finished ||  !curStream->next->enqueue))
			{
				Stop();
				// COggStreams only appends buffers, giving errors when a buffer of another format is still asigned
				alSourcei(id, AL_BUFFER, AL_NONE);
				curStream->stream.Stop();
				delete curStream->current;
				curStream->current = curStream->next;
				curStream->next = NULL;

				alSource3f(id, AL_POSITION,        0.0, 0.0, -1.0); // in case of mono streams
				alSourcef(id, AL_GAIN, curStream->current->volume);
				alSource3f(id, AL_VELOCITY,        0.0f,  0.0f,  0.0f);
				alSource3f(id, AL_DIRECTION,       0.0f,  0.0f,  0.0f);
				alSourcef(id, AL_ROLLOFF_FACTOR,  0.0f);
				alSourcei(id, AL_SOURCE_RELATIVE, true);
				curStream->stream.Play(curStream->current->file, curStream->current->volume);
			}
			curStream->stream.Update();
			CheckError("SoundSource::Update");
		}
	}
	if (curPlaying && (!IsPlaying() || ((curPlaying->loopTime > 0) && (loopStop < SDL_GetTicks()))))
		Stop();
}

int SoundSource::GetCurrentPriority() const
{
	if (curStream)
	{
		return INT_MAX;
	}
	else if (!curPlaying) {
		logOutput.Print("Warning: SoundSource::GetCurrentPriority() curPlaying is NULL (id %d)", id);
		return INT_MIN;
	}
	return curPlaying->priority;
}

bool SoundSource::IsPlaying() const
{
	CheckError("SoundSource::IsPlaying");
	ALint state;
	alGetSourcei(id, AL_SOURCE_STATE, &state);
	CheckError("SoundSource::IsPlaying");
	if (state == AL_PLAYING || curStream)
		return true;
	else
	{
		return false;
	}
}

void SoundSource::Stop()
{
	alSourceStop(id);
	if (curPlaying)
	{
		curPlaying->StopPlay();
		curPlaying = NULL;
	}
	CheckError("SoundSource::Stop");
}

void SoundSource::Play(SoundItem* item, const float3& pos, float3 velocity, float volume, bool relative)
{
	assert(!curStream);
	if (!item->PlayNow())
		return;
	Stop();
	curPlaying = item;
	alSourcei(id, AL_BUFFER, item->buffer->GetId());
	alSourcef(id, AL_GAIN, item->GetGain() * volume);
	alSourcef(id, AL_PITCH, item->GetPitch() * globalPitch);
	if (airAbsorptionSupported) {
		alSourcef(id, AL_AIR_ABSORPTION_FACTOR, airAbsorption);
	}
	velocity *= item->dopplerScale;
	alSource3f(id, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
	if (item->loopTime > 0)
		alSourcei(id, AL_LOOPING, AL_TRUE);
	else
		alSourcei(id, AL_LOOPING, AL_FALSE);
	loopStop = SDL_GetTicks() + item->loopTime;
	if (relative || !item->in3D)
	{
		alSourcei(id, AL_SOURCE_RELATIVE, AL_TRUE);
		alSource3f(id, AL_POSITION, 0.0, 0.0, -1.0);
	}
	else
	{
		alSourcei(id, AL_SOURCE_RELATIVE, AL_FALSE);
		alSource3f(id, AL_POSITION, pos.x, pos.y, pos.z);
		alSourcef(id, AL_ROLLOFF_FACTOR, item->rolloff * heightAdjustedRolloffModifier);
#ifdef __APPLE__
		alSourcef(id, AL_MAX_DISTANCE, 1000000.0f);
		// Max distance is too small by default on my Mac...
		ALfloat gain = item->GetGain() * volume;
		if (gain > 1.0f) {
			// OpenAL on Mac cannot handle AL_GAIN > 1 well, so we will adjust settings to get the same output with AL_GAIN = 1.
			ALint model = alGetInteger(AL_DISTANCE_MODEL);
			ALfloat rolloff = item->rolloff * heightAdjustedRolloffModifier, ref = referenceDistance;
			if ((model == AL_INVERSE_DISTANCE_CLAMPED) || (model == AL_INVERSE_DISTANCE)) {
				alSourcef(id, AL_REFERENCE_DISTANCE, ((gain - 1.0f) * ref / rolloff) + ref);
				alSourcef(id, AL_ROLLOFF_FACTOR, (gain + rolloff - 1.0f) / gain);
				alSourcef(id, AL_GAIN, 1.0f);
			}
		} else {
			alSourcef(id, AL_REFERENCE_DISTANCE, referenceDistance);
		}
#endif
	}
	alSourcePlay(id);

	if (item->buffer->GetId() == 0)
		logOutput.Print("SoundSource::Play: Empty buffer for item %s (file %s)", item->name.c_str(), item->buffer->GetFilename().c_str());
	CheckError("SoundSource::Play");
}

void SoundSource::PlayStream(const std::string& file, float volume, bool enqueue)
{
	boost::mutex::scoped_lock lock(streamMutex);
	if (!curStream)
	{
		StreamControl* temp = new StreamControl(id);
		curStream = temp;
	}

	delete curStream->next;
	curStream->next = new StreamControl::TrackItem;
	curStream->next->file = file;
	curStream->next->volume = volume;
	curStream->next->enqueue = enqueue;
}

void SoundSource::StreamStop()
{
	if (curStream)
	{
		boost::mutex::scoped_lock lock(streamMutex);
		if (curStream->current)
		{
			curStream->stopRequest = true;
		}
	}
	else
		assert(false);
}

void SoundSource::StreamPause()
{
	if (curStream)
	{
		boost::mutex::scoped_lock lock(streamMutex);
		if (curStream->current)
		{
			if (curStream->stream.TogglePause())
				alSourcePause(id);
			else
				alSourcePlay(id);
		}
	}
	else
		assert(false);
}

float SoundSource::GetStreamTime()
{
	if (curStream)
	{
		boost::mutex::scoped_lock lock(streamMutex);
		if (curStream->current)
			return curStream->stream.GetTotalTime();
		else
			return 0;
	}
	assert(false);
	return 0;
}

float SoundSource::GetStreamPlayTime()
{
	if (curStream)
	{
		boost::mutex::scoped_lock lock(streamMutex);
		if (curStream->current)
			return curStream->stream.GetPlayTime();
		else
			return 0;
	}
	assert(false);
	return 0;
}

void SoundSource::SetPitch(float newPitch)
{
	globalPitch = newPitch;
}

void SoundSource::SetVolume(float newVol)
{
	if (curStream && curStream->current)
	{
		alSourcef(id, AL_GAIN, newVol * curStream->current->volume);
	}
}

void SoundSource::SetAirAbsorptionSupported(bool supported)
{
	airAbsorptionSupported = supported;
}

void SoundSource::SetAirAbsorption(float factor)
{
	if (airAbsorptionSupported) {
		airAbsorption = Clamp(factor, AL_MIN_AIR_ABSORPTION_FACTOR, AL_MAX_AIR_ABSORPTION_FACTOR);

		if (airAbsorption == AL_MIN_AIR_ABSORPTION_FACTOR)
		{
			LogObject(LOG_SOUND) << "air absorption disabled";
		}
		else
		{
			LogObject(LOG_SOUND) << "air absorption enabled: " << airAbsorption;
		}
	} else {
		LogObject(LOG_SOUND) << "air absorption not supported by the sound device in use";
	}
}
