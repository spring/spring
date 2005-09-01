#ifndef SOUND_H
#define SOUND_H
// Sound.h: interface for the CSound class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SOUND_H__D6141CC1_8294_11D4_AD55_0080ADA84DE3__INCLUDED_)
#define AFX_SOUND_H__D6141CC1_8294_11D4_AD55_0080ADA84DE3__INCLUDED_

#ifndef NO_SOUND

#pragma warning(disable:4786)

#define USE_DSOUND

#if _MSC_VER > 1000
/*pragma once removed*/
#endif // _MSC_VER > 1000

#ifdef USE_DSOUND
	#ifndef _WINSOCKAPI_
		#define _WINSOCKAPI_
		#include <dsound.h>
		#undef _WINSOCKAPI_
	#else
		#include <dsound.h>
	#endif
#endif


#endif //NO_SOUND


#include <string>
#include <vector>
#include <map>
#include <deque>


class CWorldObject;
class CWaveSoundRead;

using namespace std;
class CSound  
{
public:
	CSound();
	virtual ~CSound();
	void Update();
	int GetWaveId(const string& name);
	void PlaySound(int id,float volume=1);
	void PlaySound(int id,CWorldObject* p,float volume=1);
	void PlaySound(int id,const float3& p,float volume=1);

	bool noSound;
	int maxSounds;

	float curThreshhold;
	float wantedSounds;
#ifdef USE_DSOUND
protected:
	int GetBuf(int id,float volume);
	int InitFile(const string& name);

	struct SoundInfo {
		vector<int> freebufs;
		int firstBuf;
	};
	struct PlayingSound {
		int num;
		float volume;
	};

	map<string,int> waveid;
	vector<SoundInfo*> loadedSounds;
	list<PlayingSound> playingSounds;
	vector<int> buf2id;

	std::vector<LPDIRECTSOUNDBUFFER> buffers;

	Sint32 RestoreBuffers(int num);
	Sint32 CreateStaticBuffer(const string& name);
	Sint32 FillBuffer();

	LPDIRECTSOUND		m_pDS;
	CWaveSoundRead*     m_pWaveSoundRead;
	Uint32               m_dwBufferBytes;
	LPDIRECTSOUNDNOTIFY g_pDSNotify;
#endif
};

extern CSound* sound;
#endif // !defined(AFX_SOUND_H__D6141CC1_8294_11D4_AD55_0080ADA84DE3__INCLUDED_)

#endif /* SOUND_H */
