#ifndef VERTICALLAYOUT_H
#define VERTICALLAYOUT_H

#include "GuiElement.h"

namespace agui
{

class VerticalLayout : public GuiElement
{
public:
	VerticalLayout(GuiElement* parent = NULL);
	void SetBorder(float thickness);

private:
	virtual void DrawSelf();
	virtual void GeometryChangeSelf();

	float spacing;
	float borderWidth;
};

}

#endif