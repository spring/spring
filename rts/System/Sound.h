#ifndef SOUNDSYSTEM_INTERFACE_H
#define SOUNDSYSTEM_INTERFACE_H

class CWorldObject;

// Abstract sound system interface
class ISound
{
public:
	ISound() {}
	virtual ~ISound() {}

	virtual unsigned int GetWaveId(const std::string& path) = 0;
	virtual void Update() = 0;
	virtual void PlaySound(int id, float volume=1) = 0;
	virtual void PlaySound(int id,CWorldObject* p,float volume=1) = 0;
	virtual void PlaySound(int id,const float3& p,float volume=1) = 0;

// TODO: fix this, noSound is modified in game code to toggle sound
// but OpenALSound assumes it is constant
	bool noSound; 
};

extern ISound* sound; 

#endif
