/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AVI_GENERATOR_H
#define AVI_GENERATOR_H

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/utility.hpp>

#include <windows.h>
#include <vfw.h>

#include <string>
#include <list>


class CAVIGenerator : boost::noncopyable {
public:

	CAVIGenerator(const std::string& fileName, int videoSizeX, int videoSizeY, DWORD videoFPS);
	~CAVIGenerator();

	/// Initialize engine and choose codec
	bool InitEngine();

	/// Returns last error message
	std::string GetLastErrorMessage() const	{return errorMsg;}

	bool readOpenglPixelDataThreaded();


private:
	/// name of output file
	std::string fileName;
	/// Frame rate
	DWORD videoFPS;
	/// structure contains information for a single stream
	BITMAPINFOHEADER bitmapInfo;
	/// last error string
	std::string errorMsg;

	HINSTANCE msvfw32;
	HINSTANCE avifil32;

	volatile bool quitAVIgen;

	boost::thread* AVIThread;
	boost::mutex AVIMutex;
	boost::condition AVICondition;


	std::list<unsigned char*> freeImageBuffers;
	std::list<unsigned char*> imageBuffers;

	unsigned char* readBuf;



	bool initVFW();

	HRESULT InitAVICompressionEngine();

	/// Release streams allocated for movie compression.
	void ReleaseAVICompressionEngine();

	/// Adds a frame to the movie.
	HRESULT AddFrame(unsigned char* pixelData);

	void AVIGeneratorThreadProc();


	/// frame counter
	long m_lFrame;
	/// file interface pointer
	PAVIFILE m_pAVIFile;
	/// Address of the stream interface
	PAVISTREAM m_pStream;
	/// Address of the compressed video stream
	PAVISTREAM m_pStreamCompressed;
	/// Holds compression settings
	COMPVARS cv;

	typedef DWORD (__stdcall *VideoForWindowsVersion_type)(void);
	typedef void (__stdcall *AVIFileInit_type)(void);
	typedef HRESULT (__stdcall *AVIFileOpenA_type)(PAVIFILE FAR *, LPCSTR, UINT, LPCLSID);
	typedef HRESULT (__stdcall *AVIFileCreateStreamA_type)(PAVIFILE, PAVISTREAM FAR *, AVISTREAMINFOA FAR *);
	typedef HRESULT (__stdcall *AVIMakeCompressedStream_type)(PAVISTREAM FAR *, PAVISTREAM, AVICOMPRESSOPTIONS FAR *, CLSID FAR *);
	typedef HRESULT (__stdcall *AVIStreamSetFormat_type)(PAVISTREAM, LONG, LPVOID, LONG);
	typedef ULONG (__stdcall *AVIStreamRelease_type)(PAVISTREAM);
	typedef ULONG (__stdcall *AVIFileRelease_type)(PAVIFILE);
	typedef void (__stdcall *AVIFileExit_type)(void);
	typedef HRESULT (__stdcall *AVIStreamWrite_type)(PAVISTREAM, LONG, LONG, LPVOID, LONG, DWORD, LONG FAR *, LONG FAR *);
	typedef BOOL (__stdcall *ICCompressorChoose_type)(HWND, UINT, LPVOID, LPVOID, PCOMPVARS, LPSTR);
	typedef void (__stdcall *ICCompressorFree_type)(PCOMPVARS);
	typedef HIC (__stdcall *ICOpen_type)(DWORD, DWORD, UINT);



	VideoForWindowsVersion_type VideoForWindowsVersion_ptr;
	AVIFileInit_type AVIFileInit_ptr;
	AVIFileOpenA_type AVIFileOpenA_ptr;
	AVIFileCreateStreamA_type AVIFileCreateStreamA_ptr;
	AVIMakeCompressedStream_type AVIMakeCompressedStream_ptr;
	AVIStreamSetFormat_type AVIStreamSetFormat_ptr;
	AVIStreamRelease_type AVIStreamRelease_ptr;
	AVIFileRelease_type AVIFileRelease_ptr;
	AVIFileExit_type AVIFileExit_ptr;
	AVIStreamWrite_type AVIStreamWrite_ptr;
	ICCompressorChoose_type ICCompressorChoose_ptr;
	ICCompressorFree_type ICCompressorFree_ptr;
	ICOpen_type ICOpen_ptr;

};

#endif /* AVI_GENERATOR_H */
