/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTICALLAYOUT_H
#define VERTICALLAYOUT_H

#include "GuiElement.h"
#include "Layout.h"

namespace agui
{

class VerticalLayout : public GuiElement, public Layout
{
public:
	VerticalLayout(GuiElement* parent = NULL);

private:
	virtual void DrawSelf();
	virtual void GeometryChangeSelf();
};

}

#endif
