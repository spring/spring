/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AudioChannel.h"

#include "ALShared.h"
#include "ISound.h"
#include "SoundItem.h"
#include "SoundLog.h"
#include "SoundSource.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"

const size_t AudioChannel::MAX_STREAM_QUEUESIZE = 10;


AudioChannel::AudioChannel()
	: curStreamSrc(NULL)
{
}


void AudioChannel::SetVolume(float newVolume)
{
	volume = newVolume;

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
	if (curStreamSrc == sndSource) {
		if (streamQueue.size()>0) {
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
	//boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (!enabled)
		return;

	if (volume <= 0.0f)
		return;

	SoundItem* sndItem = sound->GetSoundItem(id);
	if (!sndItem)
	{
		sound->numEmptyPlayRequests++;
		return;
	}

	if (pos.distance(sound->GetListenerPos()) > sndItem->MaxDistance())
	{
		if (!relative)
			return;
		else
			LogObject(LOG_SOUND) << "CSound::PlaySample: maxdist ignored for relative payback: " << sndItem->Name();
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
		CheckError("CSound::PlaySample");

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

	if (curStreamSrc && enqueue) {
		if (streamQueue.size() > MAX_STREAM_QUEUESIZE) {
			streamQueue.resize(MAX_STREAM_QUEUESIZE);
			streamQueue.pop_back(); //! make room for the new item
		}
		StreamQueueItem newItem(filepath, volume);
		streamQueue.push_back(newItem);
		return;
	}

	if (!curStreamSrc)
		curStreamSrc = sound->GetNextBestSource(); //! may return 0 if no sources available

	if (curStreamSrc) {
		curStreamSrc->PlayStream(this, filepath, volume);
		cur_sources[curStreamSrc] = true;
	}
}

void AudioChannel::StreamPause()
{
	if (curStreamSrc)
		curStreamSrc->StreamPause();
}

void AudioChannel::StreamStop()
{
	if (curStreamSrc)
		curStreamSrc->StreamStop();
}

float AudioChannel::StreamGetTime()
{
	if (curStreamSrc)
		return curStreamSrc->GetStreamTime();
	else
		return 0.0f;
}

float AudioChannel::StreamGetPlayTime()
{
	if (curStreamSrc)
		return curStreamSrc->GetStreamPlayTime();
	else
		return 0.0f;
}
