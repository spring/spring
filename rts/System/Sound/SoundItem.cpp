#include "SoundItem.h"

#include <sstream>
#include <stdexcept>
#include <cfloat>

#include "SoundBuffer.h"

namespace
{
template <typename T>
inline bool MapEntryValExtract(const std::map<std::string, std::string> map, const std::string& key, T& t)
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
}

SoundItem::SoundItem(boost::shared_ptr<SoundBuffer> _buffer, const std::map<std::string, std::string>& items) :
buffer(_buffer),
gain(1.0),
pitch(1.0),
dopplerScale(1.0),
maxDist(FLT_MAX),
rolloff(1.0f),
priority(0),
maxConcurrent(16),
currentlyPlaying(0),
loopTime(0),
in3D(true)
{
	if (!MapEntryValExtract(items, "name", name))
		name = buffer->GetFilename();

	MapEntryValExtract(items, "gain", gain);
	MapEntryValExtract(items, "pitch", pitch);
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

