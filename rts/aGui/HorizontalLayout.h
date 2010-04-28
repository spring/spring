/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef HORIZONTALLAYOUT_H
#define HORIZONTALLAYOUT_H

#include "GuiElement.h"
#include "Layout.h"

namespace agui
{

class HorizontalLayout : public GuiElement, public Layout
{
public:
	HorizontalLayout(GuiElement* parent = NULL);

private:
	virtual void DrawSelf();
	virtual void GeometryChangeSelf();
};

}

#endif
