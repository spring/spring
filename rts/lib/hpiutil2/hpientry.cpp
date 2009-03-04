/*
 * hpientry.cpp
 * hpi entry class implementation
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
#include "hpientry.h"
#include "hpifile.h"

/**
 * Constructor
 * @param f hpifile that this entry comes from
 * @param pname name of the object's parent (if applicable)
 * @param n name of the object
 * @param o offset of this object in the hpi file
 * @param s size of the object
 */
hpiutil::hpientry::hpientry(hpifile &f, std::string const &pname, std::string const &n, const boost::uint32_t o, const boost::uint32_t s)
  : name(n), parentname(pname), directory(false), offset(o), size(s), file(&f)
{
}

/**
 * destructor
 */
hpiutil::hpientry::~hpientry()
{
}

/**
 * path()
 * @return the full path of the object this entry represents
 */
std::string hpiutil::hpientry::path()
{
	if (parentname == "")
		return name;
	else
		return parentname + PATHSEPARATOR + name;
}
