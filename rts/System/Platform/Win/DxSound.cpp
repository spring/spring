#include "StdAfx.h"

#ifndef _WINSOCKAPI_
	#define _WINSOCKAPI_
	#include <windows.h>
	#undef _WINSOCKAPI_
#else
	#include <windows.h>
#endif

#include "LogOutput.h"
#include "Platform/ConfigHandler.h"
#include "Game/Camera.h"
#include "Sim/Objects/WorldObject.h"
#include "DxSound.h"
#include "Platform/byteorder.h"
#include "Platform/errorhandler.h"
#include "Platform/NullSound.h"
#include "FileSystem/FileHandler.h"
#include "Exceptions.h"
#include <SDL_syswm.h>

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p) = NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }


// Ogg-Vorbis audio stream object
COggStream oggStream;
using namespace std;

CDxSound::CDxSound()
{
	maxSounds = ConfigHandler::GetInstance().GetInt("MaxSounds", 16);
	if (maxSounds <= 0) {
		throw content_error("Internal error, (maxSounds <= 0) in CDxSound");
	}

	curThreshhold = 0.1f;
	wantedSounds = maxSounds * 0.75f;
	globalVolume = 1.0f;

	m_pDS  = NULL;
	m_hWnd = NULL;

	HRESULT hr;

	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	// Get window from SDL
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);

	if (SDL_GetWMInfo (&wmInfo) != 1) {
		throw "DxSound: Could not get window from SDL";
	}
	m_hWnd = wmInfo.window;

	// Initialize COM
	hr = CoInitialize( NULL );
	if (hr != S_OK && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
		throw "DxSound: Could not initialize com";
	}

	// Create IDirectSound using the primary sound device
	if (FAILED(hr = DirectSoundCreate(NULL, &m_pDS, NULL))) {
		throw "DxSound: Could not create direct sound object";
	}

    // Set coop level to DSSCL_PRIORITY
	if (FAILED(hr = m_pDS->SetCooperativeLevel(m_hWnd, DSSCL_PRIORITY))) {
		throw "DxSound: Could not set cooperative level";
	}
	
	// Get the primary buffer
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));

	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat   = NULL;
	
	if (FAILED(hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, NULL))) {
		throw "DxSound: Could not create primary sound buffer";
	}
	
	// Set primary buffer format to 22kHz and 16-bit output.
	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
	wfx.wFormatTag      = WAVE_FORMAT_PCM;
	wfx.nChannels       = 2; 
	wfx.nSamplesPerSec  = 22050;
	wfx.wBitsPerSample  = 16;
	wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	
	if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) ){
		throw "DxSound: Could not initialize primary sound format";
	}
	
	SAFE_RELEASE(pDSBPrimary);
	waveid[""] = 0;
	SoundInfo* si = SAFE_NEW SoundInfo;
	loadedSounds.push_back(si);
}


CDxSound::~CDxSound()
{
    // Release DirectSound interfaces
	while (!buffers.empty()) {
		SAFE_RELEASE(buffers.back());
		buffers.pop_back();
	}
	SAFE_RELEASE(m_pDS);

	for (std::vector<SoundInfo*>::iterator si = loadedSounds.begin(); si != loadedSounds.end(); ++si)
		delete *si;

	// Release COM
	CoUninitialize();
}



void CDxSound::PlayStream(const std::string& path, float volume,
							const float3& pos, bool)
{
	oggStream.SetDSoundObject(m_pDS);
	oggStream.Play(path, volume, pos);
}

void CDxSound::StopStream() { oggStream.Stop(); }
void CDxSound::PauseStream() { oggStream.TogglePause(); }
unsigned int CDxSound::GetStreamTime() { return oggStream.GetPlayTime(); }
void CDxSound::SetStreamVolume(float v) { oggStream.SetVolume(v); }



int CDxSound::InitFile(const string& name)
{
	if (m_pDS == 0) {
		return -1;
	}

	// Create the sound buffer object from the wave file data
	if (!CreateStaticBuffer(name.c_str())) {
		logOutput << "no such sound: " << name.c_str() << "\n";
		return -1;
	}

	waveid[name] = loadedSounds.size();
	buf2id.push_back(loadedSounds.size());
	SoundInfo* si = SAFE_NEW SoundInfo;
	si->firstBuf = buffers.size() - 1;
	si->freebufs.push_back(buffers.size() - 1);
	loadedSounds.push_back(si);

	return (buffers.size() - 1);
}

unsigned int CDxSound::GetWaveId(const string &name, bool _hardFail)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	std::map<string, int>::iterator si = waveid.find(name);

	if (si == waveid.end()) {
		hardFail = _hardFail;
		InitFile(name);
		si = waveid.find(name);
	}

	int ret = (si != waveid.end())? si->second: 0;
	POP_CODE_MODE;
	return ret;
}

