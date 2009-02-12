#include "SoundBuffer.h"

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

#include "LogOutput.h"
#include "ALShared.h"
#include "Platform/errorhandler.h"
#include "Platform/byteorder.h"

SoundBuffer::SoundBuffer() : id(0)
{
}

SoundBuffer::~SoundBuffer()
{
}

#pragma pack(push, 1)
// Header copied from WavLib by Michael McTernan
struct WAVHeader
{
	uint8_t riff[4];         // "RIFF"
	int32_t totalLength;
	uint8_t wavefmt[8];      // WAVEfmt "
	int32_t length;         // Remaining length 4 bytes
	int16_t format_tag;
	int16_t channels;       // Mono=1 Stereo=2
	int32_t SamplesPerSec;
	int32_t AvgBytesPerSec;
	int16_t BlockAlign;
	int16_t BitsPerSample;
	uint8_t data[4];         // "data"
	int32_t datalen;        // Raw data length 4 bytes
};
#pragma pack(pop)

bool SoundBuffer::LoadWAV(const std::string& file, std::vector<uint8_t> buffer, bool strict)
{
	WAVHeader* header = (WAVHeader*)(&buffer[0]);

	if (memcmp(header->riff, "RIFF", 4) || memcmp(header->wavefmt, "WAVEfmt", 7)) {
		if (strict) {
			ErrorMessageBox("ReadWAV: invalid header.", file, 0);
		}
		return false;
	}

#define hswabword(c) header->c = swabword(header->c)
#define hswabdword(c) header->c = swabdword(header->c)
	hswabword(format_tag);
	hswabword(channels);
	hswabword(BlockAlign);
	hswabword(BitsPerSample);

	hswabdword(totalLength);
	hswabdword(length);
	hswabdword(SamplesPerSec);
	hswabdword(AvgBytesPerSec);
	hswabdword(datalen);
#undef hswabword
#undef hswabdword

	if (header->format_tag != 1) { // Microsoft PCM format?
		if (strict) {
			ErrorMessageBox("ReadWAV: invalid format tag.", file, 0);
		}
		return false;
	}

	ALenum format;
	if (header->channels == 1) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_MONO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_MONO16;
		else {
			if (strict) {
				ErrorMessageBox("ReadWAV: invalid number of bits per sample (mono).", file, 0);
			}
			return false;
		}
	}
	else if (header->channels == 2) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_STEREO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_STEREO16;
		else {
			if (strict) {
				ErrorMessageBox("ReadWAV: invalid number of bits per sample (stereo).", file, 0);
			}
			return false;
		}
	}
	else {
		if (strict) {
			ErrorMessageBox("ReadWAV (%s): invalid number of channels.", file, 0);
		}
		return false;
	}

	if (header->datalen > buffer.size() - sizeof(WAVHeader)) {
//		logOutput.Print("\n");
		logOutput.Print("OpenAL: file %s has data length %d greater than actual data length %ld\n",
						file.c_str(), header->datalen, buffer.size() - sizeof(WAVHeader));
//		logOutput.Print("OpenAL: size %d\n", size);
//		logOutput.Print("OpenAL: sizeof(WAVHeader) %d\n", sizeof(WAVHeader));
//		logOutput.Print("OpenAL: format_tag %d\n", header->format_tag);
//		logOutput.Print("OpenAL: channels %d\n", header->channels);
//		logOutput.Print("OpenAL: BlockAlign %d\n", header->BlockAlign);
//		logOutput.Print("OpenAL: BitsPerSample %d\n", header->BitsPerSample);
//		logOutput.Print("OpenAL: totalLength %d\n", header->totalLength);
//		logOutput.Print("OpenAL: length %d\n", header->length);
//		logOutput.Print("OpenAL: SamplesPerSec %d\n", header->SamplesPerSec);
//		logOutput.Print("OpenAL: AvgBytesPerSec %d\n", header->AvgBytesPerSec);

		// FIXME: setting datalen to size - sizeof(WAVHeader) only
		// works for some files that have a garbage datalen field
		// in their header, others cause SEGV's inside alBufferData()
		// (eg. ionbeam.wav in XTA 9.2) -- Kloot
		// header->datalen = size - sizeof(WAVHeader);
		header->datalen = 1;
	}

	AlGenBuffer(file, format, &buffer[sizeof(WAVHeader)], header->datalen, header->SamplesPerSec);
	return true;
}

