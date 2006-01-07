// AVIGenerator.cpp: implementation of the CAVIGenerator class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <SDL_types.h>
#include "AVIGenerator.h"
#include "Rendering/GL/myGL.h"
//#include "mmgr.h"

#include <windows.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define ASSERT(x)

CAVIGenerator::CAVIGenerator()
: m_sFile(_T("Untitled.avi")), m_dwRate(30),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
	memset(&m_bih,0,sizeof(BITMAPINFOHEADER));
}

#ifdef _AVIGENERATOR_USE_MFC
CAVIGenerator::CAVIGenerator(LPCTSTR sFileName, CView* pView, DWORD dwRate)
: m_sFile(sFileName), m_dwRate(dwRate),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
		MakeExtAvi();
		SetBitmapHeader(pView);
}
#endif

CAVIGenerator::CAVIGenerator(LPCTSTR sFileName, LPBITMAPINFOHEADER lpbih, DWORD dwRate)
: m_sFile(sFileName), m_dwRate(dwRate),
m_pAVIFile(NULL), m_pStream(NULL), m_pStreamCompressed(NULL)
{
		MakeExtAvi();
		SetBitmapHeader(lpbih);
}

CAVIGenerator::~CAVIGenerator()
{
	// Just checking that all allocated ressources have been released
	ASSERT(m_pStream==NULL);
	ASSERT(m_pStreamCompressed==NULL);
	ASSERT(m_pAVIFile==NULL);
}

void CAVIGenerator::SetBitmapHeader(LPBITMAPINFOHEADER lpbih)
{
	// checking that bitmap size are multiple of 4
	ASSERT(lpbih->biWidth%4==0);
	ASSERT(lpbih->biHeight%4==0);

	// copying bitmap info structure.
	// corrected thanks to Lori Gardi
	memcpy(&m_bih,lpbih, sizeof(BITMAPINFOHEADER));
}

#ifdef _AVIGENERATOR_USE_MFC
void CAVIGenerator::SetBitmapHeader(CView *pView)
{
	ASSERT_VALID(pView);

	////////////////////////////////////////////////
	// Getting screen dimensions
	// Get client geometry 
	CRect rect; 
	pView->GetClientRect(&rect); 
	CSize size(rect.Width(),rect.Height()); 

	/////////////////////////////////////////////////
	// changing size of image so dimension are multiple of 4
	size.cx=(size.cx/4)*4;
	size.cy=(size.cy/4)*4;

	// initialize m_bih
	memset(&m_bih,0, sizeof(BITMAPINFOHEADER));
	// filling bitmap info structure.
	m_bih.biSize=sizeof(BITMAPINFOHEADER);
	m_bih.biWidth=size.cx;
	m_bih.biHeight=size.cy;
	m_bih.biPlanes=1;
	m_bih.biBitCount=24;
	m_bih.biSizeImage=((m_bih.biWidth*m_bih.biBitCount+31)/32 * 4)*m_bih.biHeight; 
	m_bih.biCompression=BI_RGB;		//BI_RGB means BRG in reality
}
#endif

