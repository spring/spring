/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VerticalLayout.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"

namespace agui
{

VerticalLayout::VerticalLayout(GuiElement* parent) : GuiElement(parent)
{
}

void VerticalLayout::DrawSelf()
{
	if (!visibleBorder)
		return;

	gui->SetDrawMode(Gui::DrawMode::COLOR);
	gui->SetColor(1.f,1.f,1.f, Opacity());
	DrawOutline();
}

void VerticalLayout::GeometryChangeSelf()
{
	GuiElement::GeometryChangeSelf();

	if (children.empty())
		return;
	unsigned numFixed = 0;
	float totalFixedSize = 0.0;
	for (ChildList::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		if ((*it)->SizeFixed())
		{
			numFixed++;
			totalFixedSize += (*it)->GetSize()[1];
		}
	}

	const float vspacePerObject = (size[1]-float(children.size()-1)*itemSpacing - 2*borderSpacing-totalFixedSize)/float(children.size()-numFixed);
	float startY = pos[1] + borderSpacing;
	for (ChildList::reverse_iterator i = children.rbegin(); i != children.rend(); ++i)
	{
		(*i)->SetPos(pos[0]+borderSpacing, startY);
		if ((*i)->SizeFixed())
		{
			(*i)->SetSize(size[0]- 2.0f*borderSpacing, (*i)->GetSize()[1], true);
			startY += (*i)->GetSize()[1] + itemSpacing;
		}
		else
		{
			(*i)->SetSize(size[0]- 2.0f*borderSpacing, vspacePerObject);
			startY += vspacePerObject + itemSpacing;
		}
	}
}

}
