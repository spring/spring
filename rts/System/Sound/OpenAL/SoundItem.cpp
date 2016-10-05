/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SoundItem.h"

#include <sstream>
#include <stdexcept>
#include <cfloat>

#include "SoundBuffer.h"
#include "System/UnsyncedRNG.h"

namespace
{
	UnsyncedRNG randnum; // no need for strong randomness here, so default seed is ok

	template <typename T>
	inline bool MapEntryValExtract(const std::map<std::string, std::string>& map, const std::string& key, T& t)
	{
		std::map<std::string, std::string>::const_iterator it = map.find(key);
		if (it != map.end())
		{
			std::istringstream stream(it->second);
			stream >> t;
			return true;
		}
		else
			return false;
	}

	template <typename T>
	void FitInIntervall(const T& lower, T& val, const T& upper)
	{
		val = std::max(std::min(val, upper), lower);
	}
}

SoundItem::SoundItem(std::shared_ptr<SoundBuffer> _buffer, const std::map<std::string, std::string>& items)
	: buffer(_buffer)
	, gain(1.0)
	, gainMod(0)
	, pitch(1.0)
	, pitchMod(0)
	, dopplerScale(1.0)
	, maxDist(FLT_MAX)
	, rolloff(1.0f)
	, priority(0)
	, maxConcurrent(16)
	, currentlyPlaying(0)
	, loopTime(0)
	, in3D(true)
{
	if (!MapEntryValExtract(items, "name", name))
		name = buffer->GetFilename();

	MapEntryValExtract(items, "gain", gain);
	MapEntryValExtract(items, "gainmod", gainMod);
	FitInIntervall(0.f, gainMod, 1.f);
	MapEntryValExtract(items, "pitch", pitch);
	MapEntryValExtract(items, "pitchmod", pitchMod);
	FitInIntervall(0.f, pitchMod, 1.f);
	MapEntryValExtract(items, "dopplerscale", dopplerScale);
	MapEntryValExtract(items, "priority", priority);
	MapEntryValExtract(items, "maxconcurrent", maxConcurrent);
	MapEntryValExtract(items, "maxdist", maxDist);
	MapEntryValExtract(items, "rolloff", rolloff);
	MapEntryValExtract(items, "in3d", in3D);
	MapEntryValExtract(items, "looptime", loopTime);
}

bool SoundItem::PlayNow()
{
	if (maxConcurrent >= currentlyPlaying)
	{
		currentlyPlaying++;
		return true;
	}
	else
	{
		return false;
	}
}

void SoundItem::StopPlay()
{
	assert(currentlyPlaying > 0);
	--currentlyPlaying;
}

float SoundItem::GetGain() const
{
	float tgain = 0;
	if (gainMod > 0)
		tgain = (float(randnum(200))/100.f - 1.f)*gainMod;
	return gain * (1.f + tgain);
}

float SoundItem::GetPitch() const
{
	float tpitch = 0;
	if (pitchMod > 0)
		tpitch = (float(randnum(200))/100.f - 1.f)*pitchMod;
	return pitch * (1.f + tpitch);
}
