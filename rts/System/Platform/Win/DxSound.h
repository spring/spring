#ifndef DX_SOUND_SYSTEM_H
#define DX_SOUND_SYSTEM_H

#include "Sound.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef _WINSOCKAPI_
	#define _WINSOCKAPI_
	#include <dsound.h>
	#undef _WINSOCKAPI_
#else
	#include <dsound.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <list>
#include <SDL_types.h>

#include "OggStream.h"

class CWorldObject;
using namespace std;

class CDxSound: public CSound
{
public:
	CDxSound();
	virtual ~CDxSound();
	void Update();
	unsigned int GetWaveId(const string& path, bool hardFail);
	void PlaySample(int id, float volume = 1.0f);
	void PlaySample(int id, const float3& p, float volume = 1.0f);

	void PlayStream(const std::string& path, float volume = 1.0f,
					const float3& pos = ZeroVector, bool loop = false);
	void StopStream();
	void PauseStream();
	unsigned int GetStreamTime();
	void SetStreamVolume(float);

	void SetVolume(float v);

private:
	int maxSounds;
	float curThreshhold;
	float wantedSounds;
	bool hardFail;

protected:
	int GetBuf(int id, float volume);
	int InitFile(const string& name);

	struct SoundInfo {
		vector<int> freebufs;
		int firstBuf;
	};
	struct PlayingSound {
		int num;
		float volume;
	};

	map<string, int> waveid;
	vector<SoundInfo*> loadedSounds;
	list<PlayingSound> playingSounds;
	vector<int> buf2id;
	float globalVolume;

	std::vector<LPDIRECTSOUNDBUFFER> buffers;

	HRESULT RestoreBuffers(int num);
	bool CreateStaticBuffer(const string& name);
	HRESULT FillBuffer();
	bool ReadWAV (const char* name, Uint8* buf, int fileSize, Uint8** soundData, Uint32* bufferSize, WAVEFORMATEX& wf);

	LPDIRECTSOUND		m_pDS;
	DWORD               m_dwBufferBytes;
	LPDIRECTSOUNDNOTIFY g_pDSNotify;
	HWND				m_hWnd;
};

#endif // DX_SOUND_SYSTEM_H
