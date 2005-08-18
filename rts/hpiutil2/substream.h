/*
 * substream.h
 * substream class definition
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

#ifndef HPIUTIL2_SUBSTREAM_H
#define HPIUTIL2_SUBSTREAM_H

#include "scrambledfile.h"

namespace hpiutil
{
	
	class substream
	{
	public:
		substream(scrambledfile &sf, const uint32_t off, const uint32_t len);
		~substream();
		uint8_t read();
		uint32_t read(uint8_t *buf);
		uint32_t read(uint8_t *buf, const uint32_t off, const uint32_t len);
		uint32_t readint();
		uint32_t checksum(const uint32_t start);
		uint8_t *data;
		uint32_t position;
		uint32_t length;
	};

}

#endif /* HPIUTIL2_SUBSTREAM_H */
