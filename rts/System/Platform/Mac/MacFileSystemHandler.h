/*
 * @file MacFileSystemHandler.h
 * @author Fabian Herb
 * @brief Abstracts locating of content on different platforms
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later
 *
 */
 
#ifndef MACFILESYSTEMHANDLER_H
#define MACFILESYSTEMHANDLER_H

#include "Platform/Linux/UnixFileSystemHandler.h"

/// Mac file system handler, configures mac-specific search paths
class MacFileSystemHandler : public UnixFileSystemHandler
{
public:
	MacFileSystemHandler(bool verbose);
protected:
	/// Adds the application bundle resources and the surrounding dir to the searchtree
	void LocateDataDirs();
};

#endif
