// AVIGenerator.cpp: implementation of the CAVIGenerator class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AVIGenerator.h"
#include <cassert>
#include "mmgr.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#pragma message("     _Adding library: vfw32.lib" ) 
#pragma comment(lib, "vfw32.lib")
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////



bool CAVIGenerator::initVFW(){
#if defined(_WIN32) && defined(__MINGW32__)

	HMODULE hMod = LoadLibrary("msvfw32.dll");
	if (NULL == hMod){
		m_sError="LoadLibrary failed.";
		return false;
	}


	VideoForWindowsVersion_ptr=(VideoForWindowsVersion_type)GetProcAddress(hMod, "VideoForWindowsVersion");  
	ICCompressorChoose_ptr=(ICCompressorChoose_type)GetProcAddress(hMod, "ICCompressorChoose");  
	ICCompressorFree_ptr=(ICCompressorFree_type)GetProcAddress(hMod, "ICCompressorFree");



	if(!VideoForWindowsVersion_ptr || !ICCompressorChoose_ptr || !ICCompressorFree_ptr){
		m_sError="initVFW Error.";   
		return false;                             
	}                



	hMod = LoadLibrary("avifil32.dll");
	if (NULL == hMod){
		m_sError="LoadLibrary failed.";
		return false;          
	}


	AVIFileInit_ptr=(AVIFileInit_type)GetProcAddress(hMod, "AVIFileInit");                  
	AVIFileOpenA_ptr=(AVIFileOpenA_type)GetProcAddress(hMod, "AVIFileOpenA");                                           
	AVIFileCreateStreamA_ptr=(AVIFileCreateStreamA_type)GetProcAddress(hMod, "AVIFileCreateStreamA");                                                
	AVIMakeCompressedStream_ptr=(AVIMakeCompressedStream_type)GetProcAddress(hMod, "AVIMakeCompressedStream");                                                     
	AVIStreamSetFormat_ptr=(AVIStreamSetFormat_type)GetProcAddress(hMod, "AVIStreamSetFormat");                                                          
	AVIStreamRelease_ptr=(AVIStreamRelease_type)GetProcAddress(hMod, "AVIStreamRelease");                                                               
	AVIFileRelease_ptr=(AVIFileRelease_type)GetProcAddress(hMod, "AVIFileRelease");                                                               
	AVIFileExit_ptr=(AVIFileExit_type)GetProcAddress(hMod, "AVIFileExit");                                                                    
	AVIStreamWrite_ptr=(AVIStreamWrite_type)GetProcAddress(hMod, "AVIStreamWrite");  


	if(!AVIFileInit_ptr || !AVIFileOpenA_ptr || !AVIFileCreateStreamA_ptr || 
		!AVIMakeCompressedStream_ptr || !AVIStreamSetFormat_ptr || !AVIStreamRelease_ptr ||
		!AVIFileRelease_ptr || !AVIFileExit_ptr || !AVIStreamWrite_ptr){
			m_sError="initVFW Error.";
			return false;                                   
	} 
	return true;   
#else

	VideoForWindowsVersion_ptr =&VideoForWindowsVersion;
	AVIFileInit_ptr = &AVIFileInit;
	AVIFileOpenA_ptr = &AVIFileOpenA;
	AVIFileCreateStreamA_ptr = &AVIFileCreateStreamA;
	AVIMakeCompressedStream_ptr = &AVIMakeCompressedStream;
	AVIStreamSetFormat_ptr = &AVIStreamSetFormat;
	AVIStreamRelease_ptr = &AVIStreamRelease;
	AVIFileRelease_ptr = &AVIFileRelease;
	AVIFileExit_ptr = &AVIFileExit;
	AVIStreamWrite_ptr = &AVIStreamWrite;
	ICCompressorChoose_ptr = &ICCompressorChoose;
	ICCompressorFree_ptr = &ICCompressorFree;

	return true;
#endif                                                                          
}



CAVIGenerator::CAVIGenerator(const std::string & sFileName, LPBITMAPINFOHEADER lpbih, DWORD dwRate)
: m_sFile(sFileName), m_dwRate(dwRate), pixelDataBuf(0),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
	MakeExtAvi();
	SetBitmapHeader(lpbih);
}

