/*
 * sqshstream.cpp
 * SQSH stream class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 *
 * This file is part of hpiutil2.
 *
 * hpiutil2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * hpiutil2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hpiutil2; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include <stdlib.h>
#include <zlib.h>
#include "sqshstream.h"

/**
 * Constructor
 * @param ss substream to read from
 */
hpiutil::sqshstream::sqshstream(substream &ss)
{
	valid = false;
	stream = &ss;
	boost::uint32_t magic = readint();
	if (magic != SQSH_MAGIC) {
		std::cerr << "Invalid SQSH header signature: 0x" << std::hex << magic << std::endl;
		return;
	}
	stream->read(); // unknown data
	compress = stream->read();
	encrypt = stream->read();
	compressedsize = readint();
	fullsize = readint();
	checksum = readint();
	boost::uint32_t newcheck = stream->checksum(SQSH_HEADER);
	if (checksum && (newcheck != checksum)) {
		std::cerr << "Chunk checksum " << std::hex << newcheck << " does not match stored checksum " << std::hex << checksum << std::endl;
		return;
	}
	if (decompress())
		valid = true;
	else
		free((void*)data);
}

/**
 * Destructor
 */
hpiutil::sqshstream::~sqshstream()
{
	if (valid)
		free((void*)data);
}

/**
 * decompress
 * decompresses the substream data if necessary
 * @return whether the decompression was successful
 */
bool hpiutil::sqshstream::decompress()
{
	boost::uint8_t *compstring = (boost::uint8_t*)calloc(compressedsize,sizeof(boost::uint8_t));
	stream->read(compstring,SQSH_HEADER,compressedsize);
	if (encrypt) {
		for (boost::uint32_t i = 0; i < compressedsize; i++)
			compstring[i] = compstring[i] - i ^ i;
	}
	position = 0;
	boost::uint32_t ret;
	if (compress == HPI_LZ77) {
		data = (boost::uint8_t*)calloc(fullsize,sizeof(boost::uint8_t));
		ret = decompresslz77(compstring,data,compressedsize,fullsize);
		free((void*)compstring);
	} else if (compress == HPI_ZLIB) {
		data = (boost::uint8_t*)calloc(fullsize,sizeof(boost::uint8_t));
		ret = decompresszlib(compstring,data,compressedsize,fullsize);
		free((void*)compstring);
	} else {
		data = compstring;
		ret = fullsize;
	}
	return ret==fullsize;
}

/**
 * read
 * reads a single byte
 * @return byte read
 */
boost::uint8_t hpiutil::sqshstream::read()
{
	if (valid) {
		if (position >= fullsize)
			return 0;
		return data[position++];
	} else
		return (boost::uint8_t)stream->read();
}

/**
 * read
 * reads data into a buffer
 * @return number of bytes read
 * @param buf buffer to read into
 */
boost::uint32_t hpiutil::sqshstream::read(boost::uint8_t *buf)
{
	if ((position >= fullsize)||!valid)
		return 0;
	boost::uint32_t oldpos = position;
	boost::uint32_t len = bitmin(sizeof(buf),(fullsize-position));
	for (boost::uint32_t i = 0; i < len; i++)
		buf[i] = data[position++];
	return position - oldpos;
}

/**
 * read
 * reads data into a buffer
 * @return number of bytes read
 * @param buf buffer to read into
 * @param off offset to start reading from
 * @param len number of bytes to read
 */
boost::uint32_t hpiutil::sqshstream::read(boost::uint8_t *buf, const boost::uint32_t off, const boost::uint32_t len)
{
	position = bitmin(off,fullsize);
	if ((position >= fullsize)||!valid)
		return 0;
	boost::uint32_t reallen = bitmin(len,(fullsize-position));
	for (boost::uint32_t i = 0; i < reallen; i++)
		buf[i] = data[position++];
	return position - off;
}

/**
 * readall
 * reads all data into a buffer
 * caller's responsibility to make sure enough
 * space is allocated
 * @return number of bytes read
 * @param buf buffer to read into
 */
boost::uint32_t hpiutil::sqshstream::readall(boost::uint8_t *buf)
{
	if (!valid)
		return 0;
	boost::uint32_t i;
	for (i = 0; i < fullsize; i++)
		buf[i] = data[i];
	return i;
}

/**
 * readint
 * reads a 32-bit integer,
 * byte swabbing if necessary
 * @return swabbed integer
 */
boost::uint32_t hpiutil::sqshstream::readint()
{
	boost::uint32_t a = read();
	boost::uint32_t b = read();
	boost::uint32_t c = read();
	boost::uint32_t d = read();
	return (d<<24)|(c<<16)|(b<<8)|a;
}

/**
 * decompresszlib
 * decompresses zlib-compressed data from one buffer to another
 * @return the number of bytes decompressed
 * @param src buffer with source compressed data
 * @param dest buffer to put destination uncompressed data
 * @param srcsize size of source data
 * @param destsize expected size of destination data
 */
boost::uint32_t hpiutil::sqshstream::decompresszlib(boost::uint8_t *src, boost::uint8_t *dest, const boost::uint32_t srcsize, const boost::uint32_t destsize)
{
	z_stream zs;
	zs.next_in = (Bytef*)src;
	zs.avail_in = srcsize;
	zs.total_in = 0;
	zs.next_out = (Bytef*)dest;
	zs.avail_out = destsize;
	zs.total_out = 0;
	zs.msg = NULL;
	zs.state = NULL;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = NULL;
	zs.data_type = Z_BINARY;
	zs.adler = 0;
	zs.reserved = 0;

	if (inflateInit(&zs) != Z_OK) {
		std::cerr << "Inflate initialization failed" << std::endl;
		return 0;
	}
	if (inflate(&zs, Z_FINISH) != Z_STREAM_END) {
		std::cerr << "Could not inflate to end of stream" << std::endl;
		return 0;
	}
	if (inflateEnd(&zs) != Z_OK) {
		std::cerr << "Could not complete inflation" << std::endl;
		return 0;
	}
	return zs.total_out;
}

/**
 * decompresslz77
 * decompresses lz77-compressed data from one buffer to another
 * @return the number of bytes decompressed
 * @param src buffer with source compressed data
 * @param dest buffer to put destination uncompressed data
 * @param srcsize size of source data
 * @param destsize expected size of destination data
 */
boost::uint32_t hpiutil::sqshstream::decompresslz77(boost::uint8_t *src, boost::uint8_t *dest, const boost::uint32_t srcsize, const boost::uint32_t destsize)
{
	int w1 = 1, w2 = 1;
	int in = 0, out = 0;
	int count;
	char dbuf[4096];
	int dptr;
	int w3 = src[in++];
	while (1) {
		if (!(w2&w3)) {
			dest[out++] = src[in];
			dbuf[w1] = src[in];
			w1 = (w1 + 1) & 0xfff;
			in++;
		} else {
			count = *((boost::uint16_t*)(src+in));
			in += 2;
			dptr = count >> 4;
			if (dptr == 0)
				return out;
			else {
				count = (count & 0x0f) + 2;
				if (count >= 0) {
					for (int x = 0; x < count; x++) {
						dest[out++] = dbuf[dptr];
						dbuf[w1] = dbuf[dptr];
						dptr = (dptr+1) & 0xfff;
						w1 = (w1 + 1) & 0xfff;
					}
				}
			}
		}
		w2 *= 2;
		if (w2 & 0x0100) {
			w2 = 1;
			w3 = src[in++];
		}
	}
	return out;
}
