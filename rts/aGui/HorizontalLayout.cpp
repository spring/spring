#include "HorizontalLayout.h"

#include "Rendering/GL/myGL.h"

namespace agui
{

HorizontalLayout::HorizontalLayout(GuiElement* parent) : GuiElement(parent)
{
	spacing = 0.005f;
	borderWidth = 0.0f;
}

void HorizontalLayout::SetBorder(float thickness)
{
	borderWidth = thickness;
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

	const float hspacePerObject = (size[0]-(float(children.size()+1)*spacing)-totalFixedSize)/float(children.size()-numFixed);
	float startX = pos[0] + spacing;
	for (ChildList::iterator i = children.begin(); i != children.end(); ++i)
	{
		(*i)->SetPos(startX, pos[1] + spacing);
		if ((*i)->SizeFixed())
		{
			(*i)->SetSize((*i)->GetSize()[0], size[1] - 2.0f*spacing, true);
			startX += (*i)->GetSize()[0] + spacing;
		}
		else
		{
			(*i)->SetSize(hspacePerObject, size[1]- 2.0f*spacing);
			startX += hspacePerObject + spacing;
		}
	}
}

}
