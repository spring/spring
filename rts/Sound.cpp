// Sound.cpp: implementation of the CSound class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Sound.h"

#ifndef NO_SOUND
#ifdef USE_DSOUND
 	#include "WavRead.h"
#else
	#ifndef _WINSOCKAPI_
		#define _WINSOCKAPI_
		#include <windows.h>
		#undef _WINSOCKAPI_
	#else
		#include <windows.h>
	#endif
#endif

#include <math.h>
#include "InfoConsole.h"
#include "RegHandler.h"
#include "Camera.h"
#include "WorldObject.h"
#include "InfoConsole.h"
//#include "mmgr.h"

extern HWND	hWnd;			// Holds Our Window Handle

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

#endif //NO_SOUND


CSound* sound;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/*void PlaySound(int num,bool loop,bool reset,CWorldObject* p)
{
	return;
};
*/
CSound::CSound()
{
#ifndef NO_SOUND
//	noSound=true;
	maxSounds=regHandler.GetInt("MaxSounds",16);
	curThreshhold=0.1;
	wantedSounds=maxSounds*0.75;
#ifdef USE_DSOUND
	m_pDS            = NULL;
	m_pWaveSoundRead = NULL;

	HRESULT             hr;
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;
	
	// Initialize COM
	hr = CoInitialize( NULL );
	if( hr!=S_OK && hr!=S_FALSE){
		info->AddLine("Couldnt initialize com");
		noSound=true;
		return;
	}
	
	// Create IDirectSound using the primary sound device
	if( FAILED( hr = DirectSoundCreate( NULL, &m_pDS, NULL ) ) ){
		info->AddLine("Couldnt create direct sound object");		
		noSound=true;
		return;
	}

    // Set coop level to DSSCL_PRIORITY
	if( FAILED( hr = m_pDS->SetCooperativeLevel( hWnd, DSSCL_PRIORITY ) ) ){
		info->AddLine("Couldnt set cooperative level");
		noSound=true;
		return;
	}
	
	// Get the primary buffer 
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat   = NULL;
	
	if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) ){
		info->AddLine("Couldnt create primary sound buffer");
		noSound=true;
		return;
	}
	
	// Set primary buffer format to 22kHz and 16-bit output.
	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
	wfx.wFormatTag      = WAVE_FORMAT_PCM; 
	wfx.nChannels       = 2; 
	wfx.nSamplesPerSec  = 22050; 
	wfx.wBitsPerSample  = 16; 
	wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	
	if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) ){
		info->AddLine("Couldnt initialize primary sound format");
		noSound=true;
		return;
	}
	
	SAFE_RELEASE( pDSBPrimary );
	noSound=false;
	waveid[""]=0;
	SoundInfo* si=new SoundInfo;
	loadedSounds.push_back(si);
#endif
#endif //NO_SOUND
}

CSound::~CSound()
{
#ifdef USE_DSOUND
	SAFE_DELETE( m_pWaveSoundRead );

    // Release DirectSound interfaces
	while(!buffers.empty()){
		SAFE_RELEASE( buffers.back());
		buffers.pop_back();
	}
  SAFE_RELEASE( m_pDS ); 

	for(vector<SoundInfo*>::iterator si=loadedSounds.begin();si!=loadedSounds.end();++si)
		delete *si;

	// Release COM
	CoUninitialize();
#endif
}

#ifdef USE_DSOUND
int CSound::InitFile(const string& name)
{
	if((m_pDS==0) || noSound)
		return -1;

	// Create the sound buffer object from the wave file data
	if( FAILED( CreateStaticBuffer(name.c_str() ) ) )
	{        
		MessageBox(0,"Couldnt create sound buffer","Sound error",0);
		noSound=true;
		return -1;
	}
	else // The sound buffer was successfully created
	{
		// Fill the buffer with wav data
		FillBuffer();
	}

	waveid[name]=loadedSounds.size();
	buf2id.push_back(loadedSounds.size());
	SoundInfo* si=new SoundInfo;
	si->firstBuf=buffers.size()-1;
	si->freebufs.push_back(buffers.size()-1);
	loadedSounds.push_back(si);

	return buffers.size()-1;
}
#endif //NO_SOUND

