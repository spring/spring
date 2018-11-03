/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StringUtil.h"
#include "System/bitops.h"

#include <zlib.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1310)
	#include <intrin.h>
#endif

#include <cstring>
#include <cinttypes>


std::string StringReplace(const std::string& text,
                          const std::string& from,
                          const std::string& to)
{
	std::string working = text;
	std::string::size_type pos = 0;
	while (true) {
		if ((pos = working.find(from, pos)) == std::string::npos)
			break;

		std::string tmp = working.substr(0, pos);
		tmp += to;
		tmp += working.substr(pos + from.size(), std::string::npos);
		pos += to.size();
		working = tmp;
	}
	return working;
}

std::string StringStrip(const std::string& str, const std::string& chars)
{
	std::string ret;
	ret.reserve(str.size());

	for (const char c: str) {
		if (chars.find(c) != std::string::npos)
			continue;

		ret.push_back(c);
	}

	return ret;
}



/// @see http://www.codeproject.com/KB/stl/stdstringtrim.aspx
void StringTrimInPlace(std::string& str, const std::string& ws)
{
	std::string::size_type pos = str.find_last_not_of(ws);
	if (pos != std::string::npos) {
		str.erase(pos + 1);

		if ((pos = str.find_first_not_of(ws)) != std::string::npos)
			str.erase(0, pos);

		return;
	}

	str.erase(str.begin(), str.end());
}

std::string StringTrim(const std::string& str, const std::string& ws)
{
	std::string copy(str);
	StringTrimInPlace(copy, ws);
	return copy;
}

bool StringToBool(std::string str)
{
	StringTrimInPlace(str);
	StringToLowerInPlace(str);

	// regex would probably be more appropriate,
	// but it is better not to rely on any external lib here
	if (str.empty()) return false;
	if (str ==     "0") return false;
	if (str ==     "n") return false;
	if (str ==    "no") return false;
	if (str ==     "f") return false;
	if (str == "false") return false;
	if (str ==   "off") return false;

	return true;
}


bool StringStartsWith(const std::string& str, const char* prefix)
{
	if ((prefix == nullptr) || (str.size() < strlen(prefix)))
		return false;

	return (str.compare(0, strlen(prefix), prefix) == 0);
}

bool StringEndsWith(const std::string& str, const char* postfix)
{
	if ((postfix == nullptr) || (str.size() < strlen(postfix)))
		return false;

	return (str.compare(str.size() - strlen(postfix), str.size(), postfix) == 0);
}


void InverseOrSetBool(bool& b, const std::string& argValue, const bool inverseArg)
{
	if (argValue.empty()) {
		// toggle
		b = !b;
	} else {
		// set
		b = inverseArg? (!StringToBool(argValue)) : (StringToBool(argValue));
	}
}


