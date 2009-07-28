#ifndef VERTICALLAYOUT_H
#define VERTICALLAYOUT_H

#include "GuiElement.h"

namespace agui
{

class VerticalLayout : public GuiElement
{
public:
	VerticalLayout(GuiElement* parent = NULL);

	virtual void AddChild(GuiElement* elem);

private:
	float spacing;
	float borderWidth;
	virtual void DrawSelf();
};

}

#endif