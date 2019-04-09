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
 : shader(shaderHandler->CreateProgramObject("[aGui::Gui]", "aGui::Gui", true))
 , inputCon(input.AddHandler(std::bind(&Gui::HandleEvent, this, std::placeholders::_1)))
{
	{
		vfsHandler->AddArchive(CArchiveScanner::GetSpringBaseContentName(), false);
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GuiVertProg4.glsl", "", GL_VERTEX_SHADER));
		shader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GuiFragProg4.glsl", "", GL_FRAGMENT_SHADER));
		shader->Link();
		vfsHandler->RemoveArchive(CArchiveScanner::GetSpringBaseContentName());
	}
	{
		const std::string& name = shader->GetName();
		const std::string& log = shader->GetLog();

		if (!shader->IsValid()) {
			LOG_L(L_ERROR, "%s-shader compilation error: %s", name.c_str(), log.c_str());

			// dummies
			shader->SetUniformLocation("");
			shader->SetUniformLocation("");
			shader->SetUniformLocation("");
			return;
		}

		shader->Enable();
		shader->SetUniformLocation("tex"); // idx 0
		shader->SetUniformLocation("elemColor"); // idx 1
		shader->SetUniformLocation("texWeight"); // idx 2

		shader->SetUniformMatrix4x4<float>("viewProjMat", false, CMatrix44f::ClipOrthoProj01(globalRendering->supportClipSpaceControl * 1.0f));
		shader->SetUniformMatrix2x2<float>("texCoorMat", false, float4(1.0f, 0.0f, 0.0f, 1.0f)); // name-based for Font::*GL4
		shader->SetUniform1i(0, 0);

		shader->Disable();
		shader->Validate();

		if (shader->IsValid())
			return;

		LOG_L(L_ERROR, "%s-shader validation error: %s", name.c_str(), log.c_str());
	}
}


void Gui::SetColor(float r, float g, float b, float a)
{
	shader->SetUniform4f(1, r, g, b, a);
}


void Gui::SetDrawMode(DrawMode newMode)
{
	if (currentDrawMode == newMode)
		return;

	switch (currentDrawMode = newMode) {
		case COLOR  : { shader->SetUniform4f(2,  0.0f, 0.0f, 0.0f, 0.0f); } break;
		case TEXTURE: { shader->SetUniform4f(2,  1.0f, 1.0f, 1.0f, 1.0f); } break;
		case FONT   : { shader->SetUniform4f(2, -1.0f, 0.0f, 0.0f, 0.0f); } break;
	}
}


void Gui::Draw()
{
	Clean();

	glAttribStatePtr->EnableBlendMask();

	shader->Enable();
	SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetDrawMode(DrawMode::COLOR);

	// not depth-sorted
	for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
		(*it).element->Draw();
	}

	shader->Disable();
	font->SetTextDepth(0.0f);
}

void Gui::Clean() {
	for (const GuiItem& item: toBeAdded) {
		const auto iter = std::find_if(elements.cbegin(), elements.cend(), [&](const GuiItem& i) { return (item.element == i.element); });

		if (iter == elements.end()) {
			if (item.asBackground) {
				elements.push_back(item);
			} else {
				elements.push_front(item);
			}
			continue;
		}

		LOG_L(L_DEBUG, "[Gui::%s] not adding duplicated object", __func__);
	}
	toBeAdded.clear();

	for (const GuiItem& item: toBeRemoved) {
		auto iter = std::find_if(elements.begin(), elements.end(), [&](const GuiItem& i) { return (item.element == i.element); });

		if (iter == elements.end())
			continue;

		delete iter->element;
		elements.erase(iter);
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
	for (auto it = elements.begin(); it != elements.end(); ++it) {
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
	for (auto it = elements.cbegin(); it != elements.cend(); ++it) {
		if (it->element->MouseOver(x, y))
			return (it->element == elem);
	}

	return false;
}

bool Gui::HandleEvent(const SDL_Event& ev)
{
	auto handler = elements.end();
	for (auto it = elements.begin(); it != elements.end(); ++it) {
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
