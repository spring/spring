/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Picture.h"

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"

namespace agui
{

	Picture::Picture(GuiElement* parent)
		: GuiElement(parent)
		, texture(0)
	{
	}

	Picture::~Picture()
	{
		if (texture) {
			glDeleteTextures(1, &texture);
		}
	}

	void Picture::Load(const std::string& _file)
	{
		file = _file;

		CBitmap bmp;
		if (bmp.Load(file)) {
			texture = bmp.CreateTexture();
		}
		else {
			LOG_L(L_WARNING, "Failed to load: %s", file.c_str());
			texture = 0;
		}
	}

#ifdef HEADLESS
	void Picture::DrawSelf() {}
#else
	void Picture::DrawSelf()
	{
		if (texture) {
			auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC>();
			auto& sh = rb.GetShader();
			const SColor color = { 1.0f, 1.0f, 1.0f, 1.0f };

			rb.AddQuadTriangles(
				{ pos[0]          , pos[1]          , 0.0f, 1.0f, color },
				{ pos[0] + size[0], pos[1]          , 1.0f, 1.0f, color },
				{ pos[0] + size[0], pos[1] + size[1], 1.0f, 0.0f, color },
				{ pos[0]          , pos[1] + size[1], 0.0f, 0.0f, color }
			);

			glBindTexture(GL_TEXTURE_2D, texture);
			sh.Enable();
			rb.DrawElements(GL_TRIANGLES);
			sh.Disable();
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}
#endif

} // namespace agui
