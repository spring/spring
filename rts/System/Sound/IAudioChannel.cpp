/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IAudioChannel.h"

#include "ALShared.h"
#include "ISound.h"
#include "SoundItem.h"
#include "SoundLog.h"
#include "SoundSource.h"


const size_t IAudioChannel::MAX_STREAM_QUEUESIZE = 10;


IAudioChannel::IAudioChannel()
	: volume(1.0f)
	, enabled(true)
	, curStreamSrc(NULL)
{
}


void IAudioChannel::SetVolume(float newVolume)
{
	volume = newVolume;

	LogObject(LOG_SOUND) << "IAudioChannel::SetVolume";

	for (std::map<CSoundSource*, bool>::iterator it = cur_sources.begin(); it != cur_sources.end(); ++it)
		it->first->UpdateVolume();
}


void IAudioChannel::Enable(bool newState)
{
	enabled = newState;

	if (!enabled)
	{
		SetVolume(0.f);
	}
}


void IAudioChannel::SoundSourceFinished(CSoundSource* sndSource)
{
	if (curStreamSrc == sndSource) {
		if (streamQueue.size()>0) {
			StreamQueueItem& next = streamQueue.back();
			StreamPlay(next.filename, next.volume, false);
			streamQueue.pop_back();
		} else {
			curStreamSrc = NULL;
		}
	}

	cur_sources.erase(sndSource);
}


void IAudioChannel::FindSourceAndPlay(size_t id, const float3 &p, const float3& velocity, float volume, bool relative)
{
	//boost::recursive_mutex::scoped_lock lck(soundMutex);

	if (!enabled)
		return;

	if (volume <= 0.0f)
		return;

	SoundItem* sndItem = sound->GetSoundItem(id);
	if (!sndItem)
	{
		//numEmptyPlayRequests++;
		return;
	}

	/*if (p.distance(myPos) > sndItem->MaxDistance())
	{
		if (!relative)
			return;
		else
			LogObject(LOG_SOUND) << "CSound::PlaySample: maxdist ignored for relative payback: " << sndItem->Name();
	}*/

	CSoundSource* sndSource = sound->GetNextBestSource();
	if (!sndSource)
		return;

	if (sndSource->GetCurrentPriority() < sndItem->GetPriority())
	{
		//if (sndSource->IsPlaying())
		//	++numAbortedPlays;

		sndSource->Play(this, sndItem, p, velocity, volume, relative);
		CheckError("CSound::PlaySample");

		cur_sources[sndSource] = true;
	}
}


void IAudioChannel::StreamPlay(const std::string& filepath, float volume, bool enqueue)
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

void IAudioChannel::StreamPause()
{
	if (curStreamSrc)
		curStreamSrc->StreamPause();
}

void IAudioChannel::StreamStop()
{
	if (curStreamSrc)
		curStreamSrc->StreamStop();
}

float IAudioChannel::StreamGetTime()
{
	if (curStreamSrc)
		return curStreamSrc->GetStreamTime();
	else
		return 0;
}

float IAudioChannel::StreamGetPlayTime()
{
	if (curStreamSrc)
		return curStreamSrc->GetStreamPlayTime();
	else
		return 0;
}
