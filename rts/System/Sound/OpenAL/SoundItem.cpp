/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SoundItem.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cfloat>

#include "SoundBuffer.h"
#include "System/GlobalRNG.h"

namespace
{
	CGlobalUnsyncedRNG randnum; // no need for strong randomness here, so default seed is ok

	template <typename T>
	inline bool MapEntryValExtract(const spring::unordered_map<std::string, std::string>& map, const std::string& key, T& t)
	{
		auto it = map.find(key);
		if (it != map.end()) {
			std::istringstream stream(it->second);
			stream >> t;
			return true;
		}

		return false;
	}

	template <typename T>
	void FitInIntervall(const T& lower, T& val, const T& upper)
	{
		val = std::max(std::min(val, upper), lower);
	}
}

SoundItem::SoundItem(size_t itemID, size_t bufferID, const spring::unordered_map<std::string, std::string>& items)
	: soundItemID(itemID)
	, soundBufferID(bufferID)
	, maxDist(FLT_MAX)
{
	if (!MapEntryValExtract(items, "name", name))
		name = (SoundBuffer::GetById(bufferID)).GetFilename();

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
	if (maxConcurrent >= currentlyPlaying) {
		currentlyPlaying++;
		return true;
	}

	return false;
}

void SoundItem::StopPlay()
{
	assert(currentlyPlaying > 0);
	--currentlyPlaying;
}

float SoundItem::GetGain() const
{
	float tgain = 0.0f;

	if (gainMod > 0.0f)
		tgain = (float(randnum(200)) / 100.0f - 1.0f) * gainMod;

	return gain * (1.0f + tgain);
}

float SoundItem::GetPitch() const
{
	float tpitch = 0.0f;

	if (pitchMod > 0.0f)
		tpitch = (float(randnum(200)) / 100.0f - 1.0f) * pitchMod;

	return pitch * (1.0f + tpitch);
}

