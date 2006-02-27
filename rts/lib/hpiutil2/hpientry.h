/*
 * hpientry.h
 * hpi entry class definition
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

#ifndef HPIUTIL2_HPIENTRY_H
#define HPIUTIL2_HPIENTRY_H

// The spring virtual filesystem uses '/' internally on all platforms, so removed win32 conditional
#define PATHSEPARATOR '/'
#define OTHERPATHSEPARATOR '\\'

#include <vector>
#include <string>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

namespace hpiutil
{
	
	class hpifile;
	class hpientry;
  typedef boost::shared_ptr<hpientry> hpientry_ptr;
	class hpientry
	{
	public:
		std::string name;
		std::string parentname;
		bool directory;
		boost::uint32_t offset;
		boost::uint32_t size;
		std::vector<hpientry_ptr> subdir;
		hpientry(hpifile &f, std::string const &pname, std::string const &n, const boost::uint32_t offset, const boost::uint32_t size);
		~hpientry();
		std::string path();
		hpifile *file;
	};

}

#endif /* HPIUTIL2_HPIENTRY_H */
