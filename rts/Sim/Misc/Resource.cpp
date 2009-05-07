/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Resource.h"

#include <float.h>

CR_BIND(CResource, );

CR_REG_METADATA(CResource, (
				CR_MEMBER(name),
				CR_MEMBER(optimum),
				CR_MEMBER(extractorRadius),
				CR_MEMBER(maxWorth)
				));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResource::CResource():
		name("UNNAMED_RESOURCE"),
		optimum(FLT_MAX),
		extractorRadius(0.0f),
		maxWorth(0.0f)
{
}

CResource::~CResource()
{
}
