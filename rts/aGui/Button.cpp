#include "Button.h"

#include "Rendering/glFont.h"

Button::Button(const std::string& _label, GuiElement* _parent) : GuiElement(_parent)
{
	Label(_label);
}

void Button::Label(const std::string& _label)
{
	label = _label;
}

void Button::DrawSelf()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glLoadIdentity();
	glColor4f(0.8f,0.8f,0.8f, 0.8f);
	
	glBegin(GL_QUADS);
	glVertex2f(pos[0], pos[1]);
	glVertex2f(pos[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]);
	glEnd();
	
	font->SetTextColor(); //default
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, 1.0f);
	font->glPrint(pos[0]+0.05, pos[1]+size[1]/2, 1.0, FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM, label);
}

bool Button::HandleEventSelf(const SDL_Event& ev)
{
}