CAVIGenerator::~CAVIGenerator()
{
	// Just checking that all allocated ressources have been released
	assert(m_pStream==NULL);
	assert(m_pStreamCompressed==NULL);
	assert(m_pAVIFile==NULL);
	assert(pixelDataBuf==NULL);
}

void CAVIGenerator::SetBitmapHeader(LPBITMAPINFOHEADER lpbih)
{
	// checking that bitmap size are multiple of 4
	assert(lpbih->biWidth%4==0);
	assert(lpbih->biHeight%4==0);
	assert(lpbih->biBitCount==24);
	assert(lpbih->biCompression==BI_RGB);
	assert(lpbih->biSizeImage==lpbih->biWidth*lpbih->biHeight*3);

	// copying bitmap info structure.
	// corrected thanks to Lori Gardi
	memcpy(&m_bih,lpbih, sizeof(BITMAPINFOHEADER));
}


HRESULT CAVIGenerator::InitEngine()
{
	AVISTREAMINFO strHdr; // information for a single stream 
	AVICOMPRESSOPTIONS opts;

	HRESULT hr;

	m_sError= "Ok";


	if(!initVFW()){
		return S_FALSE;
	}

	triggerCleanup cleaner(this);

	// Step 0 : Let's make sure we are running on 1.1 
	DWORD wVer = HIWORD(VideoForWindowsVersion_ptr());
	if (wVer < 0x010a)
	{
		// oops, we are too old, blow out of here 
		m_sError="Version of Video for Windows too old. Come on, join the 21th century!";
		return S_FALSE;
	}

	// Step 1 : initialize AVI engine
	AVIFileInit_ptr();


	memset(&cv,0,sizeof(COMPVARS));

	cv.cbSize=sizeof(COMPVARS);
	cv.dwFlags=ICMF_COMPVARS_VALID;
	cv.fccHandler=mmioFOURCC('W','M','V','3');
	cv.lQ=ICQUALITY_DEFAULT;


	HWND hWnd = FindWindow(NULL, "RtsSpring");

	if (!ICCompressorChoose_ptr(hWnd, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME, &m_bih, NULL, &cv, NULL)){
		return S_FALSE;
	}


	// Fill in the header for the video stream....
	memset(&strHdr, 0, sizeof(AVISTREAMINFO));
	strHdr.fccType                = streamtypeVIDEO;	// video stream type
	strHdr.fccHandler             = cv.fccHandler;
	strHdr.dwScale                = 1;					// should be one for video
	strHdr.dwRate                 = m_dwRate;		    // fps
	strHdr.dwSuggestedBufferSize  = m_bih.biSizeImage;	// Recommended buffer size, in bytes, for the stream.
	SetRect(&strHdr.rcFrame, 0, 0, (int)m_bih.biWidth, (int)m_bih.biHeight); // rectangle for stream
	


	memset(&opts, 0, sizeof(AVICOMPRESSOPTIONS));
	opts.fccType=streamtypeVIDEO;
	opts.fccHandler=cv.fccHandler;
	opts.dwKeyFrameEvery=cv.lKey;
	opts.dwQuality=cv.lQ;
	opts.dwBytesPerSecond=cv.lDataRate;
	opts.dwFlags=(cv.lDataRate>0?AVICOMPRESSF_DATARATE:0)|(cv.lKey>0?AVICOMPRESSF_KEYFRAMES:0);
	opts.lpFormat=NULL;
	opts.cbFormat=0;
	opts.lpParms=cv.lpState;
	opts.cbParms=cv.cbState;
	opts.dwInterleaveEvery=0;

	// Step 2 : Open the movie file for writing....
	hr = AVIFileOpenA_ptr(&m_pAVIFile,			// Address to contain the new file interface pointer
		m_sFile.c_str(),				// Null-terminated string containing the name of the file to open
		OF_WRITE | OF_CREATE | OF_SHARE_EXCLUSIVE,	    // Access mode to use when opening the file. 
		NULL);						// use handler determined from file extension.
	// Name your file .avi -> very important

	if (hr != AVIERR_OK)
	{
		//_tprintf(szBuffer,"AVI Engine failed to initialize. Check filename %s.",m_sFile);
		m_sError="AVI Engine failed to initialize. Check filename ";
		m_sError+=m_sFile;
		// Check it succeded.
		switch(hr)
		{
		case AVIERR_BADFORMAT: 
			m_sError+="The file couldn't be read, indicating a corrupt file or an unrecognized format.";
			break;
		case AVIERR_MEMORY:		
			m_sError+="The file could not be opened because of insufficient memory."; 
			break;
		case AVIERR_FILEREAD:
			m_sError+="A disk error occurred while reading the file."; 
			break;
		case AVIERR_FILEOPEN:		
			m_sError+="A disk error occurred while opening the file.";
			break;
		case REGDB_E_CLASSNOTREG:		
			m_sError+="According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it";
			break;
		}
		return hr;
	}


	// Step 3 : Create the stream;
	hr = AVIFileCreateStreamA_ptr(m_pAVIFile,		    // file pointer
		&m_pStream,		    // returned stream pointer
		&strHdr);	    // stream header

	// Check it succeded.
	if (hr != AVIERR_OK)
	{
		m_sError="AVI Stream creation failed. Check Bitmap info.";
		if (hr==AVIERR_READONLY)
		{
			m_sError+=" Read only file.";
		}
		return hr;
	}


	// Step 5:  Create a compressed stream using codec options.
	hr = AVIMakeCompressedStream_ptr(&m_pStreamCompressed, 
		m_pStream, 
		&opts, 
		NULL);

	if (hr != AVIERR_OK)
	{
		m_sError="AVI Compressed Stream creation failed.";

		switch(hr)
		{
		case AVIERR_NOCOMPRESSOR:
			m_sError+=" A suitable compressor cannot be found.";
			break;
		case AVIERR_MEMORY:
			m_sError+=" There is not enough memory to complete the operation.";
			break; 
		case AVIERR_UNSUPPORTED:
			m_sError+="Compression is not supported for this type of data. This error might be returned if you try to compress data that is not audio or video.";
			break;
		}
		return hr;
	}


	// Step 6 : sets the format of a stream at the specified position
	hr = AVIStreamSetFormat_ptr(m_pStreamCompressed, 
		0,			// position
		&m_bih,	    // stream format
		m_bih.biSize +   // format size
		m_bih.biClrUsed * sizeof(RGBQUAD));

	if (hr != AVIERR_OK)
	{
		m_sError="AVI Compressed Stream format setting failed.";
		return hr;
	}

	// Step 6 : Initialize step counter
	m_lFrame=0;
	pixelDataBuf = SAFE_NEW unsigned char[m_bih.biSizeImage];
	cleaner.disarm();

	return hr;
}


