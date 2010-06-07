/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if       defined AVI_CAPTURING
#ifdef _MSC_VER
#include "StdAfx.h"
#endif
#include "AVIGenerator.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Game/GameVersion.h"
#include "System/LogOutput.h"
#include "System/SpringApp.h"

#include <windows.h>
#if defined(_WIN32) && defined(__MINGW32__)
#include <GL/glext.h>
#endif

#include <boost/bind.hpp>
#include <cassert>
#include "mmgr.h"

#if defined(_WIN32) && !defined(__MINGW32__)
#pragma message("Adding library: vfw32.lib")
#pragma comment(lib, "vfw32.lib")
#endif




bool CAVIGenerator::initVFW() {
#if defined(_WIN32) && defined(__MINGW32__)

	msvfw32 = LoadLibrary("msvfw32.dll");
	avifil32 = LoadLibrary("avifil32.dll");

	VideoForWindowsVersion_ptr=(VideoForWindowsVersion_type)GetProcAddress(msvfw32, "VideoForWindowsVersion");
	ICCompressorChoose_ptr=(ICCompressorChoose_type)GetProcAddress(msvfw32, "ICCompressorChoose");
	ICCompressorFree_ptr=(ICCompressorFree_type)GetProcAddress(msvfw32, "ICCompressorFree");
	ICOpen_ptr=(ICOpen_type)GetProcAddress(msvfw32, "ICOpen");

	AVIFileInit_ptr=(AVIFileInit_type)GetProcAddress(avifil32, "AVIFileInit");
	AVIFileOpenA_ptr=(AVIFileOpenA_type)GetProcAddress(avifil32, "AVIFileOpenA");
	AVIFileCreateStreamA_ptr=(AVIFileCreateStreamA_type)GetProcAddress(avifil32, "AVIFileCreateStreamA");
	AVIMakeCompressedStream_ptr=(AVIMakeCompressedStream_type)GetProcAddress(avifil32, "AVIMakeCompressedStream");
	AVIStreamSetFormat_ptr=(AVIStreamSetFormat_type)GetProcAddress(avifil32, "AVIStreamSetFormat");
	AVIStreamRelease_ptr=(AVIStreamRelease_type)GetProcAddress(avifil32, "AVIStreamRelease");
	AVIFileRelease_ptr=(AVIFileRelease_type)GetProcAddress(avifil32, "AVIFileRelease");
	AVIFileExit_ptr=(AVIFileExit_type)GetProcAddress(avifil32, "AVIFileExit");
	AVIStreamWrite_ptr=(AVIStreamWrite_type)GetProcAddress(avifil32, "AVIStreamWrite");


	if (NULL == msvfw32 || NULL == avifil32) {
		errorMsg="LoadLibrary failed.";
		return false;
	}

	if (!VideoForWindowsVersion_ptr || !ICCompressorChoose_ptr || !ICCompressorFree_ptr || !ICOpen_ptr) {
		errorMsg="initVFW Error.";
		return false;
	}

	if (!AVIFileInit_ptr || !AVIFileOpenA_ptr || !AVIFileCreateStreamA_ptr ||
		!AVIMakeCompressedStream_ptr || !AVIStreamSetFormat_ptr || !AVIStreamRelease_ptr ||
		!AVIFileRelease_ptr || !AVIFileExit_ptr || !AVIStreamWrite_ptr) {
			errorMsg="initVFW Error.";
			return false;
	}
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
	ICOpen_ptr = &ICOpen;
#endif
	return true;
}


