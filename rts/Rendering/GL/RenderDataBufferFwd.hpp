/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_RENDERDATABUFFER_FWD_HDR
#define GL_RENDERDATABUFFER_FWD_HDR

#include "VertexArrayTypes.h"

namespace Shader {
	struct IProgramObject;
};
namespace GL {
	struct RenderDataBuffer;
	template<typename T> struct TRenderDataBuffer;
	template<typename T> struct WideLineAdapter;

	typedef TRenderDataBuffer<VA_TYPE_0> RenderDataBuffer0;
	typedef TRenderDataBuffer<VA_TYPE_C> RenderDataBufferC;
	typedef TRenderDataBuffer<VA_TYPE_T> RenderDataBufferT;

	typedef TRenderDataBuffer<VA_TYPE_T4> RenderDataBufferT4;
	typedef TRenderDataBuffer<VA_TYPE_TN> RenderDataBufferTN;
	typedef TRenderDataBuffer<VA_TYPE_TC> RenderDataBufferTC;

	typedef TRenderDataBuffer<VA_TYPE_2d0> RenderDataBuffer2D0;
	typedef TRenderDataBuffer<VA_TYPE_2dT> RenderDataBuffer2DT;

	typedef TRenderDataBuffer<VA_TYPE_L> RenderDataBufferL;

	typedef WideLineAdapter<VA_TYPE_C> WideLineAdapterC;
};

#endif

