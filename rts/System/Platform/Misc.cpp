#include "Misc.h"

#include <sstream>

#ifdef linux
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#elif WIN32
#include <io.h>
//#include <direct.h>
//#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
//#ifndef SHGFP_TYPE_CURRENT
//#define SHGFP_TYPE_CURRENT 0
//#endif

#elif MACOSX_BUNDLE
#include <CoreFoundation/CoreFoundation.h>

#else

#endif

namespace Platform
{

std::string GetBinaryPath()
{
#ifdef linux
	char file[256];
	const int ret = readlink("/proc/self/exe", file, 255);
	if (ret >= 0)
	{
		file[ret] = '\0';
		std::string path(file);
		return path.substr(0, path.find_last_of('/'));
	}
	else
		return "";

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

}

