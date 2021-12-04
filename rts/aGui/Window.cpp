/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // memcpy

#include "Window.h"
#include "Gui.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/Fonts/glFont.h"

namespace agui
{

Window::Window(const std::string& _title, GuiElement* parent) : GuiElement(parent), title(_title)
{
	titleHeight = 0.05f;
	dragging = false;

	pos[0] = pos[1] = 0.2f;
	size[0] = size[1] = 0.3f;

	dragPos[0] = dragPos[1] = 0.0f;

	GeometryChangeSelf();
}

void Window::AddChild(GuiElement* elem)
{
	children.push_back(elem);

	elem->SetPos(pos[0], pos[1]);
	elem->SetSize(size[0], size[1] - titleHeight);
}



void Window::DrawSelf()
{
	const float opacity = Opacity();

	// background
	gui->SetDrawMode(Gui::DrawMode::COLOR);
	gui->SetColor(0.0f, 0.0f, 0.0f, opacity);
	DrawBox();
	// foreground
	gui->SetColor(0.7f, 0.7f, 0.7f, opacity);
	DrawBox(1);
	// edges
	gui->SetColor(1.0f, 1.0f, 1.0f, opacity);
	DrawOutline();

	// string content
	gui->SetDrawMode(Gui::DrawMode::FONT);
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	font->SetTextDepth(depth);
	font->glPrint(pos[0] + 0.01f, pos[1] + size[1] - titleHeight * 0.5f, 1.0f, FONT_VCENTER | FONT_SCALE | FONT_SHADOW | FONT_NORM | FONT_BUFFERED, title);
	font->DrawBufferedGL4(gui->GetShader());
}


void Window::GeometryChangeSelf()
{
	GuiElement::GeometryChangeSelf();

	{
		VA_TYPE_T vaElems[4];
		for (int i = 0; i < 4; ++i) {
			const bool bottom = (i & 1) == 0;
			const bool right = (i & 2) == 0;
			vaElems[i].p.x = pos[0] + right * size[0];
			vaElems[i].p.y = pos[1] + size[1] - (!bottom) * titleHeight;
			vaElems[i].p.z = depth;
			vaElems[i].s = 0.0f;
			vaElems[i].t = 0.0f;
		}

		GeometryChangeSelfRaw(1, sizeof(vaElems), vaElems);
	}
}


bool Window::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if (MouseOver(ev.button.x, ev.button.y)) {
				const float mouse[2] = {PixelToGlX(ev.button.x), PixelToGlY(ev.button.y)};

				if (mouse[1] > (pos[1] + size[1] - titleHeight)) {
					dragPos[0] = mouse[0] - pos[0];
					dragPos[1] = mouse[1] - pos[1];
					dragging = true;
					return true;
				};
			}
		} break;
		case SDL_MOUSEBUTTONUP: {
			if (dragging) {
				dragging = false;
				return true;
			}
		} break;
		case SDL_MOUSEMOTION: {
			if (dragging) {
				Move(PixelToGlX(ev.motion.xrel), PixelToGlY(ev.motion.yrel) - 1);
				return true;
			}
		} break;
		case SDL_KEYDOWN: {
			if (ev.key.keysym.sym == SDLK_ESCAPE) {
				WantClose.emit();
				return true;
			}
		} break;
	}

	return false;
}

float Window::Opacity() const
{
	if (dragging)
		return (GuiElement::Opacity() * 0.5f);

	return GuiElement::Opacity();
}

}
