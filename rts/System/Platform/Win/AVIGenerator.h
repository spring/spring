// AVIGenerator.h: interface for the CAVIGenerator class.
//
// A class to easily create AVI
//
// Original code : Example code in WriteAvi.c of MSDN
// 
// Author : Jonathan de Halleux. dehalleux@auto.ucl.ac.be
//////////////////////////////////////////////////////////////////////

#ifndef AVIGENERATOR_H
#define AVIGENERATOR_H


#include <windows.h>
#include <vfw.h>

#include <GL/gl.h>
#if defined(_WIN32) && defined(__MINGW32__)
#include <GL/glext.h>
#endif

#include <string>


/*! \brief A simple class to create AVI video stream.


\par Usage

Step 1 : Declare an CAVIGenerator object
Step 2 : Set Bitmap by calling SetBitmapHeader functions + other parameters
Step 3 : Initialize engine by calling InitEngine
Step 4 : Send each frames to engine with function AddFrame
Step 5 : Close engine by calling ReleaseEngine

\par Demo Code:

\code
CAVIGenerator AviGen;
BYTE* bmBits;

// set characteristics
AviGen.SetRate(20);							// set 20fps
AviGen.SetBitmapHeader(GetActiveView());	// give info about bitmap

AviGen.InitEngine();

..... // Draw code, bmBits is the buffer containing the frame
AviGen.AddFrame(bmBits);
.....

AviGen.ReleaseEngine();
\endcode

\par Update history:

- {\bf 22-10-2002} Minor changes in constructors.

\author : Jonathan de Halleux, dehalleux@auto.ucl.ac.be (2001)
*/
class CAVIGenerator  
{
public:
	//! \name Constructors and destructors
	//@{
	//! Inplace constructor with BITMAPINFOHEADER
	CAVIGenerator(const std::string & sFileName, LPBITMAPINFOHEADER lpbih, DWORD dwRate);
	~CAVIGenerator();
	//@}

	//! \name  AVI engine function
	//@{
	/*! \brief  Initialize engine and choose codex
	Some asserts are made to check that bitmap has been properly initialized
	*/
	HRESULT InitEngine();

	/*! \brief Adds a frame to the movie. 

	The data pointed by bmBits has to be compatible with the bitmap description of the movie.
	*/
	HRESULT AddFrame(BYTE* bmBits);
	//! Release ressources allocated for movie and close file.
	void ReleaseEngine();
	//@}

	//! \name Setters and getters
	//@{
	//! returns a pointer to bitmap info
	LPBITMAPINFOHEADER GetBitmapHeader() {return &m_bih;}

	//! \name Error handling
	//@{
	//! returns last  error message
	std::string GetLastErrorMessage() const	{return m_sError;}
	//@}
	unsigned char* GetPixelBuf(){return pixelDataBuf;}


private:
	//! name of output file
	std::string m_sFile;			
	//! Frame rate 
	DWORD m_dwRate;	
	//! structure contains information for a single stream
	BITMAPINFOHEADER m_bih;	
	//! last error string
	std::string m_sError;

	unsigned char* pixelDataBuf;


	//! Sets bitmap info as in lpbih
	void SetBitmapHeader(LPBITMAPINFOHEADER lpbih);

	void MakeExtAvi();
	//! frame counter
	long m_lFrame;
	//! file interface pointer
	PAVIFILE m_pAVIFile;
	//! Address of the stream interface
	PAVISTREAM m_pStream;		
	//! Address of the compressed video stream
	PAVISTREAM m_pStreamCompressed; 
	//Holds compression settings
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


	bool initVFW();


	struct triggerCleanup{
		triggerCleanup(CAVIGenerator*  ptr) : clean(true), ptr(ptr){};
		~triggerCleanup(){
			if(clean){
				ptr->ReleaseEngine();
			}
		}
		void disarm(){
			clean = false;
		}
	private:
		bool clean;
		CAVIGenerator* ptr;
	};

};

#endif /* AVIGENERATOR_H */
