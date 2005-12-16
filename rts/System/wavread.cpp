//-----------------------------------------------------------------------------
// File: WavRead.cpp
//
// Desc: Wave file support for loading and playing Wave files using DirectSound 
//       buffers.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include <windows.h>
#include "WavRead.h"
#include "FileHandler.h"

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }




//-----------------------------------------------------------------------------
// Name: ReadMMIO()
// Desc: Support function for reading from a multimedia I/O stream
//-----------------------------------------------------------------------------
Sint32 ReadMMIO( HMMIO hmmioIn, MMCKINFO* pckInRIFF, WAVEFORMATEX** ppwfxInfo )
{
	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       

	*ppwfxInfo = NULL;

	if( ( 0 != mmioDescend( hmmioIn, pckInRIFF, NULL, 0 ) ) )
		return E_FAIL;

	if( (pckInRIFF->ckid != FOURCC_RIFF) ||
		(pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E') ) )
		return E_FAIL;

	// Search the input file for for the 'fmt ' chunk.
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if( 0 != mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK) )
		return E_FAIL;

	// Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
	// if there are extra parameters at the end, we'll ignore them
	if( ckIn.cksize < (Sint32) sizeof(PCMWAVEFORMAT) )
		return E_FAIL;

	// Read the 'fmt ' chunk into <pcmWaveFormat>.
	if( mmioRead( hmmioIn, (HPSTR) &pcmWaveFormat, 
		sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat) )
		return E_FAIL;

	// Allocate the waveformatex, but if its not pcm format, read the next
	// word, and thats how many extra bytes to allocate.
	if( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM )
	{
		if( NULL == ( *ppwfxInfo = (WAVEFORMATEX*)new CHAR[sizeof(WAVEFORMATEX)] ) )
			return E_FAIL;

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
		(*ppwfxInfo)->cbSize = 0;
	}
	else
	{
		// Read in length of extra bytes.
		WORD cbExtraBytes = 0L;
		if( mmioRead( hmmioIn, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD) )
			return E_FAIL;

		*ppwfxInfo = (WAVEFORMATEX*)new CHAR[ sizeof(WAVEFORMATEX) + cbExtraBytes ];
		if( NULL == *ppwfxInfo )
			return E_FAIL;

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy( *ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat) );
		(*ppwfxInfo)->cbSize = cbExtraBytes;

		// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
		if( mmioRead( hmmioIn, (CHAR*)(((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(WORD)),
			cbExtraBytes ) != cbExtraBytes )
		{
			delete[] *ppwfxInfo;
			*ppwfxInfo = NULL;
			return E_FAIL;
		}
	}

	// Ascend the input file out of the 'fmt ' chunk.
	if( 0 != mmioAscend( hmmioIn, &ckIn, 0 ) )
	{
		delete[] *ppwfxInfo;
		*ppwfxInfo = NULL;
		return E_FAIL;
	}

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveOpenFile()
// Desc: This function will open a wave input file and prepare it for reading,
//       so the data can be easily read with WaveReadFile. Returns 0 if
//       successful, the error code if not.
//-----------------------------------------------------------------------------
Sint32 WaveOpenFile(const  CHAR* strFileName, HMMIO* phmmioIn, WAVEFORMATEX** ppwfxInfo,
										 MMCKINFO* pckInRIFF, char *buf, int filesize )
{
	Sint32 hr;
	HMMIO   hmmioIn = NULL;
	int a;

	char c[500];
	for(a=0;strFileName[a]!=0;a++)
		c[a]=strFileName[a];
	c[a]=0;
	//	MessageBox(0,c,strFileName,0);
	MMIOINFO mminfo;
	memset(&mminfo, 0, sizeof(MMIOINFO));
	mminfo.pchBuffer = buf;
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.cchBuffer = filesize;

	if( NULL == ( hmmioIn = mmioOpen( NULL, &mminfo, MMIO_ALLOCBUF|MMIO_READ ) ) )
		return E_FAIL;

	if( FAILED( hr = ReadMMIO( hmmioIn, pckInRIFF, ppwfxInfo ) ) )
	{
		mmioClose( hmmioIn, 0 );
		return hr;
	}

	*phmmioIn = hmmioIn;

	//delete buf;
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveStartDataRead()
// Desc: Routine has to be called before WaveReadFile as it searches for the
//       chunk to descend into for reading, that is, the 'data' chunk.  For
//       simplicity, this used to be in the open routine, but was taken out and
//       moved to a separate routine so there was more control on the chunks
//       that are before the data chunk, such as 'fact', etc...
//-----------------------------------------------------------------------------
Sint32 WaveStartDataRead( HMMIO* phmmioIn, MMCKINFO* pckIn,
													MMCKINFO* pckInRIFF )
{
	// Seek to the data
	if( -1 == mmioSeek( *phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC),
		SEEK_SET ) )
		return E_FAIL;

	// Search the input file for for the 'data' chunk.
	pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
	if( 0 != mmioDescend( *phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK ) )
		return E_FAIL;

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WaveReadFile()
// Desc: Reads wave data from the wave file. Make sure we're descended into
//       the data chunk before calling this function.
//          hmmioIn      - Handle to mmio.
//          cbRead       - # of bytes to read.   
//          pbDest       - Destination buffer to put bytes.
//          cbActualRead - # of bytes actually read.
//-----------------------------------------------------------------------------
Sint32 WaveReadFile( HMMIO hmmioIn, UINT cbRead, BYTE* pbDest,
										 MMCKINFO* pckIn, UINT* cbActualRead )
{
	MMIOINFO mmioinfoIn;         // current status of <hmmioIn>

	*cbActualRead = 0;

	if( 0 != mmioGetInfo( hmmioIn, &mmioinfoIn, 0 ) )
		return E_FAIL;

	UINT cbDataIn = cbRead;
	if( cbDataIn > pckIn->cksize ) 
		cbDataIn = pckIn->cksize;       

	pckIn->cksize -= cbDataIn;

	for( DWORD cT = 0; cT < cbDataIn; cT++ )
	{
		// Copy the bytes from the io to the buffer.
		if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
		{
			if( 0 != mmioAdvance( hmmioIn, &mmioinfoIn, MMIO_READ ) )
				return E_FAIL;

			if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
				return E_FAIL;
		}

		// Actual copy.
		*((BYTE*)pbDest+cT) = *((BYTE*)mmioinfoIn.pchNext);
		mmioinfoIn.pchNext++;
	}

	if( 0 != mmioSetInfo( hmmioIn, &mmioinfoIn, 0 ) )
		return E_FAIL;

	*cbActualRead = cbDataIn;
	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CWaveSoundRead()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
CWaveSoundRead::CWaveSoundRead()
{
	m_pwfx   = NULL;
	buf = NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CWaveSoundRead()
// Desc: Destructs the class
//-----------------------------------------------------------------------------
CWaveSoundRead::~CWaveSoundRead()
{
	Close();
	delete[] m_pwfx ;
	m_pwfx=0;
	delete[] buf;
	buf=0;
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Opens a wave file for reading
//-----------------------------------------------------------------------------
Sint32 CWaveSoundRead::Open(const CHAR* strFilename )
{
	delete[] m_pwfx;
	m_pwfx=0;

	Sint32  hr;

	CFileHandler file(strFilename);
	if(file.FileExists()){
		buf = new char[file.FileSize()];
		file.Read(buf, file.FileSize());
	} else {
		MessageBox(0,"Couldnt open wav file",strFilename,0);
		buf = new char[1];
	}

	if( FAILED( hr = WaveOpenFile( strFilename, &m_hmmioIn, &m_pwfx, &m_ckInRiff , buf, file.FileSize()) ) )
		return hr;

	if( FAILED( hr = Reset() ) )
		return hr;

	return hr;
}




//-----------------------------------------------------------------------------
// Name: Reset()
// Desc: Resets the internal m_ckIn pointer so reading starts from the 
//       beginning of the file again 
//-----------------------------------------------------------------------------
Sint32 CWaveSoundRead::Reset()
{
	return WaveStartDataRead( &m_hmmioIn, &m_ckIn, &m_ckInRiff );
}




//-----------------------------------------------------------------------------
// Name: Read()
// Desc: Reads a wave file into a pointer and returns how much read
//       using m_ckIn to determine where to start reading from
//-----------------------------------------------------------------------------
Sint32 CWaveSoundRead::Read( UINT nSizeToRead, BYTE* pbData, UINT* pnSizeRead )
{
	return WaveReadFile( m_hmmioIn, nSizeToRead, pbData, &m_ckIn, pnSizeRead );
}




//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Closes an open wave file 
//-----------------------------------------------------------------------------
Sint32 CWaveSoundRead::Close()
{
	mmioClose( m_hmmioIn, 0 );
	if(buf)
	{
		delete[] buf;
		buf = 0;
	}
	return S_OK;
}


