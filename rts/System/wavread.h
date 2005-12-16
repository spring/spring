//-----------------------------------------------------------------------------
// File: WavRead.h
//
// Desc: Support for loading and playing Wave files using DirectSound sound
//       buffers.
//
// Copyright (c) 1999 Microsoft Corp. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef WAVE_READ_H
#define WAVE_READ_H


#include <mmreg.h>
#include <mmsystem.h>

#include <SDL_types.h>


//-----------------------------------------------------------------------------
// Name: class CWaveSoundRead
// Desc: A class to read in sound data from a Wave file
//-----------------------------------------------------------------------------
class CWaveSoundRead
{
public:
    WAVEFORMATEX* m_pwfx;        // Pointer to WAVEFORMATEX structure
    HMMIO         m_hmmioIn;     // MM I/O handle for the WAVE
    MMCKINFO      m_ckIn;        // Multimedia RIFF chunk
    MMCKINFO      m_ckInRiff;    // Use in opening a WAVE file

	char *buf;		//for reading wavfiles from memory
	int filesize;

public:
    CWaveSoundRead();
    ~CWaveSoundRead();

    Sint32 Open(const CHAR* strFilename );
    Sint32 Reset();
    Sint32 Read( UINT nSizeToRead, BYTE* pbData, UINT* pnSizeRead );
    Sint32 Close();

};


#endif WAVE_READ_H