struct VorbisInputBuffer
{
	uint8_t* data;
	size_t pos;
	size_t size;
};

size_t VorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	VorbisInputBuffer* buffer = (VorbisInputBuffer*)datasource;
	const size_t maxRead = std::max(size * nmemb, buffer->size - buffer->pos);
	memcpy(ptr, buffer->data + buffer->pos, maxRead);
	return maxRead;
}

int	VorbisSeek(void* datasource, ogg_int64_t offset, int whence)
{
	return -1; // not seekable
}

int	VorbisClose(void* datasource)
{
	return 0; // nothing to be done here
}

long VorbisTell(void* datasource)
{
	return ((reinterpret_cast<VorbisInputBuffer*>(datasource))->pos);
}

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

bool SoundBuffer::LoadVorbis(const std::string& file, std::vector<uint8_t> buffer, bool strict)
{
	VorbisInputBuffer buf;
	buf.data = &buffer[0];
	buf.pos = 0;
	buf.size = buffer.size();
	
	ov_callbacks vorbisCallbacks;
	vorbisCallbacks.read_func  = VorbisRead;
	vorbisCallbacks.close_func = VorbisClose;
	vorbisCallbacks.seek_func  = VorbisSeek;
	vorbisCallbacks.tell_func  = VorbisTell;

	OggVorbis_File oggStream;
	int result = 0;
	if ((result = ov_open_callbacks(&buf, &oggStream, NULL, 0, vorbisCallbacks)) < 0)
	{
		logOutput.Print("Could not open Ogg stream (reason: %s).", ErrorString(result).c_str());
		return false;
	}
	
	vorbis_info* vorbisInfo = ov_info(&oggStream, -1);
	// vorbis_comment* vorbisComment = ov_comment(&oggStream, -1);

	ALenum format;
	if (vorbisInfo->channels == 1) {
		if (vorbisInfo->rate == 8)
			format = AL_FORMAT_MONO8;
		else if (vorbisInfo->rate == 16)
			format = AL_FORMAT_MONO16;
		else
		{
			if (strict)
				ErrorMessageBox("SoundBuffer::LoadVorbis: invalid number of bits per sample (mono).", file, 0);
			return false;
		}
	}
	else if (vorbisInfo->channels == 2)
	{
		if (vorbisInfo->rate == 8)
			format = AL_FORMAT_STEREO8;
		else if (vorbisInfo->rate == 16)
			format = AL_FORMAT_STEREO16;
		else
		{
			if (strict)
				ErrorMessageBox("SoundBuffer::LoadVorbis: invalid number of bits per sample (stereo).", file, 0);
			return false;
		}
	}
	else
	{
		if (strict)
			ErrorMessageBox("SoundBuffer::LoadVorbis (%s): invalid number of channels.", file, 0);
		return false;
	}

	std::vector<uint8_t> decodeBuffer(ov_pcm_total(&oggStream, -1));
	int section = 0;
	long read = ov_read(&oggStream, (char*)&decodeBuffer[0], decodeBuffer.size(), 0, 2, 1, &section);
	LogObject() << "Read " << read << " bytes from vorbis sample";

	AlGenBuffer(file, format, &decodeBuffer[0], decodeBuffer.size(), vorbisInfo->rate);
	return true;
}

int SoundBuffer::BufferSize() const
{
	ALint size;
	alGetBufferi(id, AL_SIZE, &size);
	return static_cast<int>(size);
}

void SoundBuffer::AlGenBuffer(const std::string& file, ALenum format, const uint8_t* data, size_t datalength, int rate)
{
	alGenBuffers(1, &id);
	filename = file;
	alBufferData(id, format, data, datalength, rate);
	CheckError("SoundBuffer::AlGenBuffer");
}
