/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AudioChannel.h"

#include "ALShared.h"
#include "ISound.h"
#include "SoundItem.h"
#include "SoundLog.h"
#include "SoundSource.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"

extern boost::recursive_mutex soundMutex;

const size_t AudioChannel::MAX_STREAM_QUEUESIZE = 10;


AudioChannel::AudioChannel()
	: curStreamSrc(NULL)
{
}


void AudioChannel::SetVolume(float newVolume)
{
	volume = newVolume;

	boost::recursive_mutex::scoped_lock lck(chanMutex);

	for (std::map<CSoundSource*, bool>::iterator it = cur_sources.begin(); it != cur_sources.end(); ++it)
		it->first->UpdateVolume();
}


void AudioChannel::Enable(bool newState)
{
	enabled = newState;

	if (!enabled)
	{
		SetVolume(0.f);
	}
}


void AudioChannel::SoundSourceFinished(CSoundSource* sndSource)
{
	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc == sndSource) {
		if (!streamQueue.empty()) {
			StreamQueueItem& next = streamQueue.back();
			StreamPlay(next.fileName, next.volume, false);
			streamQueue.pop_back();
		} else {
			curStreamSrc = NULL;
		}
	}

	cur_sources.erase(sndSource);
}


void AudioChannel::FindSourceAndPlay(size_t id, const float3& pos, const float3& velocity, float volume, bool relative)
{
	if (!enabled)
		return;

	if (volume <= 0.0f)
		return;

	boost::recursive_mutex::scoped_lock slck(soundMutex);

	SoundItem* sndItem = sound->GetSoundItem(id);
	if (!sndItem)
	{
		sound->numEmptyPlayRequests++;
		return;
	}

	if (pos.distance(sound->GetListenerPos()) > sndItem->MaxDistance())
	{
		if (!relative) {
			return;
		} else {
			LOG("CSound::PlaySample: maxdist ignored for relative playback: %s",
					sndItem->Name().c_str());
		}
	}

	if (emmitsThisFrame >= emmitsPerFrame)
		return;
	emmitsThisFrame++;
	
	CSoundSource* sndSource = sound->GetNextBestSource();
	if (!sndSource)
		return;

	if (sndSource->GetCurrentPriority() < sndItem->GetPriority())
	{
		if (sndSource->IsPlaying())
			sound->numAbortedPlays++;

		sndSource->Play(this, sndItem, pos, velocity, volume, relative);
		CheckError("CSound::FindSourceAndPlay");

		boost::recursive_mutex::scoped_lock lck(chanMutex);

		cur_sources[sndSource] = true;
	}
}

void AudioChannel::PlaySample(size_t id, float volume)
{
	FindSourceAndPlay(id, float3(0.0f, 0.0f, -1.0f), ZeroVector, volume, true);
}

void AudioChannel::PlaySample(size_t id, const float3& pos, float volume)
{
	FindSourceAndPlay(id, pos, ZeroVector, volume, false);
}

void AudioChannel::PlaySample(size_t id, const float3& pos, const float3& velocity, float volume)
{
	FindSourceAndPlay(id, pos, velocity, volume, false);
}

void AudioChannel::PlaySample(size_t id, const CUnit* unit, float volume)
{
	FindSourceAndPlay(id, unit->pos, unit->speed, volume, false);
}

void AudioChannel::PlaySample(size_t id, const CWorldObject* obj, float volume)
{
	FindSourceAndPlay(id, obj->pos, ZeroVector, volume, false);
}

void AudioChannel::StreamPlay(const std::string& filepath, float volume, bool enqueue)
{
	if (!enabled)
		return;

	CSoundSource *newStreamSrc = NULL;

	if (!curStreamSrc) // this is kept outside the mutex, to avoid deadlocks
		newStreamSrc = sound->GetNextBestSource(); //! may return 0 if no sources available

	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc && enqueue) {
		if (streamQueue.size() > MAX_STREAM_QUEUESIZE) {
			streamQueue.resize(MAX_STREAM_QUEUESIZE);
			streamQueue.pop_back(); //! make room for the new item
		}
		StreamQueueItem newItem(filepath, volume);
		streamQueue.push_back(newItem);
		return;
	}

	if (!curStreamSrc && newStreamSrc)
		curStreamSrc = newStreamSrc;

	if (curStreamSrc) {
		cur_sources[curStreamSrc] = true; //! This one first, PlayStream may invoke Stop immediately thus setting curStreamSrc to NULL
		curStreamSrc->PlayStream(this, filepath, volume);
	}
}

void AudioChannel::StreamPause()
{
	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc)
		curStreamSrc->StreamPause();
}

void AudioChannel::StreamStop()
{
	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc)
		curStreamSrc->StreamStop();
}

float AudioChannel::StreamGetTime()
{
	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc)
		return curStreamSrc->GetStreamTime();
	else
		return 0.0f;
}

float AudioChannel::StreamGetPlayTime()
{
	boost::recursive_mutex::scoped_lock lck(chanMutex);

	if (curStreamSrc)
		return curStreamSrc->GetStreamPlayTime();
	else
		return 0.0f;
}