int CDxSound::GetBuf(int id, float volume)
{
	if (id <= 0 || id >= loadedSounds.size())
		return -1;

	SoundInfo* s = loadedSounds[id];
	int num;

	if (s->freebufs.empty()) {
		LPDIRECTSOUNDBUFFER b = NULL;
		buffers.push_back(b);
		HRESULT r = m_pDS->DuplicateSoundBuffer(buffers[s->firstBuf], &(buffers.back()));
		if (r != DS_OK) {
			MessageBox(0, "Couldn't duplicate sound buffer", "Sound error", 0);
			sound = SAFE_NEW CNullSound;
			delete this;
			return -2;
		}

		buf2id.push_back(id);
		s->freebufs.push_back(buffers.size() - 1);
	}

	num = s->freebufs.back();
	s->freebufs.pop_back();
	PlayingSound ps;
	ps.num = num;
	ps.volume = volume;
	playingSounds.push_back(ps);

	return num;
}

void CDxSound::SetVolume(float v)
{
	// place the volume within the 0.5 to 1.0 range
	if (v < 0.01f) {
		globalVolume = 0.0f;
	} else {
		globalVolume = 0.5f + (0.5f * max(0.0f, min(1.0f, v)));
	}
}



void CDxSound::PlaySample(int id, float volume)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;
	if (id <= 0 || id >= loadedSounds.size() || playingSounds.size() >= maxSounds) {
		POP_CODE_MODE;
		return;
	}

	if (volume == 0.0f || globalVolume == 0.0f) return;

	const float v = 1.0f - (globalVolume * max(0.0f, min(1.0f, volume)));
	int num = GetBuf(id, v);

	if (num == -2) {
		return; // shutting down
	}

	// Restore the buffers if they are lost
	HRESULT hr;
	if (FAILED( hr = RestoreBuffers(num))) {
		POP_CODE_MODE;
		return;
	}

	buffers[num]->SetVolume(int(DSBVOLUME_MIN * v));
	buffers[num]->SetPan(0);

	// Play buffer
	// DWORD dwLooped = loop? DSBPLAY_LOOPING: 0L;
	if (FAILED(hr = buffers[num]->Play(0, 0, 0/*dwLooped*/))) {
	}

	POP_CODE_MODE;
}

void CDxSound::PlaySample(int id, const float3& p, float volume)
{
	PUSH_CODE_MODE;
	ENTER_MIXED;

	if (id <= 0 || id >= loadedSounds.size()) {
		POP_CODE_MODE;
		return;
	}

	if (volume == 0.0f || globalVolume == 0.0f) return;

	float3 dif = p - camera->pos;
	float dl = dif.Length();
	float pan = dif.dot(camera->right) * DSBPAN_RIGHT / dl;
	float v = 0.0f;

	if (volume != 0.0f) {
		v = dl / ((globalVolume + 0.01f) * volume * 2000.0f);
	}

	if (v > 0.6f) {
		POP_CODE_MODE;
		return; // too quiet
	} else {
		v = max(v, (1.0f - globalVolume)); // clamp so that it isn't too loud
	}

	if (v > curThreshhold + (wantedSounds - playingSounds.size()) / wantedSounds) {
		POP_CODE_MODE;
		return;
	}

	int num = GetBuf(id, v);
	if (num == -2) {
		return; // shutting down
	}

	// Restore the buffers if they are lost
	HRESULT hr;
	if (FAILED( hr = RestoreBuffers(num)))
		return;

	buffers[num]->SetVolume(int(DSBVOLUME_MIN * v - 100));
	buffers[num]->SetPan(int(pan));

	if (FAILED(hr = buffers[num]->Play(0, 0, 0/*dwLooped*/))) {
	}

	POP_CODE_MODE;
}




#pragma pack(push, 1)

// Header copied from WavLib by Michael McTernan
struct WAVHeader
{
	Uint8 riff[4];         // "RIFF"
	Sint32 totalLength;
	Uint8 wavefmt[8];      // WAVEfmt "
	Sint32 length;         // Remaining length 4 bytes
	Sint16 format_tag;
	Sint16 channels;       // Mono=1 Stereo=2
	Sint32 SamplesPerSec;
	Sint32 AvgBytesPerSec;
	Sint16 BlockAlign;
	Sint16 BitsPerSample;
	Uint8 data[4];         // "data"
	Sint32 datalen;        // Raw data length 4 bytes
};

#pragma pack(pop)