CAVIGenerator::CAVIGenerator(const std::string& fileName, int videoSizeX, int videoSizeY, DWORD videoFPS)
							:
							fileName(fileName),
							videoFPS(videoFPS),
							errorMsg("Ok"),
							quitAVIgen(false),
							AVIThread(0),
							readBuf(NULL),
							m_lFrame(0),
							m_pAVIFile(NULL),
							m_pStream(NULL),
							m_pStreamCompressed(NULL) {

	assert(videoSizeX % 4 == 0);
	assert(videoSizeY % 4 == 0);

	memset(&bitmapInfo, 0, sizeof(BITMAPINFOHEADER));
	bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.biWidth = videoSizeX;
	bitmapInfo.biHeight = videoSizeY;
	bitmapInfo.biPlanes = 1;
	bitmapInfo.biBitCount = 24;
	bitmapInfo.biSizeImage = videoSizeX * videoSizeY * 3;
	bitmapInfo.biCompression = BI_RGB;

	if (!initVFW()) {
		quitAVIgen = true;
	}
}


CAVIGenerator::~CAVIGenerator() {

	if (AVIThread) {
		{
			boost::mutex::scoped_lock lock(AVIMutex);
			quitAVIgen = true;
			AVICondition.notify_all();
		}
		AVIThread->join();

		delete AVIThread;
		AVIThread = 0;
	}

	while (!freeImageBuffers.empty()) {
		unsigned char* tmp = freeImageBuffers.front();
		freeImageBuffers.pop_front();
		delete [] tmp;
	}
	while (!imageBuffers.empty()) {
		unsigned char* tmp = imageBuffers.front();
		imageBuffers.pop_front();
		delete [] tmp;
	}

	delete [] readBuf;
	readBuf = 0;


	ReleaseAVICompressionEngine();
	logOutput.Print(std::string("Finished writing avi file, ") + fileName);

	// Just checking that all allocated ressources have been released.
	assert(AVIThread == NULL);
	assert(m_pAVIFile == NULL);
	assert(m_pStream == NULL);
	assert(m_pStreamCompressed == NULL);
	assert(freeImageBuffers.empty());
	assert(imageBuffers.empty());
	assert(readBuf == NULL);
}


void CAVIGenerator::ReleaseAVICompressionEngine() {

	if (m_pStream && AVIStreamRelease_ptr) {
		AVIStreamRelease_ptr(m_pStream);
		m_pStream=NULL;
	}

	if(m_pStreamCompressed && AVIStreamRelease_ptr){
		AVIStreamRelease_ptr(m_pStreamCompressed);
		m_pStreamCompressed=NULL;
	}

	if (m_pAVIFile && AVIFileRelease_ptr) {
		AVIFileRelease_ptr(m_pAVIFile);
		m_pAVIFile=NULL;
	}

	if (ICCompressorFree_ptr) {
		ICCompressorFree_ptr(&cv);
	}
	if (AVIFileExit_ptr) {
		AVIFileExit_ptr();
	}

	if (msvfw32) {
		FreeLibrary(msvfw32);
	}
	if (avifil32) {
		FreeLibrary(avifil32);
	}
}