char32_t utf8::GetNextChar(const std::string& text, int& pos)
{
	// UTF8 looks like this
	// 1Byte == ASCII:      0xxxxxxxxx
	// 2Bytes encoded char: 110xxxxxxx 10xxxxxx
	// 3Bytes encoded char: 1110xxxxxx 10xxxxxx 10xxxxxx
	// 4Bytes encoded char: 11110xxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	// Originaly there were 5&6 byte versions too, but they were dropped in RFC 3629.
	// So UTF8 maps to UTF16 range only.

	static constexpr auto UTF8_CONT_MASK = 0xC0; // 11xxxxxx
	static constexpr auto UTF8_CONT_OKAY = 0x80; // 10xxxxxx

	union UTF8_4Byte {
		std::uint32_t i;
		std::uint8_t c[4];
	};

	// read next 4bytes and check if it is an utf8 sequence
	UTF8_4Byte utf8 = { 0 };
	const int remainingChars = text.length() - pos;
	if (remainingChars >= 4) {
		// we need to use memcpy cause text[pos] isn't memory aligned as ints need to be
		memcpy(&utf8.i, &text[pos], sizeof(std::uint32_t));
	} else {
		// read ahead of end of string
		if (remainingChars <= 0)
			return 0;

		// end of string reached, only read till end
		switch (remainingChars) {
			case 3: utf8.c[2] = std::uint8_t(text[pos + 2]);
			case 2: utf8.c[1] = std::uint8_t(text[pos + 1]);
			case 1: utf8.c[0] = std::uint8_t(text[pos    ]);
			default: {}
		};
	}

	// how many bytes are requested for our multi-byte utf8 sequence
	unsigned clo = count_leading_ones(utf8.c[0]);
	if (clo>4 || clo==0) clo = 1; // ignore >=5 byte ones cause of RFC 3629

	// how many healthy utf8 bytes are following
	unsigned numValidUtf8Bytes = 1; // first char is always valid
	numValidUtf8Bytes += int((utf8.c[1] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);
	numValidUtf8Bytes += int((utf8.c[2] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);
	numValidUtf8Bytes += int((utf8.c[3] & UTF8_CONT_MASK) == UTF8_CONT_OKAY);

	// check if enough trailing utf8 bytes are healthy
	// else ignore utf8 and parse it as 8bit Latin-1 char (extended ASCII)
	// this adds backwardcompatibility with the old renderer
	// which supported extended ASCII with umlauts etc.
	const auto usedUtf8Bytes = (clo <= numValidUtf8Bytes) ? clo : 1u;

	char32_t u = 0;
	switch (usedUtf8Bytes) {
		case 0:
		case 1: {
			u  = utf8.c[0];
		} break;
		case 2: {
			u  = (char32_t(utf8.c[0] & 0x1F)) << 6;
			u |= (char32_t(utf8.c[1] & 0x3F));
		} break;
		case 3: {
			u  = (char32_t(utf8.c[0] & 0x0F)) << 12;
			u |= (char32_t(utf8.c[1] & 0x3F)) <<  6;
			u |= (char32_t(utf8.c[2] & 0x3F));
		} break;
		case 4: {
			u  = (char32_t(utf8.c[0] & 0x07)) << 18;
			u |= (char32_t(utf8.c[1] & 0x3F)) << 12;
			u |= (char32_t(utf8.c[2] & 0x3F)) <<  6;
			u |= (char32_t(utf8.c[3] & 0x3F));
			//TODO limit range to UTF16!
		} break;
	}
	pos += usedUtf8Bytes;

	// replace tabs with spaces
	if (u == 0x9)
		u = 0x2007;

	return u;
}


std::string utf8::FromUnicode(char32_t ch)
{
	std::string str;

	// in:  0000 0000  0000 0000  0000 0000  0aaa aaaa
	// out:                                  0aaa aaaa
	if (ch < (1 << 7)) {
		str += (char)ch;
	}

	// in:  0000 0000  0000 0000  0000 0bbb  bbaa aaaa
	// out:                       110b bbbb  10aa aaaa
	else if (ch < (1 << 11)) {
		str += 0xC0 | (char)(ch >> 6);
		str += 0x80 | (char)(ch & 0x3F);
	}

	// in:  0000 0000  0000 0000  cccc bbbb  bbaa aaaa
	// out:            1110 cccc  10bb bbbb  10aa aaaa
	else if (ch < (1 << 16)) {
		str += 0xE0 | (char) (ch >> 12        );
		str += 0x80 | (char)((ch >>  6) & 0x3F);
		str += 0x80 | (char)( ch        & 0x3F);
	}

	// in:  0000 0000  000d ddcc  cccc bbbb  bbaa aaaa
	// out: 1111 0ddd  10cc cccc  10bb bbbb  10aa aaaa
	else if (ch < (1 << 21)) {
		str += 0xF0 | (char) (ch >> 18        );
		str += 0x80 | (char)((ch >> 12) & 0x3F);
		str += 0x80 | (char)((ch >>  6) & 0x3F);
		str += 0x80 | (char)( ch        & 0x3F);
	}

	return str;
}


#if !defined(UNITSYNC) && !defined(UNIT_TEST) && !defined(BUILDING_AI)
std::vector<std::uint8_t> zlib::deflate(const std::vector<std::uint8_t>& inflData) { return (zlib::deflate(inflData.data(), inflData.size())); }
std::vector<std::uint8_t> zlib::deflate(const std::uint8_t* inflData, unsigned long inflSize) {
	std::vector<std::uint8_t> deflData(compressBound(inflSize));

	unsigned long bufSize = deflData.size();

	if (compress(deflData.data(), &bufSize,  inflData, inflSize) == Z_OK) {
		deflData.resize(bufSize);
		deflData.shrink_to_fit();
	} else {
		deflData.clear();
	}

	return (std::move(deflData));
}

std::vector<std::uint8_t> zlib::inflate(const std::vector<std::uint8_t>& deflData) { return (zlib::inflate(deflData.data(), deflData.size())); }
std::vector<std::uint8_t> zlib::inflate(const std::uint8_t* deflData, unsigned long deflSize) {
	// "the LSB does not describe any mechanism by which a
	// compressor can communicate the size required to the
	// uncompressor" ==> we must reserve some fixed-length
	// buffer (starting at 256K bytes to handle very large
	// blobs) for each new decompression attempt
	std::vector<std::uint8_t> inflData(256 * 1024);

	unsigned long bufSize = inflData.size();
	unsigned long rawSize = bufSize;

	int ret;
	while ((ret = uncompress(inflData.data(), &rawSize,  deflData, deflSize)) == Z_BUF_ERROR) {
		inflData.resize(rawSize = (bufSize *= 2));
	}


	if (ret != Z_OK)
		inflData.clear();

	inflData.resize(rawSize);
	inflData.shrink_to_fit();

	return (std::move(inflData));
}
#endif //UNITSYNC
