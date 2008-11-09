/*
 * @file MacFileSystemHandler.cpp
 * @author Fabian Herb
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later
 */

#include <CoreFoundation/CoreFoundation.h>
#include "StdAfx.h"
#include "LogOutput.h"
#include "MacFileSystemHandler.h"

/**
 * This does exactly the same as the constructor of UnixFileSystemHandler, but unfortunately
 * simply making LocateDataDirs() virtual did not work, it would not call the correct one.
 */
MacFileSystemHandler::MacFileSystemHandler(bool verbose) : UnixFileSystemHandler(verbose, false)
{
	LocateDataDirs();
	InitVFS();
	
	for (std::vector<DataDir>::const_iterator d = datadirs.begin(); d != datadirs.end(); ++d) {
		if (d->readable) {
			if (d->writable)
				logOutput.Print("Using read-write data directory: %s", d->path.c_str());
			else
				logOutput.Print("Using read-only  data directory: %s", d->path.c_str());
		}
	}
}

void MacFileSystemHandler::LocateDataDirs()
{
	// Get the path to the application bundle we are running:
	char cPath[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if(!mainBundle)
	    throw content_error("Could not determine bundle path");

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	if(!mainBundleURL)
		 throw content_error("Could not determine bundle path");

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
	if(!cfStringRef)
		 throw content_error("Could not determine bundle path");

    CFStringGetCString(cfStringRef, cPath, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);
	std::string path(cPath);
	
	datadirs.clear();
	writedir = NULL;
	
	// Add bundle resources:
	datadirs.push_back(path + "/Contents/Resources/");
	datadirs.rbegin()->readable = true;
	// Add the directory surrounding the bundle, for users to add mods and maps in:
	datadirs.push_back(filesystem.GetDirectory(path));
	// Use surrounding directory as writedir for now, should propably
	// change this to something inside the home directory:
	datadirs.rbegin()->writable = true;
	datadirs.rbegin()->readable = true;
	writedir = &*datadirs.rbegin();
	
	// See UnixFileSystemHandler::LocateDataDirs() on why we change the working
	// directory here:
	chdir(GetWriteDir().c_str());
}