HRESULT CAVIGenerator::InitAVICompressionEngine() {

	AVISTREAMINFO strHdr; // Information for a single stream
	AVICOMPRESSOPTIONS opts;
	HRESULT hr;


	// Let us make sure we are running on 1.1
	DWORD wVer = HIWORD(VideoForWindowsVersion_ptr());
	if (wVer < 0x010a) {
		// oops, we are too old, blow out of here
		errorMsg = "Version of Video for Windows too old. Come on, join the 21th century!";
		return S_FALSE;
	}

	// Initialize AVI engine
	AVIFileInit_ptr();


	memset(&cv, 0, sizeof(COMPVARS));
	cv.cbSize = sizeof(COMPVARS);
	cv.dwFlags = ICMF_COMPVARS_VALID;
	cv.fccHandler = mmioFOURCC('x', 'v', 'i', 'd'); // default video codec
	cv.lQ = ICQUALITY_DEFAULT;


	//Set the compression, prompting dialog if necessary
	if (!ICCompressorChoose_ptr(NULL, ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME, &bitmapInfo, NULL, &cv, NULL)) {
		return S_FALSE;
	}

	// Fill in the header for the video stream....
	memset(&strHdr, 0, sizeof(AVISTREAMINFO));
	strHdr.fccType                = streamtypeVIDEO;	// video stream type
	strHdr.fccHandler             = cv.fccHandler;
	strHdr.dwScale                = 1;					// should be one for video
	strHdr.dwRate                 = videoFPS;			// fps
	strHdr.dwSuggestedBufferSize  = bitmapInfo.biSizeImage;	// Recommended buffer size, in bytes, for the stream.
	SetRect(&strHdr.rcFrame, 0, 0, bitmapInfo.biWidth, bitmapInfo.biHeight);
	strcpy(strHdr.szName, "Spring video.");


	memset(&opts, 0, sizeof(AVICOMPRESSOPTIONS));
	opts.fccType = streamtypeVIDEO;
	opts.fccHandler = cv.fccHandler;
	opts.dwKeyFrameEvery = cv.lKey;
	opts.dwQuality = cv.lQ;
	opts.dwBytesPerSecond = cv.lDataRate;
	opts.dwFlags = ((cv.lDataRate > 0) ? AVICOMPRESSF_DATARATE : 0) | ((cv.lKey > 0) ? AVICOMPRESSF_KEYFRAMES : 0);
	opts.lpFormat = NULL;
	opts.cbFormat = 0;
	opts.lpParms = cv.lpState;
	opts.cbParms = cv.cbState;
	opts.dwInterleaveEvery = 0;



	//Open the movie file for writing
	hr = AVIFileOpenA_ptr(&m_pAVIFile,			// Address to contain the new file interface pointer
		fileName.c_str(),				// Null-terminated string containing the name of the file to open
		OF_WRITE | OF_CREATE | OF_SHARE_EXCLUSIVE,	// Access mode to use when opening the file.
		NULL);						// use handler determined from file extension.

	if (hr != AVIERR_OK) {
		errorMsg = "AVI Engine failed to initialize. Check filename ";
		errorMsg += fileName;
		// Translate error code
		switch (hr) {
			case AVIERR_BADFORMAT: {
				errorMsg += "The file couldn't be read, indicating a corrupt file or an unrecognized format.";
			} break;
			case AVIERR_MEMORY: {
				errorMsg += "The file could not be opened because of insufficient memory.";
			} break;
			case AVIERR_FILEREAD: {
				errorMsg += "A disk error occurred while reading the file.";
			} break;
			case AVIERR_FILEOPEN: {
				errorMsg += "A disk error occurred while opening the file.";
			} break;
			case REGDB_E_CLASSNOTREG: {
				errorMsg += "According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it";
			} break;
			default: {
				errorMsg += "Unknown error.";
			}
		}
		return hr;
	}


	// Create the stream
	hr = AVIFileCreateStreamA_ptr(m_pAVIFile,		    // file pointer
		&m_pStream,		    // returned stream pointer
		&strHdr);	    // stream header

	if (hr != AVIERR_OK) {
		errorMsg = "AVI Stream creation failed. Check Bitmap info.";
		if (hr == AVIERR_READONLY) {
			errorMsg += " Read only file.";
		}
		return hr;
	}


	// Create a compressed stream using codec options.
	hr = AVIMakeCompressedStream_ptr(&m_pStreamCompressed, m_pStream, &opts, NULL);
	if (hr != AVIERR_OK) {
		errorMsg = "AVI Compressed Stream creation failed.";

		switch (hr) {
			case AVIERR_NOCOMPRESSOR: {
				errorMsg += " A suitable compressor cannot be found.";
			} break;
			case AVIERR_MEMORY: {
				errorMsg += " There is not enough memory to complete the operation.";
			} break;
			case AVIERR_UNSUPPORTED: {
				errorMsg += "Compression is not supported for this type of data. This error might be returned if you try to compress data that is not audio or video.";
			} break;
			default: {
				errorMsg += "Unknown error.";
			}
		}
		return hr;
	}


	//Sets the format of a stream at the specified position
	hr = AVIStreamSetFormat_ptr(m_pStreamCompressed,
		0,			// position
		&bitmapInfo,	    // stream format
		bitmapInfo.biSize +   // format size
		bitmapInfo.biClrUsed * sizeof(RGBQUAD));

	if (hr != AVIERR_OK) {
		errorMsg = "AVI Compressed Stream format setting failed.";
		return hr;
	}

	return hr;
}


