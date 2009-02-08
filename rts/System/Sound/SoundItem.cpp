#include "SoundItem.h"

#include <sstream>
#include <stdexcept>
#include <cfloat>

#include "SoundBuffer.h"

namespace
{
template<typename T>
void StringConvert(const std::string& input, T& t)
{
	std::istringstream stream(input);
	stream >> t;
}
}

SoundItem::SoundItem(boost::shared_ptr<SoundBuffer> _buffer, const std::map<std::string, std::string>& items) :
buffer(_buffer),
gain(1.0),
pitch(1.0),
maxDist(FLT_MAX),
priority(0),
maxConcurrent(16),
currentlyPlaying(0),
in3D(true)
{
	std::map<std::string, std::string>::const_iterator it = items.find("name");

	it = items.find("name");
	if (it != items.end())
		name = it->second;
	else
		name = buffer->GetFilename();

	if ((it = items.find("gain")) != items.end())
		StringConvert(it->second, gain);
	if ((it = items.find("pitch")) != items.end())
		StringConvert(it->second, pitch);
	if ((it = items.find("priority")) != items.end())
		StringConvert(it->second, priority);
	if ((it = items.find("maxconcurrent")) != items.end())
		StringConvert(it->second, maxConcurrent);
	if ((it = items.find("maxdist")) != items.end())
		StringConvert(it->second, maxDist);
	if ((it = items.find("in3d")) != items.end())
		StringConvert(it->second, in3D);
	if ((it = items.find("looptime")) != items.end())
		StringConvert(it->second, loopTime);
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

