#ifndef HORIZONTALLAYOUT_H
#define HORIZONTALLAYOUT_H

#include "GuiElement.h"

namespace agui
{

class HorizontalLayout : public GuiElement
{
public:
	HorizontalLayout(GuiElement* parent = NULL);

	virtual void AddChild(GuiElement* elem);

	virtual void SetPos(float x, float y);
	virtual void SetSize(float x, float y);
	void SetBorder(float thickness);

private:
	float spacing;
	float borderWidth;
	virtual void DrawSelf();
	void UpdateChildGeo();
};

}
#endif