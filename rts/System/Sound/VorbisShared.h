#ifndef VORBIS_SHARED
#define VORBIS_SHARED

#include <vorbis/vorbisfile.h>
#include <string>

std::string ErrorString(int code);

struct VorbisInputBuffer
{
	uint8_t* data;
	size_t pos;
	size_t size;
};

size_t VorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource);
int	VorbisSeek(void* datasource, ogg_int64_t offset, int whence);
int	VorbisClose(void* datasource);
long VorbisTell(void* datasource);

#endif
