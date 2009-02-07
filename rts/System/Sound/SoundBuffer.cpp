#include "SoundBuffer.h"

#include "LogOutput.h"
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

	alGenBuffers(1, &id);
	filename = file;
	alBufferData(id, format, &buffer[sizeof(WAVHeader)], header->datalen, header->SamplesPerSec);
	return true;
}