HRESULT CAVIGenerator::InitEngine()
{
	AVISTREAMINFO strHdr; // information for a single stream 
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};

	TCHAR szBuffer[1024];
	HRESULT hr;

	m_sError=_T("Ok");

	// Step 0 : Let's make sure we are running on 1.1 
	DWORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		 // oops, we are too old, blow out of here 
		m_sError=_T("Version of Video for Windows too old. Come on, join the 21th century!");
		return S_FALSE;
	}

	// Step 1 : initialize AVI engine
	AVIFileInit();

	// Step 2 : Open the movie file for writing....
	hr = AVIFileOpen(&m_pAVIFile,			// Address to contain the new file interface pointer
		       (LPCSTR)m_sFile,				// Null-terminated string containing the name of the file to open
		       OF_WRITE | OF_CREATE,	    // Access mode to use when opening the file. 
		       NULL);						// use handler determined from file extension.
											// Name your file .avi -> very important

	if (hr != AVIERR_OK)
	{
		_tprintf(szBuffer,_T("AVI Engine failed to initialize. Check filename %s."),m_sFile);
		m_sError=szBuffer;
		// Check it succeded.
		switch(hr)
		{
		case AVIERR_BADFORMAT: 
			m_sError+=_T("The file couldn't be read, indicating a corrupt file or an unrecognized format.");
			break;
		case AVIERR_MEMORY:		
			m_sError+=_T("The file could not be opened because of insufficient memory."); 
			break;
		case AVIERR_FILEREAD:
			m_sError+=_T("A disk error occurred while reading the file."); 
			break;
		case AVIERR_FILEOPEN:		
			m_sError+=_T("A disk error occurred while opening the file.");
			break;
		case REGDB_E_CLASSNOTREG:		
			m_sError+=_T("According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it");
			break;
		}

		return hr;
	}

	// Fill in the header for the video stream....
	memset(&strHdr, 0, sizeof(strHdr));
	strHdr.fccType                = streamtypeVIDEO;	// video stream type
	strHdr.fccHandler             = 0;
	strHdr.dwScale                = 1;					// should be one for video
	strHdr.dwRate                 = m_dwRate;		    // fps
	strHdr.dwSuggestedBufferSize  = m_bih.biSizeImage;	// Recommended buffer size, in bytes, for the stream.
	SetRect(&strHdr.rcFrame, 0, 0,		    // rectangle for stream
	    (int) m_bih.biWidth,
	    (int) m_bih.biHeight);

	// Step 3 : Create the stream;
	hr = AVIFileCreateStream(m_pAVIFile,		    // file pointer
			         &m_pStream,		    // returned stream pointer
			         &strHdr);	    // stream header

	// Check it succeded.
	if (hr != AVIERR_OK)
	{
		m_sError=_T("AVI Stream creation failed. Check Bitmap info.");
		if (hr==AVIERR_READONLY)
		{
			m_sError+=_T(" Read only file.");
		}
		return hr;
	}


	// Step 4: Get codec and infos about codec
	memset(&opts, 0, sizeof(opts));
	// Poping codec dialog
	if (!AVISaveOptions(NULL, 0, 1, &m_pStream, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
	{
		AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
		return S_FALSE;
	}

	// Step 5:  Create a compressed stream using codec options.
	hr = AVIMakeCompressedStream(&m_pStreamCompressed, 
				m_pStream, 
				&opts, 
				NULL);

	if (hr != AVIERR_OK)
	{
		m_sError=_T("AVI Compressed Stream creation failed.");
		
		switch(hr)
		{
		case AVIERR_NOCOMPRESSOR:
			m_sError+=_T(" A suitable compressor cannot be found.");
				break;
		case AVIERR_MEMORY:
			m_sError+=_T(" There is not enough memory to complete the operation.");
				break; 
		case AVIERR_UNSUPPORTED:
			m_sError+=_T("Compression is not supported for this type of data. This error might be returned if you try to compress data that is not audio or video.");
			break;
		}

		return hr;
	}

	// releasing memory allocated by AVISaveOptionFree
	hr=AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
	if (hr!=AVIERR_OK)
	{
		m_sError=_T("Error releasing memory");
		return hr;
	}

	// Step 6 : sets the format of a stream at the specified position
	hr = AVIStreamSetFormat(m_pStreamCompressed, 
					0,			// position
					&m_bih,	    // stream format
					m_bih.biSize +   // format size
					m_bih.biClrUsed * sizeof(RGBQUAD));

	if (hr != AVIERR_OK)
	{
		m_sError=_T("AVI Compressed Stream format setting failed.");
		return hr;
	}

	// Step 6 : Initialize step counter
	m_lFrame=0;

	return hr;
}

void CAVIGenerator::ReleaseEngine()
{
	if (m_pStream)
	{
		AVIStreamRelease(m_pStream);
		m_pStream=NULL;
	}

	if (m_pStreamCompressed)
	{
		AVIStreamRelease(m_pStreamCompressed);
		m_pStreamCompressed=NULL;
	}

	if (m_pAVIFile)
	{
		AVIFileRelease(m_pAVIFile);
		m_pAVIFile=NULL;
	}

	// Close engine
	AVIFileExit();
}

HRESULT CAVIGenerator::AddFrame(BYTE *bmBits)
{
	HRESULT hr;

	// compress bitmap
	hr = AVIStreamWrite(m_pStreamCompressed,	// stream pointer
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

void CAVIGenerator::MakeExtAvi()
{
	// finding avi
	if( _tcsstr((const char *)m_sFile,(const char *)_T("avi"))==NULL )
	{
		m_sFile+=_T(".avi");
	}
}
