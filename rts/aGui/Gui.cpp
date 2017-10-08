/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Input/InputHandler.h"
#include "Gui.h"

#include <functional>
#include <SDL_events.h>

#include "GuiElement.h"
#include "Game/Camera.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/Log/ILog.h"
#include "System/Matrix44f.h"


namespace agui
{

Gui::Gui()
 : currentDrawMode(DrawMode::COLOR)
 , shader(nullptr)
{
	inputCon = input.AddHandler(std::bind(&Gui::HandleEvent, this, std::placeholders::_1));

	shader = shaderHandler->CreateProgramObject("[aGui::Gui]", "aGui::Gui", true);
	{
		const std::string archiveName = CArchiveScanner::GetSpringBaseContentName();
		vfsHandler->AddArchive(archiveName, false);
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GuiFragProg.glsl", "", GL_FRAGMENT_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GuiVertProg.glsl", "", GL_VERTEX_SHADER));
		shader->Link();
		vfsHandler->RemoveArchive(archiveName);
	}

	if (!shader->IsValid()) {
		const char* fmt = "%s-shader compilation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
		return;
	}

	shader->Enable();
	shader->SetUniformLocation("viewProjMatrix");
	shader->SetUniformLocation("tex");
	shader->SetUniformLocation("color");
	shader->SetUniformLocation("texWeight");

	shader->SetUniformMatrix4fv(0, false, CMatrix44f::OrthoProj(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f));
	shader->SetUniform1i(1, 0);

	shader->Disable();
	shader->Validate();

	if (!shader->IsValid()) {
		const char* fmt = "%s-shader validation error: %s";
		LOG_L(L_ERROR, fmt, shader->GetName().c_str(), shader->GetLog().c_str());
	}
}


void Gui::SetColor(float r, float g, float b, float a)
{
	shader->SetUniform4f(2, r, g, b, a);
}

void Gui::SetColor(const float* v)
{
	shader->SetUniform4fv(2, v);
}

void Gui::GuiColorCallback(const float* v)
{
	gui->SetColor(v);
}

void Gui::SetDrawMode(DrawMode newMode)
{
	if (currentDrawMode == newMode)
		return;

	switch (currentDrawMode = newMode) {
		case COLOR: {
			shader->SetUniform4f(3, 0.0f, 0.0f, 0.0f, 0.0f);
		} break;
		case TEXTURE: {
			shader->SetUniform4f(3, 1.0f, 1.0f, 1.0f, 1.0f);
		} break;
		case MASK: {
			shader->SetUniform4f(3, 0.0f, 0.0f, 0.0f, 1.0f);
		} break;
		case TEXT: {
		} break;
	}
}


void Gui::Draw()
{
	Clean();

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);

	shader->Enable();
	SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	font->SetColorCallback(GuiColorCallback);
	smallFont->SetColorCallback(GuiColorCallback);
	SetDrawMode(DrawMode::COLOR);

	for (ElList::reverse_iterator it = elements.rbegin(); it != elements.rend(); ++it) {
		(*it).element->Draw();
	}

	font->SetColorCallback(nullptr);
	smallFont->SetColorCallback(nullptr);
	shader->Disable();
}

void Gui::Clean() {
	for (ElList::iterator it = toBeAdded.begin(); it != toBeAdded.end(); ++it) {
		bool duplicate = false;
		for (ElList::iterator elIt = elements.begin(); elIt != elements.end(); ++elIt) {
			if (it->element == elIt->element) {
				LOG_L(L_DEBUG, "Gui::AddElement: skipping duplicated object");
				duplicate = true;
				break;
			}
		}
		if (!duplicate) {
			if (it->asBackground)
				elements.push_back(*it);
			else
				elements.push_front(*it);
		}
	}
	toBeAdded.clear();

	for (ElList::iterator it = toBeRemoved.begin(); it != toBeRemoved.end(); ++it) {
		for (ElList::iterator elIt = elements.begin(); elIt != elements.end(); ++elIt) {
			if (it->element == elIt->element) {
				delete (elIt->element);
				elements.erase(elIt);
				break;
			}
		}
	}
	toBeRemoved.clear();
}

Gui::~Gui() {
	Clean();
	inputCon.disconnect();
	shaderHandler->ReleaseProgramObjects("[aGui::Gui]");
}

void Gui::AddElement(GuiElement* elem, bool asBackground)
{
	toBeAdded.emplace_back(elem, asBackground);
}

void Gui::RmElement(GuiElement* elem)
{
	// has to be delayed, otherwise deleting a button during a callback would segfault
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it) {
		if ((*it).element == elem) {
			toBeRemoved.emplace_back(elem, true);
			break;
		}
	}
}

void Gui::UpdateScreenGeometry(int screenx, int screeny, int screenOffsetX, int screenOffsetY)
{
	GuiElement::UpdateDisplayGeo(screenx, screeny, screenOffsetX, screenOffsetY);
}

bool Gui::MouseOverElement(const GuiElement* elem, int x, int y) const
{
	for (ElList::const_iterator it = elements.begin(); it != elements.end(); ++it) {
		if (it->element->MouseOver(x, y))
			return (it->element == elem);
	}

	return false;
}

bool Gui::HandleEvent(const SDL_Event& ev)
{
	ElList::iterator handler = elements.end();
	for (ElList::iterator it = elements.begin(); it != elements.end(); ++it) {
		if (it->element->HandleEvent(ev)) {
			handler = it;
			break;
		}
	}
	if (handler != elements.end() && !handler->asBackground) {
		elements.push_front(*handler);
		elements.erase(handler);
	}
	return false;
}


Gui* gui = nullptr;

}