bool CAVIGenerator::InitEngine() {

	if (quitAVIgen) {
		// error in initVFW
		return false;
	}

	for (int i=0; i < 10; i++) {
		unsigned char* tmpBuf = new unsigned char[bitmapInfo.biSizeImage];
		freeImageBuffers.push_back(tmpBuf);
	}

	HWND mainWindow = FindWindow(NULL, ("Spring " + SpringVersion::GetFull()).c_str());
	if (globalRendering->fullScreen) {
		ShowWindow(mainWindow, SW_SHOWMINNOACTIVE);
	}
	boost::mutex::scoped_lock lock(AVIMutex);
	AVIThread = new boost::thread(boost::bind(&CAVIGenerator::AVIGeneratorThreadProc, this));
	AVICondition.wait(lock);  // Wait until InitAVICompressionEngine() completes.
	if (globalRendering->fullScreen) {
		ShowWindow(mainWindow, SW_RESTORE);
	}
	return !quitAVIgen;
}


HRESULT CAVIGenerator::AddFrame(unsigned char* pixelData){

	// compress bitmap
	HRESULT hr = AVIStreamWrite_ptr(m_pStreamCompressed,	// stream pointer
		m_lFrame,				// time of this frame
		1,						// number to write
		pixelData,				// image buffer
		bitmapInfo.biSizeImage,	// size of this frame
		AVIIF_KEYFRAME,			// flags....
		NULL,
		NULL);

	// updating frame counter
	m_lFrame++;

	return hr;
}


bool CAVIGenerator::readOpenglPixelDataThreaded() {

	while (true) {
		boost::mutex::scoped_lock lock(AVIMutex);
		if (quitAVIgen) {
			return false;
		} else if (readBuf != NULL) {
			imageBuffers.push_back(readBuf);
			readBuf = NULL;
			AVICondition.notify_all();
		}
		if (freeImageBuffers.empty()) {
			AVICondition.wait(lock);
		} else {
			readBuf = freeImageBuffers.front();
			freeImageBuffers.pop_front();
			break;
		}
	}

	glReadPixels(0, 0, bitmapInfo.biWidth, bitmapInfo.biHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, readBuf);
	return true;
}


void CAVIGenerator::AVIGeneratorThreadProc() {

	//Run init from the encoding thread because custom controls displayed by codecs
	//sends window messages to the thread it started from, thus deadlocking if
	//sending to the main thread whilst it is blocking on readOpenglPixelDataThreaded().
	quitAVIgen = InitAVICompressionEngine() != AVIERR_OK;
	AVICondition.notify_all();
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	unsigned char* localWriteBuf = 0;
	bool encoderError = false;

	while (true) {
		{
			boost::mutex::scoped_lock lock(AVIMutex);
			if (encoderError) {
				logOutput.Print("The avi generator terminated unexpectedly!");
				quitAVIgen = true;
				// Do not let the main thread wait, as the encoder will not
				// process and free the remaining content in imageBuffers.
				AVICondition.notify_all();
			}
			if (quitAVIgen) {
				break;
			}
			if (localWriteBuf != 0) {
				freeImageBuffers.push_back(localWriteBuf);
				localWriteBuf = 0;
				AVICondition.notify_all();
			}
			if (imageBuffers.empty()) {
				AVICondition.wait(lock);
				continue;
			}
			localWriteBuf = imageBuffers.front();
			imageBuffers.pop_front();
		}
		if (AddFrame(localWriteBuf)) {
			encoderError = true;
		}
		MSG msg;
		// Handle all messages the codec sends.
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (GetMessage(&msg, NULL, 0, 0) < 0) {
				encoderError = true;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	delete [] localWriteBuf;
}

#endif // defined AVI_CAPTURING

