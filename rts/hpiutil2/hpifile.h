/*
 * hpifile.h
 * hpi file class definition
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

#ifndef HPIUTIL2_HPIFILE_H
#define HPIUTIL2_HPIFILE_H

#include <vector>
#include "sqshstream.h"
#include "hpientry.h"

#define HAPI_MAGIC 0x49504148
#define BANK_MAGIC 0x4b4e4142
#define MARKER_MAGIC 0x00010000

class hpifile
{
public:
	bool valid;
	hpifile(const char *fname);
	hpifile(std::string const &fname);
	~hpifile();
	std::vector<hpientry*> flatlist;
	hpientry& dirinfo(std::string const &parentname, std::string const &dirname, const uint32_t offset);
	hpientry& fileinfo(std::string const &parentname, std::string const &name, const uint32_t offset);
	uint32_t getdata(hpientry const &he, uint8_t *data);
private:
	scrambledfile *file;
	void validate(const char *n);
	uint32_t header_hapimagic;
	uint32_t header_bankmagic;
	uint32_t header_offset;
	uint32_t header_key;
	uint32_t header_diroffset;
};

#endif /* HPIUTIL2_HPIFILE_H */