bool CDxSound::ReadWAV (const char *name, Uint8 *buf, int fileSize, Uint8 **soundData, Uint32* bufferSize, WAVEFORMATEX& wf)
{
	WAVHeader *header = (WAVHeader *)buf;

	if (memcmp (header->riff, "RIFF",4) || memcmp (header->wavefmt, "WAVEfmt", 7)) {
		if (hardFail) {
			handleerror(0, "ReadWAV: invalid header.", name, 0);
		}
		return false;
	}

#define hswabword(c) header->c = swabword(header->c)
#define hswabdword(c) header->c = swabdword(header->c)
	hswabword(format_tag);
	hswabword(channels);
	hswabword(BlockAlign);
	hswabword(BitsPerSample);

	hswabdword(totalLength);
	hswabdword(length);
	hswabdword(SamplesPerSec);
	hswabdword(AvgBytesPerSec);
	hswabdword(datalen);
#undef hswabword
#undef hswabdword

	if (header->format_tag != 1) {
		// Microsoft PCM format?
		if (hardFail) {
			handleerror(0,"ReadWAV: invalid format tag.", name, 0);
		}
		return false;
	}

	wf.cbSize = 0;
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = header->channels;
	wf.nSamplesPerSec = header->SamplesPerSec;
	wf.wBitsPerSample = header->BitsPerSample;
	wf.nBlockAlign = header->channels * header->BitsPerSample / 8;
	wf.nAvgBytesPerSec = header->SamplesPerSec * wf.nBlockAlign;

	*bufferSize = header->datalen > fileSize-sizeof(WAVHeader)? fileSize - sizeof(WAVHeader): header->datalen;
	*soundData = buf + sizeof(WAVHeader);

	return true;
}

bool CDxSound::CreateStaticBuffer(const string& path)
{
	HRESULT hr;

	// Open the wav file and copy it to a buffer
	Uint8* buf = 0;
	CFileHandler file(path);
	int fileSize = file.FileSize();

	if (file.FileExists()) {
		buf = SAFE_NEW Uint8[fileSize];
		file.Read(buf, fileSize);
	} else {
		return false;
	}

	// Read the WAV file
	Uint8* sampleData = 0;
	Uint32 bufferSize;
	WAVEFORMATEX wf;

	if (!ReadWAV(path.c_str(), buf, fileSize, &sampleData, &bufferSize, wf)) {
		delete[] buf;
		return false;
	}

	LPDIRECTSOUNDBUFFER b = NULL;
	buffers.push_back(b);
	buffers.back() = 0;

	// Set up the direct sound buffer, and only request the flags needed
	// since each requires some overhead and limits if the buffer can 
	// be hardware accelerated
	DSBUFFERDESC dsbd;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));

	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_STATIC | DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = bufferSize;
	dsbd.lpwfxFormat   = &wf;

	// Create the static DirectSound buffer
	if (FAILED(hr = m_pDS->CreateSoundBuffer(&dsbd, &buffers.back(), NULL))) {
		delete[] buf;
		return false;
	}

	VOID*   pbData  = NULL;
	VOID*   pbData2 = NULL;
	DWORD   dwLength;
	DWORD   dwLength2;

	// Lock the buffer down
	if (FAILED(hr = buffers.back()->Lock(0, bufferSize, &pbData, &dwLength,
		&pbData2, &dwLength2, 0L))) {
		delete[] buf;
		return false;
	}

	memcpy(pbData, sampleData, bufferSize);

	// Unlock the buffer, we don't need it anymore.
	buffers.back()->Unlock(pbData, bufferSize, NULL, 0);
	delete[] buf;
	return true;
}


HRESULT CDxSound::RestoreBuffers(int num)
{
	HRESULT hr;

	if (NULL == buffers[num])
		return S_OK;

	DWORD dwStatus;
	if (FAILED(hr = buffers[num]->GetStatus( &dwStatus)))
		return hr;

	if (dwStatus & DSBSTATUS_BUFFERLOST) {
		// Since the app could have just been activated, then
		// DirectSound may not be giving us control yet, so
		// the restoring the buffer may fail.
		// If it does, sleep until DirectSound gives us control.
		do {
			hr = buffers[num]->Restore();
			if (hr == DSERR_BUFFERLOST)
				SleepEx(10, true);
		}
		while (FAILED(hr));
	}

	return S_OK;
}

void CDxSound::Update()
{
	oggStream.Update();

	float total = wantedSounds * 0.5f;

	for (std::list<PlayingSound>::iterator pi = playingSounds.begin(); pi != playingSounds.end(); ) {
		int num = pi->num;
		DWORD status;
		buffers[num]->GetStatus(&status);
		if (!status & DSBSTATUS_PLAYING) {
			pi = playingSounds.erase(pi);
			loadedSounds[buf2id[num]]->freebufs.push_back(num);
		} else {
			total -= (0.5f - pi->volume);
			++pi;
		}
	}

	total /= wantedSounds;
	curThreshhold = (curThreshhold + total) * 0.5f;
}
