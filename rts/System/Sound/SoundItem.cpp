#include "SoundItem.h"

#include <sstream>
#include <stdexcept>

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
priority(0),
maxConcurrent(256),
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
	if ((it = items.find("in3d")) != items.end())
		StringConvert(it->second, in3D);
}
