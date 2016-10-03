/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SoundBuffer.h"


#include "System/Sound/SoundLog.h"
#include "ALShared.h"
#include "VorbisShared.h"
#include "System/Platform/byteorder.h"

#include <vorbis/vorbisfile.h>
#include <ogg/ogg.h>
#include <cstring>

namespace
{
struct VorbisInputBuffer
{
	std::uint8_t* data;
	size_t pos;
	size_t size;
};

size_t VorbisRead(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	VorbisInputBuffer* buffer = static_cast<VorbisInputBuffer*>(datasource);
	const size_t maxRead = std::min(size * nmemb, buffer->size - buffer->pos);
	memcpy(ptr, buffer->data + buffer->pos, maxRead);
	buffer->pos += maxRead;
	return maxRead;
}

int	VorbisClose(void* datasource)
{
	return 0; // nothing to be done here
}
}

SoundBuffer::bufferMapT SoundBuffer::bufferMap; // filename, index into Buffers
SoundBuffer::bufferVecT SoundBuffer::buffers;

SoundBuffer::SoundBuffer() : id(0), channels(0), length(0.0f)
{
}

SoundBuffer::~SoundBuffer()
{
}

#pragma pack(push, 1)
// Header copied from WavLib by Michael McTernan
struct WAVHeader
{
	std::uint8_t riff[4];         // "RIFF"
	std::int32_t totalLength;
	std::uint8_t wavefmt[8];      // WAVEfmt "
	std::int32_t length;         // Remaining length 4 bytes
	std::int16_t format_tag;
	std::int16_t channels;       // Mono=1 Stereo=2
	std::int32_t SamplesPerSec;
	std::int32_t AvgBytesPerSec;
	std::int16_t BlockAlign;
	std::int16_t BitsPerSample;
	std::uint8_t data[4];         // "data"
	std::int32_t datalen;        // Raw data length 4 bytes
};
#pragma pack(pop)

bool SoundBuffer::LoadWAV(const std::string& file, std::vector<std::uint8_t> buffer)
{
	WAVHeader* header = (WAVHeader*)(&buffer[0]);

	if ((buffer.empty()) || memcmp(header->riff, "RIFF", 4) || memcmp(header->wavefmt, "WAVEfmt", 7)) {
		LOG_L(L_ERROR, "ReadWAV: invalid header: %s", file.c_str());
		return false;
	}

#define hswabword(c) swabWordInPlace(header->c)
#define hswabdword(c) swabDWordInPlace(header->c)
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
		LOG_L(L_ERROR, "ReadWAV (%s): invalid format tag", file.c_str());
		return false;
	}

	ALenum format;
	if (header->channels == 1) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_MONO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_MONO16;
		else {
			LOG_L(L_ERROR, "ReadWAV (%s): invalid number of bits per sample (mono)", file.c_str());
			return false;
		}
	}
	else if (header->channels == 2) {
		if (header->BitsPerSample == 8) format = AL_FORMAT_STEREO8;
		else if (header->BitsPerSample == 16) format = AL_FORMAT_STEREO16;
		else {
			LOG_L(L_ERROR, "ReadWAV (%s): invalid number of bits per sample (stereo)", file.c_str());
			return false;
		}
	}
	else {
		LOG_L(L_ERROR, "ReadWAV (%s): invalid number of channels.", file.c_str());
		return false;
	}

	if (static_cast<unsigned>(header->datalen) > buffer.size() - sizeof(WAVHeader)) {
		LOG_L(L_ERROR,
				"WAV file %s has data length %i greater than actual data length %i",
				file.c_str(), header->datalen,
				(int)(buffer.size() - sizeof(WAVHeader)));

//		LOG_L(L_WARNING, "OpenAL: size %d\n", size);
//		LOG_L(L_WARNING, "OpenAL: sizeof(WAVHeader) %d\n", sizeof(WAVHeader));
//		LOG_L(L_WARNING, "OpenAL: format_tag %d\n", header->format_tag);
//		LOG_L(L_WARNING, "OpenAL: channels %d\n", header->channels);
//		LOG_L(L_WARNING, "OpenAL: BlockAlign %d\n", header->BlockAlign);
//		LOG_L(L_WARNING, "OpenAL: BitsPerSample %d\n", header->BitsPerSample);
//		LOG_L(L_WARNING, "OpenAL: totalLength %d\n", header->totalLength);
//		LOG_L(L_WARNING, "OpenAL: length %d\n", header->length);
//		LOG_L(L_WARNING, "OpenAL: SamplesPerSec %d\n", header->SamplesPerSec);
//		LOG_L(L_WARNING, "OpenAL: AvgBytesPerSec %d\n", header->AvgBytesPerSec);

		header->datalen = std::uint32_t(buffer.size() - sizeof(WAVHeader))&(~std::uint32_t((header->BitsPerSample*header->channels)/8 -1));
	}

	if (!AlGenBuffer(file, format, &buffer[sizeof(WAVHeader)], header->datalen, header->SamplesPerSec)) {
		LOG_L(L_WARNING, "Loading audio failed for %s", file.c_str());
	}

	filename = file;
	channels = header->channels;
	length   = float(header->datalen) / (header->channels * header->SamplesPerSec * header->BitsPerSample);

	return true;
}

