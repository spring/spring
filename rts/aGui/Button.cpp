/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Button.h"

#include "Gui.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"

namespace agui
{

Button::Button(const std::string& _label, GuiElement* _parent)
	: GuiElement(_parent)
{
	Label(_label);
}

void Button::Label(const std::string& _label)
{
	label = _label;
	clicked = false;
	hovered = false;
}

void Button::DrawSelf()
{
	gui->SetDrawMode(Gui::DrawMode::COLOR);
	const float opacity = Opacity();
	gui->SetColor(0.8f, 0.8f, 0.8f, opacity);

	DrawBox();

	gui->SetColor(1.0f, 1.0f, 1.0f, 0.1f);
	if (clicked) {
		glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE); // additive blending
		gui->SetColor(0.2f, 0.0f, 0.0f, opacity);
		DrawBox();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gui->SetColor(1.0f, 0.0f, 0.0f, opacity / 2.0f);
		DrawOutline();
	} else if (hovered) {
		glAttribStatePtr->BlendFunc(GL_ONE, GL_ONE); // additive blending
		gui->SetColor(0.0f, 0.0f, 0.2f, opacity);
		DrawBox();
		glAttribStatePtr->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gui->SetColor(1.0f, 1.0f, 1.0f, opacity / 2.0f);
		DrawOutline();
	}

	gui->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	gui->SetDrawMode(Gui::DrawMode::FONT);
	font->SetTextColor(1.0f, 1.0f, 1.0f, opacity);
	font->SetOutlineColor(0.0f, 0.0f, 0.0f, opacity);
	font->SetTextDepth(depth);
	font->glPrint(pos[0] + size[0] * 0.5f, pos[1] + size[1] * 0.5f, 1.0, FONT_CENTER | FONT_VCENTER | FONT_SHADOW | FONT_SCALE | FONT_NORM | FONT_BUFFERED, label);
	font->DrawBufferedGL4(gui->GetShader());
}

bool Button::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if ((ev.button.button == SDL_BUTTON_LEFT)
					&& MouseOver(ev.button.x, ev.button.y)
					&& gui->MouseOverElement(GetRoot(), ev.button.x, ev.button.y))
			{
				clicked = true;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP: {
			if ((ev.button.button == SDL_BUTTON_LEFT)
					&& MouseOver(ev.button.x, ev.button.y)
					&& clicked)
			{
				if (!Clicked.empty()) {
					Clicked.emit();
				} else {
					LOG_L(L_WARNING, "Button %s clicked without callback", label.c_str());
				}
				clicked = false;
				return true;
			}
			break;
		}
		case SDL_MOUSEMOTION: {
			if (MouseOver(ev.motion.x, ev.motion.y)
					&& gui->MouseOverElement(GetRoot(), ev.motion.x, ev.motion.y))
			{
				hovered = true;
			} else {
				hovered = false;
				clicked = false;
			}
			break;
		}
	}
	return false;
}

} // namespace agui
