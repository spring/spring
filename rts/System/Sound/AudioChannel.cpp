/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AudioChannel.h"

#include "ALShared.h"
#include "ISound.h"
#include "SoundItem.h"
#include "SoundLog.h"
#include "SoundSource.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "Sim/Objects/WorldObject.h"
#include "Sim/Units/Unit.h"

#include <climits>

extern boost::recursive_mutex soundMutex;

const size_t AudioChannel::MAX_STREAM_QUEUESIZE = 10;


AudioChannel::AudioChannel()
	: curStreamSrc(NULL)
{
}


void AudioChannel::SetVolume(float newVolume)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	volume = newVolume;

	for (std::map<CSoundSource*, bool>::iterator it = cur_sources.begin(); it != cur_sources.end(); ++it) {
		it->first->UpdateVolume();
	}
}


void AudioChannel::Enable(bool newState)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	enabled = newState;

	if (!enabled) {
		SetVolume(0.f);
	}
}


void AudioChannel::SoundSourceFinished(CSoundSource* sndSource)
{
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
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (!enabled)
		return;

	if (volume <= 0.0f)
		return;

	SoundItem* sndItem = sound->GetSoundItem(id);
	if (!sndItem) {
		sound->numEmptyPlayRequests++;
		return;
	}

	if (pos.distance(sound->GetListenerPos()) > sndItem->MaxDistance()) {
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

	if (cur_sources.size() >= maxConcurrentSources) {
		CSoundSource* src = NULL;
		int prio = INT_MAX;
		for (std::map<CSoundSource*, bool>::iterator it = cur_sources.begin(); it != cur_sources.end(); ++it) {
			if (it->first->GetCurrentPriority() < prio) {
				src  = it->first;
				prio = it->first->GetCurrentPriority();
			}
		}

		if (src && prio <= sndItem->GetPriority()) {
			src->Stop();
		} else {
			LOG_L(L_DEBUG, "CSound::PlaySample: Max concurrent sounds in channel reached! Dropping playback!");
			return;
		}
	}

	CSoundSource* sndSource = sound->GetNextBestSource();
	if (!sndSource) {
		LOG_L(L_DEBUG, "CSound::PlaySample: Max sounds reached! Dropping playback!");
		return;
	}

	if (sndSource->GetCurrentPriority() < sndItem->GetPriority()) {
		if (sndSource->IsPlaying())
			sound->numAbortedPlays++;

		sndSource->Play(this, sndItem, pos, velocity, volume, relative);
		CheckError("CSound::FindSourceAndPlay");

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


void AudioChannel::PlayRandomSample(const GuiSoundSet& soundSet, const CUnit* unit)
{
	PlayRandomSample(soundSet, unit->pos);
}

void AudioChannel::PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos)
{
	const int soundIdx = soundSet.getRandomIdx();

	if (soundIdx < 0)
		return;

	const int soundID = soundSet.getID(soundIdx);
	const float soundVol = soundSet.getVolume(soundIdx);

	PlaySample(soundID, pos, soundVol);
}


void AudioChannel::StreamPlay(const std::string& filepath, float volume, bool enqueue)
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

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
		cur_sources[curStreamSrc] = true; //! This one first, PlayStream may invoke Stop immediately thus setting curStreamSrc to NULL
		curStreamSrc->PlayStream(this, filepath, volume);
	}
}

void AudioChannel::StreamPause()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (curStreamSrc)
		curStreamSrc->StreamPause();
}

void AudioChannel::StreamStop()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (curStreamSrc)
		curStreamSrc->StreamStop();
}

float AudioChannel::StreamGetTime()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (curStreamSrc)
		return curStreamSrc->GetStreamTime();
	else
		return 0.0f;
}

float AudioChannel::StreamGetPlayTime()
{
	boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (curStreamSrc)
		return curStreamSrc->GetStreamPlayTime();
	else
		return 0.0f;
}
