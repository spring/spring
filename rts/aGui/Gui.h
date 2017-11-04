/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GUI_H
#define GUI_H

#include <list>
#include <slimsig/connection.h>
#include "System/Input/InputHandler.h"

union SDL_Event;

namespace Shader {
	struct IProgramObject;
}

namespace agui
{

class GuiElement;

class Gui
{
public:
	Gui();
	~Gui();

	void Clean();
	void Draw();
	void AddElement(GuiElement*, bool asBackground = false);
	/// deletes the element on the next draw
	void RmElement(GuiElement*);

	void UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY);

	bool MouseOverElement(const GuiElement*, int x, int y) const;
	void SetColor(float r, float g, float b, float a);
	void SetColor(const float* v) { SetColor(v[0], v[1], v[2], v[3]); }

	enum DrawMode {
		COLOR   = 0,
		TEXTURE = 1,
		FONT    = 2,
	};

	void SetDrawMode(DrawMode newMode);

	Shader::IProgramObject* GetShader() { return shader; }

private:
	bool HandleEvent(const SDL_Event& ev);

	struct GuiItem {
		GuiItem(GuiElement* el, bool back) : element(el), asBackground(back) {}
		GuiElement* element;
		bool asBackground;
	};

	std::list<GuiItem> elements;
	std::list<GuiItem> toBeRemoved;
	std::list<GuiItem> toBeAdded;

	DrawMode currentDrawMode = DrawMode::COLOR;

	Shader::IProgramObject* shader;

	InputHandler::SignalType::connection_type inputCon;
};

extern Gui* gui;

}

#endif
