#ifndef HORIZONTALLAYOUT_H
#define HORIZONTALLAYOUT_H

#include "GuiElement.h"

namespace agui
{

class HorizontalLayout : public GuiElement
{
public:
	HorizontalLayout(GuiElement* parent = NULL);
	void SetBorder(float thickness);

private:
	virtual void GeometryChangeSelf();

	float spacing;
	float borderWidth;
	virtual void DrawSelf();
};

}

#endif