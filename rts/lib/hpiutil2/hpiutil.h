/*
 * hpiutil.h
 * HPIUtil main header
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

#ifndef HPIUTIL2_H
#define HPIUTIL2_H

#include "hpifile.h"

namespace hpiutil
{
	hpifile* HPIOpen(const char *filename);
	void HPIClose(hpifile &hpi);
	hpientry_ptr HPIOpenFile(hpifile const &hpi, const char *filename);
	void HPICloseFile(hpientry_ptr &he);
	boost::uint32_t HPIGet(char *dest, hpientry_ptr const &he, const int offset, const int bytecount);
	std::vector<hpientry_ptr> HPIGetFiles(hpifile const &hpi);
	std::vector<hpientry_ptr> HPIDir(hpifile const &hpi, const char *dirname);
	hpientry_ptr HPIReadFlatList(hpifile const &hpi, const char *name, const bool dir);

}

#endif /* HPIUTIL2_H */
