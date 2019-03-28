/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_WIDE_LINE_ADAPTER_HDR
#define GL_WIDE_LINE_ADAPTER_HDR

#include "AttribState.hpp"
#include "RenderDataBuffer.hpp"
#include "System/FastMath.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"

#include <vector>

namespace GL {
	std::vector<float>* GetWideLineBuffer();
template<typename VertexArrayType>
class WideLineAdapter {
public:
	static constexpr size_t VAT_IN_FLOATS = sizeof(VertexArrayType) / sizeof(float);

	void Setup(TRenderDataBuffer<VertexArrayType>* b, float x, float y, float w, const CMatrix44f& t) {
		assert(offset == 0);
		buffer = b;
		xScale = x;
		yScale = y;
		ixScale = 1.0f / x;
		iyScale = 1.0f / y;
		width = w;
		transform = t;
		invTransform = t.Invert();
	}
	bool CheckSizeE(size_t ne) const { return ((offset + (ne - 1)) < (GL::GetWideLineBuffer()->size() / VAT_IN_FLOATS)); }

	void AssertSizeE(size_t ne) const { assert(CheckSizeE(ne)); }

	void Append(const VertexArrayType& e) { Append(&e, 1); }
	void Append(const VertexArrayType* e, size_t ne) { AssertSizeE(ne); std::memcpy(&GL::GetWideLineBuffer()->at(offset * VAT_IN_FLOATS), e, ne * sizeof(VertexArrayType)); offset += ne; }

	void SafeAppend(const VertexArrayType& e) { SafeAppend(&e, 1); }
	void SafeAppend(const VertexArrayType* e, size_t ne) {
		if (ne == 0 || !CheckSizeE(ne))
			return;
		Append(e, ne);
	}

	void Submit(uint32_t primType) {
		assert(primType == GL_LINES || primType == GL_LINE_STRIP || primType == GL_LINE_LOOP ||primType == GL_QUADS);

		if (offset == 0) {
			assert(buffer->NumElems() == 0);
			buffer->Submit(primType); // shouldn't do anything, but someone asked submit, so we submit
			return;
		}

		VertexArrayType* va = reinterpret_cast<VertexArrayType*>(GetWideLineBuffer()->data());

		if (width <= 1.0f) { // no need to convert to quads
			buffer->SafeAppend(va, offset);
			buffer->Submit(primType);
			offset = 0;
			return;
		}
		glAttribStatePtr->PushPolygonMode();
		glAttribStatePtr->PolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		const size_t vertexCount = offset;
		size_t lineCount;
		switch(primType) {
			case GL_LINES      : { lineCount = vertexCount / 2; break; }
			case GL_LINE_STRIP : { lineCount = vertexCount - 1; break; }
			case GL_LINE_LOOP  : { lineCount = vertexCount;     break; }
			case GL_QUADS      : { lineCount = vertexCount;     break; }
			default: { assert(false); break; }
		}

		//TODO - smooth line connections, prevent quad overlap
		for (size_t i = 0; i < lineCount; ++i) {
			VertexArrayType* v1;
			VertexArrayType* v2;
			switch(primType) {
				case GL_LINES      : { v1 = va + 2 * i; v2 = va + 2 * i + 1;                break; }
				case GL_LINE_STRIP : { v1 = va + i;     v2 = va + i + 1;                    break; }
				case GL_LINE_LOOP  : { v1 = va + i;     v2 = va + (i + 1) % vertexCount;    break; }
				case GL_QUADS      : { v1 = va + i;     v2 = va + i + 1 - 4 * (i % 4 == 3); break; }
				default: { assert(false); break; }
			}
			OutputQuad(*v1, *v2, va);
		}

		buffer->SafeAppend(va + vertexCount, offset - vertexCount);
		buffer->Submit(GL_QUADS);
		offset = 0;

		glAttribStatePtr->PopPolygonMode();
	}
private:
	inline void OutputVertex(const VertexArrayType& v, const float3& c, VertexArrayType* va) {
		VertexArrayType& vref = va[offset++];
		vref = v;
		const float4 vc = invTransform * float4(c, 1.0f);
		vref.p = vc / vc.w;
	}

	void OutputQuad(const VertexArrayType& v1, const VertexArrayType& v2, VertexArrayType* va) {
		const float4 c1 = transform * float4(v1.p, 1.0f);
		const float3 cf1 = c1 / c1.w;
		const float4 c2 = transform * float4(v2.p, 1.0f);
		const float3 cf2 = c2 / c2.w;
		const float dx = (cf1.x - cf2.x) * xScale;
		const float dy = (cf2.y - cf1.y) * yScale;
		const float invLength = math::isqrt(dx * dx + dy * dy) * width;
		const float3 perp(dy * invLength * ixScale, dx * invLength * iyScale, 0.0f);
		const float3 c1Left = cf1 + perp;
		const float3 c1Right = cf1 - perp;
		const float3 c2Left = cf2 - perp;
		const float3 c2Right = cf2 + perp;

		OutputVertex(v1, c1Left,  va);
		OutputVertex(v1, c1Right, va);
		OutputVertex(v2, c2Left,  va);
		OutputVertex(v2, c2Right, va);
	}


	TRenderDataBuffer<VertexArrayType>* buffer;
	size_t offset = 0; // in sizeof(VertexArrayType) / sizeof(float)
	float xScale;
	float yScale;
	float ixScale;
	float iyScale;
	float width;
	CMatrix44f transform;
	CMatrix44f invTransform;
};
	typedef WideLineAdapter<VA_TYPE_C> WideLineAdapterC;
	WideLineAdapterC* GetWideLineAdapterC();
}

#endif //GL_WIDE_LINE_ADAPTER_HDR
