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

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>
#include "creg/creg_cond.h"

class CResource
{
public:
	CR_DECLARE(CResource);

	CResource();
	~CResource();

	/// The name of this resource, eg. "Energy" or "Metal"
	std::string name;
	/// The optimum value for this resource, eg. 0 for "Waste" or MAX_FLOAT for "Metal"
	float optimum;
	/// The default extractor radius for the resource map, 0.0f if non applicable
	float extractorRadius;
	/// What value 255 in the resource map is worth
	float maxWorth;
};

#endif // _RESOURCE_H
