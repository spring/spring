/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "GZFileHandler.h"

#include <cassert>
#include <string>
#include <zlib.h>

#include "FileQueryFlags.h"
#include "FileSystem.h"


#ifndef TOOLS
	#include "DataDirsAccess.h"
	#include "System/StringUtil.h"
	#include "System/Platform/Misc.h"
#endif

#define BUFFER_SIZE 8192


//We must call Open from here since in the CFileHandler ctor
//virtual functions aren't called.
CGZFileHandler::CGZFileHandler(const char* fileName, const char* modes)
{
	Open(fileName, modes);
}


CGZFileHandler::CGZFileHandler(const std::string& fileName, const std::string& modes)
{
	Open(fileName, modes);
}


bool CGZFileHandler::ReadToBuffer(const std::string& path)
{
	assert(fileBuffer.empty());

	gzFile file = gzopen(path.c_str(), "rb");
	if (file == Z_NULL)
		return false;

	std::uint8_t unzipBuffer[BUFFER_SIZE];

	while (true) {
		int unzippedBytes = gzread(file, unzipBuffer, BUFFER_SIZE);
		if (unzippedBytes < 0) {
			fileBuffer.clear();
			fileSize = -1;
			gzclose(file);
			return false;
		}
		if (unzippedBytes == 0)
			break;
		fileBuffer.insert(fileBuffer.end(), unzipBuffer, unzipBuffer + unzippedBytes);
	}
	gzclose(file);

	fileSize = fileBuffer.size();
	return true;
}

bool CGZFileHandler::UncompressBuffer()
{
	std::vector<std::uint8_t> compressed;
	std::swap(compressed, fileBuffer);


	z_stream zstream;
	zstream.opaque = Z_NULL;
	zstream.zalloc = Z_NULL;
	zstream.zfree  = Z_NULL;
	zstream.data_type = Z_BINARY;

	//+16 marks it's a gzip header
	inflateInit2(&zstream, 15 + 16);

	zstream.next_in   = &compressed[0];
	zstream.avail_in  = compressed.size();

	std::uint8_t unzipBuffer[BUFFER_SIZE];

	while (true) {
		zstream.avail_out = BUFFER_SIZE;
		zstream.next_out = unzipBuffer;
		const int ret = inflate(&zstream, Z_NO_FLUSH);
		if (ret != Z_OK) {
			fileBuffer.clear();
			fileSize = -1;
			return false;
		}

		const size_t unzippedBytes = BUFFER_SIZE - zstream.avail_out;
		fileBuffer.insert(fileBuffer.end(), unzipBuffer, unzipBuffer + unzippedBytes);

		if (ret == Z_STREAM_END)
			break;
	}

	inflateEnd(&zstream);


	fileSize = fileBuffer.size();
	return true;
}


bool CGZFileHandler::TryReadFromPWD(const std::string& fileName)
{
#ifndef TOOLS
	if (FileSystem::IsAbsolutePath(fileName))
		return false;
	const std::string fullpath(Platform::GetOrigCWD() + fileName);
#else
	const std::string fullpath(fileName);
#endif
	return ReadToBuffer(fullpath);
}


bool CGZFileHandler::TryReadFromRawFS(const std::string& fileName)
{
#ifndef TOOLS
	const std::string rawpath = dataDirsAccess.LocateFile(fileName);
	return ReadToBuffer(rawpath);
#else
	return false;
#endif
}


bool CGZFileHandler::TryReadFromVFS(const std::string& fileName, int section)
{
	return CFileHandler::TryReadFromVFS(fileName, section) && UncompressBuffer();
}