int CSound::GetWaveId(const string &name)
{
#ifndef NO_SOUND
	PUSH_CODE_MODE;
	ENTER_MIXED;
	if(noSound){
		POP_CODE_MODE;
		return 0;
	}
	map<string,int>::iterator si;
	if((si=waveid.find(name))==waveid.end()){
		InitFile(name);
		si=waveid.find(name);
	}
	int ret=si->second;
	POP_CODE_MODE;
	return ret;
#endif //NO_SOUND
}

#ifndef NO_SOUND
int CSound::GetBuf(int id,float volume)
{
	SoundInfo* s=loadedSounds[id];
	int num;
	if(s->freebufs.empty()){
		LPDIRECTSOUNDBUFFER b=NULL;
		buffers.push_back(b);
		HRESULT r=m_pDS->DuplicateSoundBuffer(buffers[s->firstBuf],&(buffers.back()));
		if(r!=DS_OK){
			MessageBox(0,"Couldnt duplicate sound buffer","Sound error",0);
			noSound=true;
			return -1;
		}
		buf2id.push_back(id);
		s->freebufs.push_back(buffers.size()-1);
	}
	num=s->freebufs.back();
	s->freebufs.pop_back();
	PlayingSound ps;
	ps.num=num;
	ps.volume=volume;
	playingSounds.push_back(ps);

	return num;
}
#endif

#ifdef USE_DSOUND
void CSound::PlaySound(int id)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	if(noSound || id<=0 || id>=loadedSounds.size() || playingSounds.size()>=maxSounds){
		POP_CODE_MODE;
		return;
	}

	float v=0;
	HRESULT hr;
	int num=GetBuf(id,v);

	// Restore the buffers if they are lost
	if( FAILED( hr = RestoreBuffers(num) ) ){
		POP_CODE_MODE;
		return;
	}
	
	buffers[num]->SetVolume(DSBVOLUME_MIN*v-100);
	buffers[num]->SetPan(0);
//	if(reset)
//		buffers[num]->SetCurrentPosition( 0L );    
	
	// Play buffer 
//	DWORD dwLooped = loop ? DSBPLAY_LOOPING : 0L;
	if( FAILED( hr = buffers[num]->Play( 0, 0, 0/*dwLooped*/ ) ) ){

	}
	POP_CODE_MODE;
}

void CSound::PlaySound(int id,CWorldObject* p,float volume)
{
	PlaySound(id,p->pos,volume);
}

void CSound::PlaySound(int id,const float3& p,float volume)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	if(noSound || id<=0 || id>=loadedSounds.size()){
		POP_CODE_MODE;
		return;
	}

	HRESULT hr;
	float3 dif=p - camera->pos;
	float dl=dif.Length();
	float pan=dif.dot(camera->right)*DSBPAN_RIGHT/dl;
	float v=0;
	if(volume!=0)
		v=dl/(volume*1000);
	if(v>0.6){
		POP_CODE_MODE;
		return;
	}

//	info->AddLine("%i %i %f",maxSounds,playingSounds.size(),v);

	if(v>curThreshhold+(wantedSounds-playingSounds.size())/wantedSounds){
		POP_CODE_MODE;
		return;
	}

	int num=GetBuf(id,v);

	// Restore the buffers if they are lost
	if( FAILED( hr = RestoreBuffers(num) ) )
		return;
	
	buffers[num]->SetVolume(DSBVOLUME_MIN*v-100);
	buffers[num]->SetPan(pan);

	if( FAILED( hr = buffers[num]->Play( 0, 0, 0/*dwLooped*/ ) ) ){

	}
	POP_CODE_MODE;
}
#else
void CSound::PlaySound(int id){};
void CSound::PlaySound(int id,CWorldObject* p,float volume){};
void CSound::PlaySound(int id,const float3& p,float volume){};
#endif

