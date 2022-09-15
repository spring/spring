#pragma once

#include <memory>

#include "Rendering/GL/VertexArrayTypes.h"
#include "Rendering/GL/RenderBuffers.h"

class CglFont;
class CglFontRenderer {
public:
	virtual ~CglFontRenderer() {}

	virtual void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) = 0;
	virtual void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) = 0;
	virtual void DrawTraingleElements() = 0;
	virtual void PushGLState(const CglFont* font) = 0;
	virtual void PopGLState() = 0;

	virtual bool IsLegacy() const = 0;
	virtual bool IsValid() const = 0;
	virtual void GetStats(std::array<size_t, 8>& stats) const = 0;

	static CglFontRenderer* CreateInstance();
	static void DeleteInstance(CglFontRenderer*& instance);
protected:
	GLint currProgID = 0;

	// should be enough to hold all data for a given frame
	static constexpr size_t NUM_BUFFER_ELEMS = (1 << 14);
	static constexpr size_t NUM_TRI_BUFFER_VERTS = (4 * NUM_BUFFER_ELEMS);
	static constexpr size_t NUM_TRI_BUFFER_ELEMS = (6 * NUM_BUFFER_ELEMS);
};

namespace Shader {
	struct IProgramObject;
};
class CglShaderFontRenderer final: public CglFontRenderer {
public:
	CglShaderFontRenderer();
	virtual ~CglShaderFontRenderer() override;

	virtual void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	virtual void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	virtual void DrawTraingleElements() override;
	virtual void PushGLState(const CglFont* font) override;
	virtual void PopGLState() override;

	virtual bool IsLegacy() const override { return false; }
	virtual bool IsValid() const override { return fontShader->IsValid(); }
	virtual void GetStats(std::array<size_t, 8>& stats) const override;
private:
	TypedRenderBuffer<VA_TYPE_TC> primaryBufferTC;
	TypedRenderBuffer<VA_TYPE_TC> outlineBufferTC;

	static inline size_t fontShaderRefs = 0;
	static inline std::unique_ptr<Shader::IProgramObject> fontShader = nullptr;
};

class CglNoShaderFontRenderer final: public CglFontRenderer {
public:
	CglNoShaderFontRenderer();
	virtual ~CglNoShaderFontRenderer() override;

	virtual void AddQuadTrianglesPB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	virtual void AddQuadTrianglesOB(VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl) override;
	virtual void DrawTraingleElements() override;
	virtual void PushGLState(const CglFont* font) override;
	virtual void PopGLState() override;

	virtual bool IsLegacy() const override { return true; }
	virtual bool IsValid() const override { return true; }
	virtual void GetStats(std::array<size_t, 8>& stats) const override;
private:
	void AddQuadTrianglesImpl(bool primary, VA_TYPE_TC&& tl, VA_TYPE_TC&& tr, VA_TYPE_TC&& br, VA_TYPE_TC&& bl);

	std::array<std::vector<VA_TYPE_TC>, 2> verts; // OL, PM
	std::array<std::vector<uint16_t  >, 2> indcs; // OL, PM
};