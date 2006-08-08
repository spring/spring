/*
 * scrambledfile.h
 * Encrypted file class definition
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
#ifndef HPIUTIL2_SCRAMBLEDFILE_H
#define HPIUTIL2_SCRAMBLEDFILE_H

#include <string>
#include <fstream>
#include <boost/cstdint.hpp>

/*
 * Performance mods
 * Bitwise is generally faster, but disable
 * if you're having problems
 */
#define MODS

#ifdef MODS
#define bitmin(x,y)	((y)+(((x)-(y))&-((x)<(y))))
#define bitmax(x,y)	((x)-(((x)-(y))&-((x)<(y))))
#define bitdiv(n,d)	((n)>>(d))
#define bitmod(n,d)	((n)&((1<<(d))-1))
#else
#define bitmin(x,y)	(x<y?x:y)
#define bitmax(x,y)	(x>y?x:y)
#define bitdiv(n,d)	((n)/(1<<(d)))
#define bitmod(n,d)	((n)%(1<<(d)))
#endif

namespace hpiutil
{
	
	class scrambledfile
	{
	public:
		bool scrambled;
		boost::uint32_t key;
		scrambledfile(const char *fname);
		scrambledfile(std::string const &fname);
		~scrambledfile();
		boost::uint32_t read();
		boost::uint32_t read(boost::uint8_t *buf);
		boost::uint32_t read(boost::uint8_t *buf,const boost::uint32_t off, const boost::uint32_t len);
		boost::uint32_t readint();
		std::string readstring();
		void seek(const boost::uint32_t pos);
		void setkey(const boost::uint32_t k);
		std::fstream file;
	};

}

#endif /* HPIUTIL2_SCRAMBLEDFILE_H */
