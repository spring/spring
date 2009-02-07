#ifndef SOUNDITEM_H
#define SOUNDITEM_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class SoundBuffer;

class SoundItem
{
	friend class SoundSource;
public:
	SoundItem(boost::shared_ptr<SoundBuffer> buffer, const std::map<std::string, std::string>& items);
	
private:
	boost::shared_ptr<SoundBuffer> buffer;
	std::string name;
	std::string file;
	float gain;
	//float gainFWHM;
	float pitch;
	//float pitchFWHM;
	int priority;
	unsigned maxConcurrent;
	bool in3D;
};

#endif
