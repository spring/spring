#include "HorizontalLayout.h"

#include "Rendering/GL/myGL.h"

namespace agui
{

HorizontalLayout::HorizontalLayout(GuiElement* parent) : GuiElement(parent)
{
	spacing = 0.005f;
	borderWidth = 1.0f;
}

void HorizontalLayout::SetPos(float x, float y)
{
	GuiElement::SetPos(x, y);
	UpdateChildGeo();
}

void HorizontalLayout::SetSize(float x, float y)
{
	GuiElement::SetSize(x, y);
	UpdateChildGeo();
}

void HorizontalLayout::AddChild(GuiElement* elem)
{
	children.push_back(elem);
	UpdateChildGeo();
}

void HorizontalLayout::DrawSelf()
{
	if (borderWidth > 0)
	{
		glColor4f(1.f,1.f,1.f, 1.f);	
		DrawBox(GL_LINE_LOOP);
	}
}

void HorizontalLayout::UpdateChildGeo()
{
	const float hspacePerObject = (size[0]-(float(children.size()-1)*spacing))/float(children.size());
	size_t num = 0;
	for (ChildList::iterator i = children.begin(); i != children.end(); ++i)
	{
		(*i)->SetPos(pos[0] + num*hspacePerObject + (num+1)*spacing, pos[1] + spacing);
		(*i)->SetSize(hspacePerObject - 2.0f * spacing, size[1] - 2.0f*spacing);
		++num;
	}
}

}
