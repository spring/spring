#include "Misc.h"

#ifdef linux
#include <unistd.h>

#elif WIN32
#include <io.h>
#include <shlobj.h>
#include <shlwapi.h>

#elif MACOSX_BUNDLE
#include <CoreFoundation/CoreFoundation.h>

#else

#endif

namespace Platform
{

std::string GetBinaryPath()
{
#ifdef linux
	std::string path(GetBinaryFile());
	size_t pathlength = path.find_last_of('/');
	if (pathlength != std::string::npos)
		return path.substr(0, pathlength);
	else
		return path;

#elif WIN32
	TCHAR currentDir[MAX_PATH+1];
	int ret = ::GetModuleFileName(0, currentDir, sizeof(currentDir));
	if (ret == 0 || ret == sizeof(currentDir))
		return "";
	char drive[MAX_PATH], dir[MAX_PATH], file[MAX_PATH], ext[MAX_PATH];
	_splitpath(currentDir, drive, dir, file, ext);
	_makepath(currentDir, drive, dir, NULL, NULL);
	return std::string(currentDir);

#elif MACOSX_BUNDLE
	char cPath[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	
	CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	CFURLRef binaryPathURL = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault , mainBundleURL);
	
	CFStringRef cfStringRef = CFURLCopyFileSystemPath(binaryPathURL, kCFURLPOSIXPathStyle);
	
	CFStringGetCString(cfStringRef, cPath, 1024, kCFStringEncodingASCII);
	
	CFRelease(mainBundleURL);
	CFRelease(binaryPathURL);
	CFRelease(cfStringRef);
	
	return std::string(cPath);

#else
	return "";

#endif
}


std::string GetBinaryFile()
{
#ifdef linux
	char file[256];
	const int ret = readlink("/proc/self/exe", file, 255);
	if (ret >= 0)
	{
		file[ret] = '\0';
		return std::string(file);
	}
	else
		return "";

#elif WIN32
	TCHAR currentDir[MAX_PATH+1];
	int ret = ::GetModuleFileName(0, currentDir, sizeof(currentDir));
	if (ret == 0 || ret == sizeof(currentDir))
		return "";
	return std::string(currentDir);

#elif MACOSX_BUNDLE
	//TODO
	return "";
#else
	return "";

#endif
}

std::string GetOS()
{
#if defined(WIN32)
	return "Microsoft Windows";
#elif defined(__linux__)
	return "Linux";
#elif defined(__FreeBSD__)
	return "FreeBSD";
#elif defined(__APPLE__)
	return "Mac OS X";
#else
	return "unknown OS";
#endif
}

bool Is64Bit()
{
	return (sizeof(void*) == 8);
}

#ifdef WIN32
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

/** @brief checks if the current process is running in 32bit emulation mode
    @return FALSE, TRUE, -1 on error (usually no permissions) */
bool Is32BitEmulation()
{
	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
		{
			return false;
		}
	}
	return bIsWow64;
}
#else
// simply assume other OS doesn't need 32bit emulation
bool Is32BitEmulation()
{
	return false;
}
#endif

}

