#include "VorbisShared.h"

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
};

size_t VorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	VorbisInputBuffer* buffer = (VorbisInputBuffer*)datasource;
	const size_t maxRead = std::max(size * nmemb, buffer->size - buffer->pos);
	memcpy(ptr, buffer->data + buffer->pos, maxRead);
	return maxRead;
};

int	VorbisSeek(void* datasource, ogg_int64_t offset, int whence)
{
	VorbisInputBuffer* buffer = (VorbisInputBuffer*)datasource;
	switch (whence) {
		case SEEK_SET:
			buffer->pos = offset;
			break;
		case SEEK_CUR:
			buffer->pos += offset;
			break;
		case SEEK_END:
			buffer->pos = buffer->size + offset;
			break;
	}
	return 0;
};

int	VorbisClose(void* datasource)
{
	return 0; // nothing to be done here
};

long VorbisTell(void* datasource)
{
	return ((reinterpret_cast<VorbisInputBuffer*>(datasource))->pos);
};
