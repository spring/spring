#include "GuiElement.h"

#include "Rendering/GL/myGL.h"

int GuiElement::screensize[2];

GuiElement::GuiElement(GuiElement* _parent) : parent(_parent)
{
	if (parent)
		parent->AddChild(this);
}

GuiElement::~GuiElement()
{
	for (ChildList::iterator it = children.begin(); it != children.end(); ++it)
	{
		delete *it;
	}
}

void GuiElement::Draw()
{
	DrawSelf();
	for (ChildList::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->Draw();
	}
}

bool GuiElement::HandleEvent(const SDL_Event& ev)
{
	HandleEventSelf(ev);
	for (ChildList::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->HandleEvent(ev);
	}
	return false;
}

bool GuiElement::MouseOver(int x, int y)
{
	float mouse[2] = {PixelToGlX(x), PixelToGlY(y)};
	return (mouse[0] >= pos[0] && mouse[0] <= pos[0]+size[0]) && (mouse[1] >= pos[1] && mouse[1] <= pos[1]+size[1]);
}

void GuiElement::UpdateDisplayGeo(int x, int y)
{
	screensize[0] = x;
	screensize[1] = y;
}

float GuiElement::PixelToGlX(int x)
{
	return float(x)/float(screensize[0]);
}

float GuiElement::PixelToGlY(int y)
{
	return 1.0f - float(y)/float(screensize[1]);
}

void GuiElement::SetPos(float x, float y)
{
	pos[0] = x;
	pos[1] = y;
}

void GuiElement::SetSize(float x, float y)
{
	size[0] = x;
	size[1] = y;
}

void GuiElement::DrawBox(int how)
{
	glBegin(how);
	glVertex2f(pos[0], pos[1]);
	glVertex2f(pos[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]);
	glEnd();
}
