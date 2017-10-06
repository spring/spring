/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Window.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"

namespace agui
{

Window::Window(const std::string& _title, GuiElement* parent) : GuiElement(parent), title(_title)
{
	titleHeight = 0.05f;
	dragging = false;

	size[0] = size[1] = 0.3f;
	pos[0] = pos[1] = 0.2f;
	dragPos[0] = dragPos[1] = 0.0f;
}

void Window::AddChild(GuiElement* elem)
{
	children.push_back(elem);
	elem->SetPos(pos[0], pos[1]);
	elem->SetSize(size[0], size[1]-titleHeight);
}

void Window::DrawSelf()
{
	gui->SetDrawMode(Gui::DrawMode::COLOR);
	const float opacity = Opacity();
	gui->SetColor(0.0f,0.0f,0.0f, opacity);
	DrawBox(GL_QUADS);

	gui->SetColor(0.7f,0.7f,0.7f, opacity);
	glBegin(GL_QUADS);
	glVertex2f(pos[0], pos[1]+size[1]-titleHeight);
	glVertex2f(pos[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]+size[1]);
	glVertex2f(pos[0]+size[0], pos[1]+size[1]-titleHeight);
	glEnd();

	glLineWidth(2.0f);
	gui->SetColor(1.0f,1.0f,1.0f, opacity);
	DrawBox(GL_LINE_LOOP);
	/*
	glBegin(GL_LINE);
	glVertex2f(pos[0], pos[1]-titleHeight);
	glVertex2f(pos[0]+size[1], pos[1]-titleHeight);
	glEnd();*/

	font->Begin();
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	font->glPrint(pos[0]+0.01, pos[1]+size[1]-titleHeight/2, 1.0, FONT_VCENTER | FONT_SCALE | FONT_SHADOW | FONT_NORM, title);
	gui->SetDrawMode(Gui::DrawMode::MASK);
	font->End();
}

bool Window::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if (MouseOver(ev.button.x, ev.button.y))
			{
				float mouse[2] = {PixelToGlX(ev.button.x), PixelToGlY(ev.button.y)};
				if (mouse[1] > pos[1]+size[1]-titleHeight)
				{
					dragPos[0] = mouse[0] - pos[0];
					dragPos[1] = mouse[1] - pos[1];
					dragging = true;
					return true;
				};
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			if (dragging)
			{
				dragging = false;
				return true;
			}
			break;
		}
		case SDL_MOUSEMOTION: {
			if (dragging)
			{
				Move(PixelToGlX(ev.motion.xrel), PixelToGlY(ev.motion.yrel)-1);
				return true;
			}
			break;
		}
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE)
			{
				WantClose.emit();
				return true;
			}
			break;
		}
	}
	return false;
}

float Window::Opacity() const
{
	if (dragging)
		return GuiElement::Opacity()/2.f;
	else
		return GuiElement::Opacity();
}

}
