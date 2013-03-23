/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include <string>
#include "System/creg/creg_cond.h"

class CResource
{
public:
	CR_DECLARE_STRUCT(CResource);

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
