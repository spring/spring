#include "VerticalLayout.h"

#include "Rendering/GL/myGL.h"

namespace agui
{

VerticalLayout::VerticalLayout(GuiElement* parent) : GuiElement(parent)
{
	spacing = 0.005f;
	borderWidth = 0.0f;
}

void VerticalLayout::SetBorder(float thickness)
{
	borderWidth = thickness;
}

void VerticalLayout::DrawSelf()
{
	if (borderWidth > 0)
	{
		glLineWidth(borderWidth);
		glColor4f(1.f,1.f,1.f, 1.f);
		DrawBox(GL_LINE_LOOP);
	}
}

void VerticalLayout::GeometryChangeSelf()
{
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

	const float vspacePerObject = (size[1]-(float(children.size()+1)*spacing)-totalFixedSize)/float(children.size()-numFixed);
	float startY = pos[1] + spacing;
	for (ChildList::reverse_iterator i = children.rbegin(); i != children.rend(); ++i)
	{
		(*i)->SetPos(pos[0]+spacing, startY);
		if ((*i)->SizeFixed())
		{
			(*i)->SetSize(size[0]- 2.0f*spacing, (*i)->GetSize()[1], true);
			startY += (*i)->GetSize()[1] + spacing;
		}
		else
		{
			(*i)->SetSize(size[0]- 2.0f*spacing, vspacePerObject);
			startY += vspacePerObject + spacing;
		}
	}
}

}