void CAVIGenerator::ReleaseEngine()
{
	if (m_pStream)
	{
		AVIStreamRelease_ptr(m_pStream);
		m_pStream=NULL;
	}

	if (m_pStreamCompressed)
	{
		AVIStreamRelease_ptr(m_pStreamCompressed);
		m_pStreamCompressed=NULL;
	}

	if (m_pAVIFile)
	{
		AVIFileRelease_ptr(m_pAVIFile);
		m_pAVIFile=NULL;
	}

	ICCompressorFree_ptr(&cv);

	delete [] pixelDataBuf;
	pixelDataBuf=0;

	// Close engine
	AVIFileExit_ptr();
}

HRESULT CAVIGenerator::AddFrame(BYTE *bmBits)
{
	HRESULT hr;

	// compress bitmap
	hr = AVIStreamWrite_ptr(m_pStreamCompressed,	// stream pointer
		m_lFrame,						// time of this frame
		1,						// number to write
		bmBits,					// image buffer
		m_bih.biSizeImage,		// size of this frame
		AVIIF_KEYFRAME,			// flags....
		NULL,
		NULL);

	// updating frame counter
	m_lFrame++;

	return hr;
}

void CAVIGenerator::MakeExtAvi(){

	std::size_t pos = m_sFile.find_last_of(".avi");
	if(pos == std::string::npos || pos + 1 != m_sFile.size()){
		m_sFile += ".avi";      
	}
}
