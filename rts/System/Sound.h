#ifndef SOUND_H
#define SOUND_H

#include <vector>
#include <AL/al.h>
#include <AL/alc.h>

class CWorldObject;

using namespace std;
class CSound  
{
public:
	ALuint LoadALBuffer(string path);
	ALuint GetWaveId(string path);
	void Update();
	void PlaySound(int id, float volume=1);
	void PlaySound(int id,CWorldObject* p,float volume=1);
	void PlaySound(int id,const float3& p,float volume=1);
	bool noSound;
	int maxSounds;
	CSound();
	virtual ~CSound();
private:
	int cur;
	void UpdateListener();
	void Enqueue(ALuint src);
	vector<string> LoadedFiles;
	vector<ALuint> Buffers;
	ALuint *Sources;
	ALCdevice *device;
	ALCcontext *context;
};

extern CSound* sound;

#endif /* SOUND_H */
