/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NULL_TREE_DRAWER_H_
#define _NULL_TREE_DRAWER_H_

#include "ITreeDrawer.h"

class CNullTreeDrawer : public ITreeDrawer
{
public:
	CNullTreeDrawer(): ITreeDrawer() {}

	void DrawPass() override {}
	void DrawShadowPass() override {}
	void Update() override {}
};

#endif

