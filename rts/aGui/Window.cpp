#include "Window.h"


#include "Rendering/GL/myGL.h"
#include "Rendering/glFont.h"

Window::Window(const std::string& _title, GuiElement* parent) : GuiElement(parent), title(_title)
{
	titleHeight = 0.01f;
	dragging = false;
}


void Window::DrawSelf()
{
	glColor4f(0.0f,0.0f,0.0f, 1.0f);
	DrawBox(GL_QUADS);
	
	glLineWidth(1.0f);
	glColor4f(1.0f,1.0f,1.0f, 1.0f);
	DrawBox(GL_LINE_LOOP);
	
	glBegin(GL_LINE);
	glVertex2f(pos[0], pos[1]-titleHeight);
	glVertex2f(pos[0]+size[1], pos[1]-titleHeight);
	glEnd();
	
	glColor4f(0.0f,0.0f,0.0f, 1.0f);
	font->glPrint(pos[0]+0.01, pos[1]-titleHeight/2, 1.0, FONT_VCENTER | FONT_SCALE | FONT_NORM, title);
}

bool Window::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if (MouseOver(ev.button.x, ev.button.y))
			{
				float mouseY = PixelToGlY(ev.button.y);
				if (mouseY < pos[1]-titleHeight)
				{
					dragging = true;
				};
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			dragging = false;
		}
		case SDL_MOUSEMOTION: {
			if (dragging)
			{
				 SetPos(PixelToGlX(ev.button.x), PixelToGlY(ev.button.y));
			}
			break;
		}
	}
	return false;
}
