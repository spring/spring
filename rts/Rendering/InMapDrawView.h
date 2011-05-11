/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_VIEW_H
#define IN_MAP_DRAW_VIEW_H

#include <string>
#include <vector>
#include <list>

#include "System/float3.h"
#include "System/creg/creg.h"
#include "Game/InMapDraw.h"

/**
 * The V in MVC for InMapDraw.
 * @see CInMapDraw for M & C
 */
class CInMapDrawView
{
	CR_DECLARE(CInMapDrawView);

public:
	CInMapDrawView();
	~CInMapDrawView();

	void Draw();

private:
	unsigned int texture;

	std::vector<const CInMapDraw::MapPoint*> visibleLabels;
};

extern CInMapDrawView* inMapDrawerView;

#endif /* IN_MAP_DRAW_VIEW_H */
