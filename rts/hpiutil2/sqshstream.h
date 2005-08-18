/*
 * sqshstream.h
 * SQSH stream class definition
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

#ifndef HPIUTIL2_SQSHSTREAM_H
#define HPIUTIL2_SQSHSTREAM_H

#include <sstream>
#include "substream.h"

#define HPI_NONE 0
#define HPI_LZ77 1
#define HPI_ZLIB 2

#define SQSH_MAGIC 0x48535153
#define SQSH_HEADER 19

namespace hpiutil
{
	
	class sqshstream
	{
	public:
		bool valid;
		uint8_t encrypt;
		uint8_t compress;
		uint32_t position;
		sqshstream(substream &ss);
		~sqshstream();
		uint8_t read();
		uint32_t read(uint8_t *buf);
		uint32_t read(uint8_t *buf, const uint32_t off, const uint32_t len);
		uint32_t readall(uint8_t *buf);
		uint32_t readint();
	private:
		bool decompress();
		uint32_t decompresszlib(uint8_t *src, uint8_t *dest, const uint32_t srcsize, const uint32_t destsize);
		uint32_t decompresslz77(uint8_t *src, uint8_t *dest, const uint32_t srcsize, const uint32_t destsize);
		substream *stream;
		std::istringstream *fullstream;
		uint8_t *data;
		uint32_t compressedsize;
		uint32_t fullsize;
		uint32_t checksum;
	};

}

#endif /* HPIUTIL2_SQSHSTREAM_H */
