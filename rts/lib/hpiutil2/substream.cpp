/*
 * substream.cpp
 * substream class implementation
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

#include <stdlib.h>
#include "substream.h"

/**
 * Constructor
 * @param sf scrambledfile to read from
 * @param off offset in scrambledfile to start from
 * @param len length of substream data
 */
hpiutil::substream::substream(scrambledfile &sf, const boost::uint32_t off, const boost::uint32_t len)
{
	data = (boost::uint8_t*)calloc(len,sizeof(boost::uint8_t));
	sf.read(data,off,len);
	position = 0;
	length = len;
}

/**
 * Destructor
 */
hpiutil::substream::~substream()
{
	free((void*)data);
}

/**
 * read
 * reads a single byte
 * @return byte read
 */
boost::uint8_t hpiutil::substream::read()
{
	if (position >= length)
		return 0;
	return data[position++];
}

/**
 * read
 * reads data into a buffer
 * @return the number of bytes read
 * @param buf buffer to read into
 */
boost::uint32_t hpiutil::substream::read(boost::uint8_t *buf)
{
	if (position >= length)
		return 0;
	boost::uint32_t oldpos = position;
	boost::uint32_t len = bitmin(sizeof(buf),(length-position));
	for (boost::uint32_t j = 0; j < len; j++)
		buf[j] = data[position++];
	return position - oldpos;
}

/**
 * read
 * reads data into a buffer
 * @return the number of bytes read
 * @param buf buffer to read into
 * @param off offset in substream to start reading from
 * @param len size of data to read
 */
boost::uint32_t hpiutil::substream::read(boost::uint8_t *buf, const boost::uint32_t off, const boost::uint32_t len)
{
	position = bitmin(off,length);
	if (position >= length)
		return 0;
	boost::uint32_t reallen = bitmin(len,(length-position));
	for (boost::uint32_t j = 0; j < reallen; j++)
		buf[j] = data[position++];
	return position-off;
}

/**
 * checksum
 * performs a checksum on the substream data by
 * adding up all bytes.
 * @return checksum
 * @param start byte to start with
 */
boost::uint32_t hpiutil::substream::checksum(const boost::uint32_t start)
{
	boost::uint32_t check = 0;
	for (boost::uint32_t i = start; i < length; i++)
		check += data[i];
	return check;
}
