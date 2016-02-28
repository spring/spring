/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_TREE_DRAWER_H_
#define _BASIC_TREE_DRAWER_H_

#include "ITreeDrawer.h"
#include "Rendering/GL/myGL.h"


// XXX This has a duplicate in AdvTreeGenerator.h
#define MAX_TREE_HEIGHT 60

class CBasicTreeDrawer : public ITreeDrawer
{
public:
	CBasicTreeDrawer();
	virtual ~CBasicTreeDrawer();

	void Draw(float treeDistance, bool drawReflection);
	void Update() {}

private:
	GLuint treetex;
	int lastListClean;
};

#endif // _BASIC_TREE_DRAWER_H_