#ifdef USE_DSOUND
HRESULT CSound::CreateStaticBuffer(const string& name)
{
	string filename("sounds\\"+name);
	HRESULT hr; 
	
	// Free any previous globals 
	SAFE_DELETE( m_pWaveSoundRead );
	
	LPDIRECTSOUNDBUFFER b=NULL;
	buffers.push_back(b);
	buffers.back()=0;
	
	// Create a new wave file class
	m_pWaveSoundRead = new CWaveSoundRead();
	
	// Load the wave file
	if( FAILED( m_pWaveSoundRead->Open( filename.c_str() ) ) )
	{
		MessageBox(0,"Bad wave file.",filename.c_str(),0);
	}
	
	// Set up the direct sound buffer, and only request the flags needed
	// since each requires some overhead and limits if the buffer can 
	// be hardware accelerated
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_STATIC | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = m_pWaveSoundRead->m_ckIn.cksize;
	dsbd.lpwfxFormat   = m_pWaveSoundRead->m_pwfx;
	
	// Create the static DirectSound buffer 
	if( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &buffers.back(), NULL ) ) )
		return hr;
	
	// Remember how big the buffer is
	m_dwBufferBytes = dsbd.dwBufferBytes;
	
	return S_OK;
	
	return 0;
}

HRESULT CSound::FillBuffer()
{
	HRESULT hr; 
	BYTE*   pbWavData; // Pointer to actual wav data 
	UINT    cbWavSize; // Size of data
	VOID*   pbData  = NULL;
	VOID*   pbData2 = NULL;
	DWORD   dwLength;
	DWORD   dwLength2;

	// The size of wave data is in pWaveFileSound->m_ckIn
	INT nWaveFileSize = m_pWaveSoundRead->m_ckIn.cksize;

	// Allocate that buffer.
	pbWavData = new BYTE[ nWaveFileSize ];
	if( NULL == pbWavData )
		return E_OUTOFMEMORY;

	if( FAILED( hr = m_pWaveSoundRead->Read( nWaveFileSize, 
		pbWavData, 
		&cbWavSize ) ) )           
		return hr;

	// Reset the file to the beginning 
	m_pWaveSoundRead->Reset();

	// Lock the buffer down
	if( FAILED( hr = buffers.back()->Lock( 0, m_dwBufferBytes, &pbData, &dwLength, 
		&pbData2, &dwLength2, 0L ) ) )
		return hr;

	// Copy the memory to it.
	memcpy( pbData, pbWavData, m_dwBufferBytes );

	// Unlock the buffer, we don't need it anymore.
	buffers.back()->Unlock( pbData, m_dwBufferBytes, NULL, 0 );
	pbData = NULL;

	// We dont need the wav file data buffer anymore, so delete it 
	delete[] pbWavData ;

	return S_OK;

	return 0;
}

HRESULT CSound::RestoreBuffers(int num)
{
	
    HRESULT hr;

    if( NULL == buffers[num] )
        return S_OK;

    DWORD dwStatus;
    if( FAILED( hr = buffers[num]->GetStatus( &dwStatus ) ) )
        return hr;

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do 
        {
            hr = buffers[num]->Restore();
            if( hr == DSERR_BUFFERLOST )
                SleepEx(10, true);
        }
        while( hr = buffers[num]->Restore() );

        if( FAILED( hr = FillBuffer() ) )
            return hr;
    }

    return S_OK;

}
#endif

void CSound::Update()
{
#ifndef NO_SOUND
	float total=wantedSounds*0.5;
	for(std::list<PlayingSound>::iterator pi=playingSounds.begin();pi!=playingSounds.end();){
		int num=pi->num;
		DWORD status;
		buffers[num]->GetStatus(&status);
		if(!status & DSBSTATUS_PLAYING){
			pi=playingSounds.erase(pi);
			loadedSounds[buf2id[num]]->freebufs.push_back(num);
		} else {
			total-=(0.5-pi->volume);
			++pi;
		}
	}
	total/=wantedSounds;
	curThreshhold=(curThreshhold+total)*0.5;
//	info->AddLine("curt %.2f",curThreshhold);
#endif //NO_SOUND
}