bool SoundBuffer::LoadVorbis(const std::string& file, std::vector<std::uint8_t> buffer)
{
	VorbisInputBuffer buf;
	buf.data = &buffer[0];
	buf.pos = 0;
	buf.size = buffer.size();

	ov_callbacks vorbisCallbacks;
	vorbisCallbacks.read_func  = VorbisRead;
	vorbisCallbacks.close_func = VorbisClose;
	vorbisCallbacks.seek_func  = NULL;
	vorbisCallbacks.tell_func  = NULL;

	OggVorbis_File oggStream;
	const int result = ov_open_callbacks(&buf, &oggStream, NULL, 0, vorbisCallbacks);
	if (result < 0) {
		LOG_L(L_WARNING, "Could not open Ogg stream (reason: %s).",
				ErrorString(result).c_str());
		return false;
	}

	vorbis_info* vorbisInfo = ov_info(&oggStream, -1);
	// vorbis_comment* vorbisComment = ov_comment(&oggStream, -1);

	ALenum format;
	if (vorbisInfo->channels == 1)
	{
		format = AL_FORMAT_MONO16;
	}
	else if (vorbisInfo->channels == 2)
	{
		format = AL_FORMAT_STEREO16;
	}
	else
	{
		LOG_L(L_ERROR, "File %s: invalid number of channels: %i",
					file.c_str(), vorbisInfo->channels);
		return false;
	}

	size_t pos = 0;
	std::vector<std::uint8_t> decodeBuffer(512*1024); // 512kb read buffer
	int section = 0;
	long read = 0;
	do
	{
		if (4*pos > 3*decodeBuffer.size()) // enlarge buffer so ov_read has enough space
			decodeBuffer.resize(decodeBuffer.size()*2);
		read = ov_read(&oggStream, (char*)&decodeBuffer[pos], decodeBuffer.size() - pos, 0, 2, 1, &section);
		switch(read)
		{
			case OV_HOLE:
				LOG_L(L_WARNING,
						"%s: garbage or corrupt page in stream (non-fatal)",
						file.c_str());
				continue; // read next
			case OV_EBADLINK:
				LOG_L(L_WARNING, "%s: corrupted stream", file.c_str());
				return false; // abort
			case OV_EINVAL:
				LOG_L(L_WARNING, "%s: corrupted headers", file.c_str());
				return false; // abort
			default:
				break; // all good
		};
		pos += read;
	} while (read > 0); // read == 0 indicated EOF, read < 0 is error

	AlGenBuffer(file, format, &decodeBuffer[0], pos, vorbisInfo->rate);
	filename = file;
	channels = vorbisInfo->channels;
	length   = ov_time_total(&oggStream, -1);
	return true;
}

int SoundBuffer::BufferSize() const
{
	ALint size;
	alGetBufferi(id, AL_SIZE, &size);
	return static_cast<int>(size);
}

void SoundBuffer::Initialise()
{
	buffers.resize(1); // empty ("zero") buffer
}

void SoundBuffer::Deinitialise()
{
	bufferMap.clear();
	buffers.clear();
}

size_t SoundBuffer::GetId(const std::string& name)
{
	bufferMapT::const_iterator it = bufferMap.find(name);
	if (it != bufferMap.end())
		return it->second;
	else
		return 0;
}

boost::shared_ptr<SoundBuffer> SoundBuffer::GetById(const size_t id)
{
	assert(id < buffers.size());
	return buffers.at(id);
}

size_t SoundBuffer::Count()
{
	return buffers.size();
}

size_t SoundBuffer::AllocedSize()
{
	int numBytes = 0;
	for (bufferVecT::const_iterator it = ++buffers.begin(); it != buffers.end(); ++it)
		numBytes += (*it)->BufferSize();
	return numBytes;
}

size_t SoundBuffer::Insert(boost::shared_ptr<SoundBuffer> buffer)
{
	size_t bufId = buffers.size();
	buffers.push_back(buffer);
	bufferMap[buffer->GetFilename()] = bufId;
	return bufId;
}

bool SoundBuffer::AlGenBuffer(const std::string& file, ALenum format, const std::uint8_t* data, size_t datalength, int rate)
{
	alGenBuffers(1, &id);
	if (!CheckError("SoundBuffer::AlGenBuffers"))
		return false;
	alBufferData(id, format, (ALvoid*) data, datalength, rate);
	return CheckError("SoundBuffer::AlGenBufferData");
}
