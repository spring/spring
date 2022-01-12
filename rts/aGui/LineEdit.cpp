/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LineEdit.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "System/Misc/SpringTime.h"


namespace agui
{

LineEdit::LineEdit(GuiElement* parent)
	: GuiElement(parent)
	, hasFocus(false)
	, crypt(false)
	, cursorPos(0)
{
}

void LineEdit::SetFocus(bool focus)
{
	hasFocus = focus;
}

void LineEdit::SetCrypt(bool focus)
{
	crypt = focus;
}

void LineEdit::SetContent(const std::string& line, bool moveCursor)
{
	content = line;
	if (!moveCursor)
		return;
	cursorPos = content.size();
}

void LineEdit::DrawSelf()
{
	gui->SetDrawMode(Gui::DrawMode::COLOR);
	const float opacity = Opacity();
	gui->SetColor(1.0f, 1.0f, 1.0f, opacity);
	DrawBox();

	if (hasFocus) {
		gui->SetColor(0.0f, 0.0f, 0.0f, opacity);
		DrawOutline();
	} else {
		gui->SetColor(0.5f, 0.5f, 0.5f, opacity);
		DrawOutline();
	}

	std::string tempText;
	if (crypt) {
		tempText.resize(content.size(), '*');
	} else {
		tempText = content;
	}

	const float textCenter = pos[1] + size[1]/2;
	if (hasFocus) {
		// draw the caret
		const std::string caretStr = tempText.substr(0, cursorPos);
		float caretWidth = font->GetSize() * font->GetTextWidth(caretStr) / float(screensize[0]);

		char c = tempText[cursorPos];
		if (c == 0) { c = ' '; }

		float cursorHeight = font->GetSize() * font->GetLineHeight() / float(screensize[1]);
		float cw = font->GetSize() * font->GetCharacterWidth(c) /float(screensize[0]);
		float csx = pos[0] + 0.01 + caretWidth;
		float f = 0.5f * (1.0f + fastmath::sin(spring_now().toMilliSecsf() * 0.015f));
		gui->SetColor(f, f, f, opacity);
		glRectf(csx, textCenter + cursorHeight/2, csx + cw, textCenter - cursorHeight/2);
		gui->SetColor(0.0f, 0.0f, 0.0f, 1.0f); // black
	}

	gui->SetDrawMode(Gui::DrawMode::FONT);
	font->SetTextColor(); //default
	font->SetTextDepth(depth);
	font->glPrint(pos[0] + 0.01, textCenter, 1.0, FONT_VCENTER | FONT_SCALE | FONT_NORM | FONT_BUFFERED, tempText);
	font->DrawBufferedGL4(gui->GetShader());
}

bool LineEdit::HandleEventSelf(const SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEBUTTONDOWN: {
			if (MouseOver(ev.button.x, ev.button.y)) {
				hasFocus = true;
			} else {
				hasFocus = false;
			}
			break;
		}
		case SDL_TEXTINPUT: {
			content.insert(cursorPos, ev.text.text);
			cursorPos+=strlen(ev.text.text);
		} break;
		case SDL_KEYDOWN: {
			if (!hasFocus) {
				break;
			}
			switch(ev.key.keysym.sym)
			{
				case SDLK_BACKSPACE: {
					if (cursorPos > 0) {
						content.erase(--cursorPos, 1);
					}
					break;
				}
				case SDLK_DELETE: {
					if (cursorPos < content.size()) {
						content.erase(cursorPos, 1);
					}
					break;
				}
				case SDLK_ESCAPE: {
					hasFocus = false;
					break;
				}
				case SDLK_RIGHT: {
					if (cursorPos < content.size()) {
						++cursorPos;
					}
					break;
				}
				case SDLK_LEFT: {
					if (cursorPos > 0) {
						--cursorPos;
					}
					break;
				}
				case SDLK_HOME: {
					cursorPos = 0;
					break;
				}
				case SDLK_END: {
					cursorPos = content.size();
					break;
				}
				case SDLK_RETURN: {
					DefaultAction.emit();
					return true;
				}
			}
			break;
		}
	}
	return false;
}

} // namespace agui
