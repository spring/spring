/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef IN_MAP_DRAW_VIEW_H
#define IN_MAP_DRAW_VIEW_H

#include <string>
#include <vector>

#include "System/float3.h"
#include "Game/InMapDrawModel.h"

/**
 * The V in MVC for InMapDraw.
 * @see CInMapDrawModel for M
 * @see CInMapDraw for C
 */
class CInMapDrawView
{
public:
	CInMapDrawView();
	~CInMapDrawView();

	void Draw();

private:
	unsigned int texture;

	std::vector<const CInMapDrawModel::MapPoint*> visibleLabels;
};

extern CInMapDrawView* inMapDrawerView;

#endif /* IN_MAP_DRAW_VIEW_H */
