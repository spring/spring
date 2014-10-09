/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VorbisShared.h"

#include <vorbis/vorbisfile.h>
#include <string.h>

std::string ErrorString(int code)
{
	switch (code) {
		case OV_EREAD:
			return std::string("Read from media.");
		case OV_ENOTVORBIS:
			return std::string("Not Vorbis data.");
		case OV_EVERSION:
			return std::string("Vorbis version mismatch.");
		case OV_EBADHEADER:
			return std::string("Invalid Vorbis header.");
		case OV_EFAULT:
			return std::string("Internal logic fault (bug or heap/stack corruption.");
		default:
			return std::string("Unknown Ogg error.");
	}
}
