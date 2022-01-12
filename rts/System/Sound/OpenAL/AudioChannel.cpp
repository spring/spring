/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AudioChannel.h"

#include "ALShared.h"
#include "SoundItem.h"
#include "SoundSource.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GuiSoundSet.h"
#include "Sim/Objects/WorldObject.h"
#include "System/Sound/ISound.h"
#include "System/Sound/SoundLog.h"
#include "System/Threading/SpringThreading.h"

#include <climits>

extern spring::recursive_mutex soundMutex;



void AudioChannel::SetVolume(float newVolume)
{
	volume = std::max(newVolume, 0.0f);

	if (curSources.empty())
		return;

	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	for (CSoundSource* src: curSources) {
		src->UpdateVolume();
	}

	CheckError("AudioChannel::SetVolume");
}


void AudioChannel::Enable(bool newState)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if ((enabled = newState))
		return;

	SetVolume(0.0f);
}


void AudioChannel::SoundSourceFinished(CSoundSource* sndSource)
{
	if (curStreamSrc == sndSource) {
		if (!streamQueue.empty()) {
			StreamPlay(streamQueue.front(), false);
			streamQueue.pop_front();
		} else {
			curStreamSrc = nullptr;
		}
	}

	curSources.erase(sndSource);
}


void AudioChannel::FindSourceAndPlay(size_t id, const float3& pos, const float3& velocity, float volume, bool relative)
{
	if (id == 0 || volume <= 0.0f)
		return;

	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (!enabled)
		return;

	// get the sound item, then find a source for it
	const SoundItem* sndItem = sound->GetSoundItem(id);

	if (sndItem == nullptr) {
		sound->numEmptyPlayRequests++;
		return;
	}

	// check distance to listener
	if (pos.distance(sound->GetListenerPos()) > sndItem->MaxDistance()) {
		if (!relative)
			return;

		LOG("[AudioChannel::%s] maximum distance ignored for relative playback of sound-item \"%s\"", __func__, (sndItem->Name()).c_str());
	}

	// don't spam to many sounds per frame
	if (emitsThisFrame >= emitsPerFrame)
		return;

	emitsThisFrame++;

	// check if the sound item is already played
	if (curSources.size() >= maxConcurrentSources) {
		CSoundSource* src = nullptr;

		int prio = INT_MAX;

		for (CSoundSource* tmp: curSources) {
			if (tmp->GetCurrentPriority() < prio) {
				src  = tmp;
				prio = src->GetCurrentPriority();
			}
		}

		if (src == nullptr || prio > sndItem->GetPriority()) {
			LOG_L(L_DEBUG, "[AudioChannel::%s] maximum concurrent playbacks reached for sound-item %s", __func__, (sndItem->Name()).c_str());
			return;
		}

		src->Stop();
	}

	// find a sound source to play the item in
	CSoundSource* sndSource = sound->GetNextBestSource();

	if (sndSource == nullptr || (sndSource->GetCurrentPriority() >= sndItem->GetPriority())) {
		LOG_L(L_DEBUG, "[AudioChannel::%s] no source found for sound-item %s", __func__, (sndItem->Name()).c_str());
		return;
	}
	if (sndSource->IsPlaying())
		sound->numAbortedPlays++;

	// play the sound item
	sndSource->PlayAsync(this, id, pos, velocity, volume, sndItem->GetPriority(), relative);
	curSources.insert(sndSource);
}


void AudioChannel::PlaySample(size_t id, float volume)
{
	FindSourceAndPlay(id, -FwdVector, ZeroVector, volume, true);
}

void AudioChannel::PlaySample(size_t id, const float3& pos, float volume)
{
	FindSourceAndPlay(id, pos, ZeroVector, volume, false);
}

void AudioChannel::PlaySample(size_t id, const float3& pos, const float3& velocity, float volume)
{
	FindSourceAndPlay(id, pos, velocity, volume, false);
}

void AudioChannel::PlaySample(size_t id, const CWorldObject* obj, float volume)
{
	FindSourceAndPlay(id, obj->pos, obj->speed, volume, false);
}


void AudioChannel::PlayRandomSample(const GuiSoundSet& soundSet, const CWorldObject* obj) { PlayRandomSample(soundSet, obj->pos, obj->speed); }
void AudioChannel::PlayRandomSample(const GuiSoundSet& soundSet, const float3& pos, const float3& vel)
{
	int soundIdx = -1;

	switch (soundSet.NumSounds()) {
		case  0: {                                         return; } break;
		case  1: { soundIdx =                                   0; } break;
		default: { soundIdx = guRNG.NextInt(soundSet.NumSounds()); } break;
	}

	FindSourceAndPlay(soundSet.getID(soundIdx), pos, vel, soundSet.getVolume(soundIdx), false);
}


void AudioChannel::StreamPlay(const std::string& filepath, float volume, bool enqueue)
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (!enabled)
		return;

	if (curStreamSrc != nullptr && enqueue) {
		if (streamQueue.size() > MAX_STREAM_QUEUESIZE) {
			streamQueue.resize(MAX_STREAM_QUEUESIZE);
			streamQueue.pop_back(); // make room for the new item
		}

		streamQueue.emplace_back(filepath, volume);
		return;
	}

	if (curStreamSrc == nullptr)
		curStreamSrc = sound->GetNextBestSource(); // may return 0 if no sources available

	if (curStreamSrc == nullptr)
		return;

	// insert first, PlayStream may invoke Stop immediately thus setting curStreamSrc to NULL
	curSources.insert(curStreamSrc);
	curStreamSrc->PlayStream(this, filepath, volume);
}

void AudioChannel::StreamPause()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (curStreamSrc != nullptr)
		curStreamSrc->StreamPause();
}

void AudioChannel::StreamStop()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (curStreamSrc != nullptr)
		curStreamSrc->StreamStop();
}

float AudioChannel::StreamGetTime()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (curStreamSrc != nullptr)
		return curStreamSrc->GetStreamTime();

	return 0.0f;
}

float AudioChannel::StreamGetPlayTime()
{
	std::lock_guard<spring::recursive_mutex> lck(soundMutex);

	if (curStreamSrc != nullptr)
		return curStreamSrc->GetStreamPlayTime();

	return 0.0f;
}
