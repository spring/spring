/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GuiElement.h"
#include "System/SafeUtil.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Shaders/Shader.h"

namespace agui
{

int GuiElement::screensize[2];
int GuiElement::screenoffset[2];

GuiElement::GuiElement(GuiElement* _parent) : parent(_parent), fixedSize(false), weight(1)
{
	size[0] = size[1] = 0.0f;
	pos[0] = pos[1] = 0.0f;

	if (parent)
		parent->AddChild(this);
}

GuiElement::~GuiElement()
{
	for (auto& ch: children)
		spring::SafeDelete(ch);
}

void GuiElement::Draw()
{
	DrawSelf();
	for (auto ch: children)
	{
		ch->Draw();
	}
}

bool GuiElement::HandleEvent(const SDL_Event& ev)
{
	if (HandleEventSelf(ev))
		return true;
	for (auto ch : children)
	{
		if (ch->HandleEvent(ev))
			return true;
	}
	return false;
}

bool GuiElement::MouseOver(int x, int y) const
{
	float mouse[2] = {PixelToGlX(x), PixelToGlY(y)};
	return (mouse[0] >= pos[0] && mouse[0] <= pos[0]+size[0]) && (mouse[1] >= pos[1] && mouse[1] <= pos[1]+size[1]);
}

bool GuiElement::MouseOver(float x, float y) const
{
	return (x >= pos[0] && x <= pos[0]+size[0]) && (y >= pos[1] && y <= pos[1]+size[1]);
}

void GuiElement::UpdateDisplayGeo(int x, int y, int xOffset, int yOffset)
{
	screensize[0] = x;
	screensize[1] = y;
	screenoffset[0] = xOffset;
	screenoffset[1] = yOffset;
}

float GuiElement::PixelToGlX(int x)
{
	return float(x - screenoffset[0])/float(screensize[0]);
}

float GuiElement::PixelToGlY(int y)
{
	return 1.0f - float(y - screenoffset[1])/float(screensize[1]);
}

float GuiElement::GlToPixelX(float x)
{
	return x*float(screensize[0]) + float(screenoffset[0]);
}

float GuiElement::GlToPixelY(float y)
{
	return y*float(screensize[1]) + float(screenoffset[1]);
}

void GuiElement::AddChild(GuiElement* elem)
{
	children.push_back(elem);
	elem->SetPos(pos[0], pos[1]);
	elem->SetSize(size[0], size[1]);
	GeometryChange();
}

void GuiElement::SetPos(float x, float y)
{
	pos[0] = x;
	pos[1] = y;
	GeometryChange();
}

void GuiElement::SetSize(float x, float y, bool fixed)
{
	size[0] = x;
	size[1] = y;
	fixedSize = fixed;
	GeometryChange();
}

void GuiElement::GeometryChange()
{
	GeometryChangeSelf();
	for (auto ch : children)
		ch->GeometryChange();
}

float GuiElement::DefaultOpacity() const
{
	return 0.8f;
}

void GuiElement::Move(float x, float y)
{
	pos[0] += x;
	pos[1] += y;
	for (auto ch : children)
		ch->Move(x, y);
}

#ifdef HEADLESS
void GuiElement::DrawBox(int primType, const SColor& color) {}
#else
void GuiElement::DrawBox(int primType, const SColor& color)
{
	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DC>();
	auto& sh = rb.GetShader();

	sh.Enable();
	switch (primType)
	{
	case GL_QUADS: {
		rb.AddQuadTriangles(
			{ pos[0]          , pos[1]          , color },
			{ pos[0] + size[0], pos[1]          , color },
			{ pos[0] + size[0], pos[1] + size[1], color },
			{ pos[0]          , pos[1] + size[1], color }
		);
		rb.DrawElements(GL_TRIANGLES);
	} break;
	case GL_LINE_LOOP: {
		rb.AddVertices({
			{ pos[0]          , pos[1]          , color },
			{ pos[0] + size[0], pos[1]          , color },
			{ pos[0] + size[0], pos[1] + size[1], color },
			{ pos[0]          , pos[1] + size[1], color }
		});
		rb.DrawArrays(primType);
	} break;
	default:
		assert(false);
		break;
	}
	sh.Disable();
}
#endif // !HEADLESS


}
