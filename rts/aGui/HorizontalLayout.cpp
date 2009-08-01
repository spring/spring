#include "HorizontalLayout.h"

#include "Rendering/GL/myGL.h"

namespace agui
{

HorizontalLayout::HorizontalLayout(GuiElement* parent) : GuiElement(parent)
{
}

void HorizontalLayout::DrawSelf()
{
	if (borderWidth > 0)
	{
		glLineWidth(borderWidth);
		glColor4f(1.f,1.f,1.f, 1.f);
		DrawBox(GL_LINE_LOOP);
	}
}

void HorizontalLayout::GeometryChangeSelf()
{
	if (children.empty())
		return;
	unsigned numFixed = 0;
	float totalFixedSize = 0.0;
	for (ChildList::const_iterator it = children.begin(); it != children.end(); ++it)
	{
		if ((*it)->SizeFixed())
		{
			numFixed++;
			totalFixedSize += (*it)->GetSize()[0];
		}
	}

	const float hspacePerObject = (size[0]-float(children.size()-1)*itemSpacing - 2*borderSpacing-totalFixedSize)/float(children.size()-numFixed);
	float startX = pos[0] + borderSpacing;
	for (ChildList::iterator i = children.begin(); i != children.end(); ++i)
	{
		(*i)->SetPos(startX, pos[1] + borderSpacing);
		if ((*i)->SizeFixed())
		{
			(*i)->SetSize((*i)->GetSize()[0], size[1] - 2.0f*borderSpacing, true);
			startX += (*i)->GetSize()[0] + itemSpacing;
		}
		else
		{
			(*i)->SetSize(hspacePerObject, size[1]- 2.0f*borderSpacing);
			startX += hspacePerObject + itemSpacing;
		}
	}
}

}
