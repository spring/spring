/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cstring>

#include "AttribState.hpp"
#include "Rendering/GL/myGL.h"
#include "System/MainDefines.h"


template<typename GLgetFunc, typename GLparamType, typename ReturnType, size_t N>
static ReturnType glGetT(GLgetFunc glGetFunc, GLenum param) {
	ReturnType ret;
	GLparamType arr[N];

	static_assert(sizeof(ret) == sizeof(arr), "");

	memset(arr, 0, sizeof(arr));
	glGetError();
	glGetFunc(param, &arr[0]);
	assert(glGetError() == GL_NO_ERROR);
	memcpy(reinterpret_cast<GLparamType*>(&ret), &arr[0], sizeof(ret));

	return ret;
}

template<typename ReturnType = int, size_t N = 1>
static ReturnType glGetIntT(GLenum param) {
	return (glGetT<decltype(&glGetIntegerv), GLint, ReturnType, N>(glGetIntegerv, param));
}

template<typename ReturnType = float, size_t N = 1>
static ReturnType glGetFloatT(GLenum param) {
	return (glGetT<decltype(&glGetFloatv), GLfloat, ReturnType, N>(glGetFloatv, param));
};


static              GL::AttribState  glAttribStates[2];
static _threadlocal GL::AttribState* glAttribState = nullptr;

static _threadlocal decltype(&glEnable) glSetStateFuncs[2] = {nullptr, nullptr};

GL::AttribState* GL::SetAttribStatePointer(bool mainThread) { return (glAttribStates[mainThread].Init(), glAttribState = &glAttribStates[mainThread]); }
GL::AttribState* GL::GetAttribStatePointer(               ) { return (                                   glAttribState                              ); }



GL::AttribState::AttribState() {
	attribBitsStack.fill(0);
	depthClampStack.fill(GL_FALSE);
	depthTestStack.fill(GL_FALSE);
	depthMaskStack.fill(GL_FALSE);
	depthFuncStack.fill(GL_INVALID_ENUM);
	alphaTestStack.fill(GL_FALSE);
	alphaFuncStack.fill({GL_INVALID_ENUM, 0.0f});
	blendFuncStack.fill({GL_INVALID_ENUM, GL_INVALID_ENUM});
	blendFlagStack.fill(GL_FALSE);
	stencilTestStack.fill(GL_FALSE);
	scissorTestStack.fill(GL_FALSE);
	polyModeStack.fill({GL_INVALID_ENUM, GL_INVALID_ENUM});
	pofsCtrlStack.fill({0.0f, 0.0f});
	pofsFlagStack[0].fill(GL_FALSE);
	frontFaceStack.fill(GL_INVALID_ENUM);
	cullFaceStack.fill(GL_INVALID_ENUM);
	cullFlagStack.fill(GL_FALSE);
	colorMaskStack.fill({GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE});
}

void GL::AttribState::Init() {
	// avoid initializing twice if not mtLoading
	if (glSetStateFuncs[0] != nullptr)
		return;

	glSetStateFuncs[0] = glDisable;
	glSetStateFuncs[1] = glEnable;

	// set initial GL defaults, stateless rendering stopgap
	// TODO: stencil state, GL_{COLOR_LOGIC_OP,SAMPLE_ALPHA_TO_COVERAGE,PRIMITIVE_RESTART,CLIP_DISTANCEi}?
	PushDepthClamp(glIsEnabled(GL_DEPTH_CLAMP));
	PushDepthTest(glIsEnabled(GL_DEPTH_TEST));
	PushDepthMask(glGetIntT(GL_DEPTH_WRITEMASK));
	PushDepthFunc(glGetIntT(GL_DEPTH_FUNC));
	PushAlphaTest(glIsEnabled(GL_ALPHA_TEST));
	PushAlphaFunc(glGetIntT(GL_ALPHA_TEST_FUNC), glGetFloatT(GL_ALPHA_TEST_REF));
	// FIXME: does not return proper values until glBlendFunc has been called
	// PushBlendFunc(glGetIntT(GL_BLEND_SRC_ALPHA), glGetIntT(GL_BLEND_DST_ALPHA));
	PushBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	PushBlendFlag(glIsEnabled(GL_BLEND));
	PushStencilTest(glIsEnabled(GL_STENCIL_TEST));
	PushScissorTest(glIsEnabled(GL_SCISSOR_TEST));
	// FIXME: return values are for front- and back-facing polygons resp.
	// PushPolygonMode(glGetIntT<PolyModeState, 2>(GL_POLYGON_MODE));
	PushPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	PushPolygonOffset(glGetFloatT(GL_POLYGON_OFFSET_FACTOR), glGetFloatT(GL_POLYGON_OFFSET_UNITS));
	PushPolygonOffsetFill(glIsEnabled(GL_POLYGON_OFFSET_FILL));
	PushFrontFace(glGetIntT(GL_FRONT_FACE)); // GL_CCW
	PushCullFace(glGetIntT(GL_CULL_FACE_MODE)); // GL_BACK
	PushCullFlag(glIsEnabled(GL_CULL_FACE));
	PushColorMask(glGetIntT<ColorMaskState, 4>(GL_COLOR_WRITEMASK));
}


void GL::AttribState::Enable(uint32_t attrib) {
	switch (attrib) {
		case GL_DEPTH_CLAMP : { glEnable( depthClampStack[ depthClampStackSize - 1] = GL_TRUE); } break;
		case GL_DEPTH_TEST  : { glEnable(  depthTestStack[  depthTestStackSize - 1] = GL_TRUE); } break;
		case GL_ALPHA_TEST  : { glEnable(  alphaTestStack[  alphaTestStackSize - 1] = GL_TRUE); } break;
		case GL_BLEND       : { glEnable(  blendFlagStack[  blendFlagStackSize - 1] = GL_TRUE); } break;
		case GL_STENCIL_TEST: { glEnable(stencilTestStack[stencilTestStackSize - 1] = GL_TRUE); } break;
		case GL_SCISSOR_TEST: { glEnable(scissorTestStack[scissorTestStackSize - 1] = GL_TRUE); } break;
		case GL_CULL_FACE   : { glEnable(   cullFaceStack[   cullFaceStackSize - 1] = GL_TRUE); } break;
		default             : {                                                                 } break;
	}
}

void GL::AttribState::Disable(uint32_t attrib) {
	switch (attrib) {
		case GL_DEPTH_CLAMP : { glDisable( depthClampStack[ depthClampStackSize - 1] = GL_FALSE); } break;
		case GL_DEPTH_TEST  : { glDisable(  depthTestStack[  depthTestStackSize - 1] = GL_FALSE); } break;
		case GL_ALPHA_TEST  : { glDisable(  alphaTestStack[  alphaTestStackSize - 1] = GL_FALSE); } break;
		case GL_BLEND       : { glDisable(  blendFlagStack[  blendFlagStackSize - 1] = GL_FALSE); } break;
		case GL_STENCIL_TEST: { glDisable(stencilTestStack[stencilTestStackSize - 1] = GL_FALSE); } break;
		case GL_SCISSOR_TEST: { glDisable(scissorTestStack[scissorTestStackSize - 1] = GL_FALSE); } break;
		case GL_CULL_FACE   : { glDisable(   cullFaceStack[   cullFaceStackSize - 1] = GL_FALSE); } break;
		default             : {                                                                   } break;
	}
}


void GL::AttribState::Push(uint32_t attribBits) {
	assert(attribBitsStackSize < attribBitsStack.size());
	attribBitsStack[attribBitsStackSize++] = attribBits;

	if ((attribBits & GL_ENABLE_BIT) != 0) {
		assert(  depthTestStackSize <   depthTestStack.size());
		assert(stencilTestStackSize < stencilTestStack.size());
		assert(scissorTestStackSize < scissorTestStack.size());

		  depthTestStack[  depthTestStackSize] =   depthTestStack[  depthTestStackSize - 1];
		stencilTestStack[stencilTestStackSize] = stencilTestStack[stencilTestStackSize - 1];
		scissorTestStack[scissorTestStackSize] = scissorTestStack[scissorTestStackSize - 1];

		  depthTestStackSize++;
		stencilTestStackSize++;
		scissorTestStackSize++;
	}

	if ((attribBits & GL_POLYGON_BIT) != 0) {
		assert( polyModeStackSize <  polyModeStack.size());
		assert( pofsCtrlStackSize <  pofsCtrlStack.size());
		assert( cullFaceStackSize <  cullFaceStack.size());
		assert(frontFaceStackSize < frontFaceStack.size());

		 polyModeStack[  polyModeStackSize] =  polyModeStack[ polyModeStackSize - 1];
		 pofsCtrlStack[  pofsCtrlStackSize] =  pofsCtrlStack[ pofsCtrlStackSize - 1];
		 cullFaceStack[  cullFaceStackSize] =  cullFaceStack[ cullFaceStackSize - 1];
		frontFaceStack[ frontFaceStackSize] = frontFaceStack[frontFaceStackSize - 1];

		 polyModeStackSize++;
		 pofsCtrlStackSize++;
		 cullFaceStackSize++;
		frontFaceStackSize++;
	}

	if ((attribBits & GL_DEPTH_BUFFER_BIT) != 0) {
		assert(depthFuncStackSize < depthFuncStack.size());

		depthFuncStack[depthFuncStackSize] = depthFuncStack[depthFuncStackSize - 1];
		depthFuncStackSize++;
	}

	if ((attribBits & GL_COLOR_BUFFER_BIT) != 0) {
		assert(colorMaskStackSize < colorMaskStack.size());

		colorMaskStack[colorMaskStackSize] = colorMaskStack[colorMaskStackSize - 1];
		colorMaskStackSize++;
	}

	#if 0
	if ((attribBits & GL_STENCIL_BUFFER_BIT) != 0) {
	}

	if ((attribBits & GL_SCISSOR_BIT) != 0) {
	}
	#endif
}

void GL::AttribState::Pop() {
	assert(attribBitsStackSize > 0);
	--attribBitsStackSize;

	if ((attribBitsStack[attribBitsStackSize] & GL_ENABLE_BIT) != 0) {
		assert(  depthTestStackSize > 0);
		assert(stencilTestStackSize > 0);
		assert(scissorTestStackSize > 0);

		DepthTest(depthTestStack[--depthTestStackSize]);
		StencilTest(stencilTestStack[--stencilTestStackSize]);
		ScissorTest(scissorTestStack[--scissorTestStackSize]);
	}

	if ((attribBitsStack[attribBitsStackSize] & GL_POLYGON_BIT) != 0) {
		assert(polyModeStackSize > 0);
		assert(pofsCtrlStackSize > 0);
		assert(cullFaceStackSize > 0);
		assert(frontFaceStackSize > 0);

		PolygonMode(polyModeStack[--polyModeStackSize]);
		PolygonOffset(pofsCtrlStack[--pofsCtrlStackSize]);
		CullFace(cullFaceStack[--cullFaceStackSize]);
		FrontFace(frontFaceStack[--frontFaceStackSize]);
	}

	if ((attribBitsStack[attribBitsStackSize] & GL_DEPTH_BUFFER_BIT) != 0) {
		assert(depthFuncStackSize > 0);
		DepthFunc(depthFuncStack[--depthFuncStackSize]);
	}

	if ((attribBitsStack[attribBitsStackSize] & GL_COLOR_BUFFER_BIT) != 0) {
		assert(colorMaskStackSize > 0);
		ColorMask(colorMaskStack[--colorMaskStackSize]);
	}

	#if 0
	if ((attribBitsStack[attribBitsStackSize] & GL_STENCIL_BUFFER_BIT) != 0) {
	}

	if ((attribBitsStack[attribBitsStackSize] & GL_SCISSOR_BIT) != 0) {
	}
	#endif
}


void GL::AttribState::DepthClamp(bool enable) {
	assert(depthClampStackSize > 0);
	glSetStateFuncs[  depthClampStack[depthClampStackSize - 1] = enable  ](GL_DEPTH_CLAMP);
}
void GL::AttribState::PushDepthClamp(bool enable) {
	assert(depthClampStackSize < depthClampStack.size());
	glSetStateFuncs[  depthClampStack[depthClampStackSize++] = enable  ](GL_DEPTH_CLAMP);
}
void GL::AttribState::PopDepthClamp() {
	assert(depthClampStackSize > 0);
	glSetStateFuncs[  depthClampStack[--depthClampStackSize]  ](GL_DEPTH_CLAMP);
}


void GL::AttribState::DepthTest(bool enable) {
	assert(depthTestStackSize > 0);
	glSetStateFuncs[  depthTestStack[depthTestStackSize - 1] = enable  ](GL_DEPTH_TEST);
}
void GL::AttribState::PushDepthTest(bool enable) {
	assert(depthTestStackSize < depthTestStack.size());
	glSetStateFuncs[  depthTestStack[depthTestStackSize++] = enable  ](GL_DEPTH_TEST);
}
void GL::AttribState::PopDepthTest() {
	assert(depthTestStackSize > 0);
	glSetStateFuncs[  depthTestStack[--depthTestStackSize]  ](GL_DEPTH_TEST);
}


void GL::AttribState::DepthMask(bool enable) {
	assert(depthMaskStackSize > 0);
	glDepthMask(depthMaskStack[depthMaskStackSize - 1] = enable);
}
void GL::AttribState::PushDepthMask(bool enable) {
	assert(depthMaskStackSize < depthMaskStack.size());
	glDepthMask(depthMaskStack[depthMaskStackSize++] = enable);
}
void GL::AttribState::PopDepthMask() {
	assert(depthMaskStackSize > 0);
	glDepthMask(depthMaskStack[--depthMaskStackSize]);
}


void GL::AttribState::DepthFunc(uint32_t dtFunc) {
	assert(depthFuncStackSize > 0);
	glDepthFunc(depthFuncStack[depthFuncStackSize - 1] = dtFunc);
}
void GL::AttribState::PushDepthFunc(uint32_t dtFunc) {
	assert(depthFuncStackSize < depthFuncStack.size());
	glDepthFunc(depthFuncStack[depthFuncStackSize++] = dtFunc);
}
void GL::AttribState::PopDepthFunc() {
	assert(depthFuncStackSize > 0);
	glDepthFunc(depthFuncStack[--depthFuncStackSize]);
}


void GL::AttribState::AlphaTest(bool enable) {
	assert(alphaTestStackSize > 0);
	glSetStateFuncs[  alphaTestStack[alphaTestStackSize - 1] = enable  ](GL_ALPHA_TEST);
}
void GL::AttribState::PushAlphaTest(bool enable) {
	assert(alphaTestStackSize < alphaTestStack.size());
	glSetStateFuncs[  alphaTestStack[alphaTestStackSize++] = enable  ](GL_ALPHA_TEST);
}
void GL::AttribState::PopAlphaTest() {
	assert(alphaTestStackSize > 0);
	glSetStateFuncs[  alphaTestStack[--alphaTestStackSize]  ](GL_ALPHA_TEST);
}


void GL::AttribState::AlphaFunc(uint32_t atFunc, float refVal) {
	assert(alphaFuncStackSize > 0);
	glAlphaFunc(alphaFuncStack[alphaFuncStackSize - 1].atFunc = atFunc, alphaFuncStack[alphaFuncStackSize - 1].refVal = refVal);
}
void GL::AttribState::PushAlphaFunc(uint32_t atFunc, float refVal) {
	assert(alphaFuncStackSize < alphaFuncStack.size());
	alphaFuncStack[alphaFuncStackSize++] = {atFunc, refVal};
	glAlphaFunc(atFunc, refVal);
}
void GL::AttribState::PopAlphaFunc() {
	assert(alphaFuncStackSize > 0);
	--alphaFuncStackSize;
	glAlphaFunc(alphaFuncStack[alphaFuncStackSize].atFunc, alphaFuncStack[alphaFuncStackSize].refVal);
}


void GL::AttribState::BlendFunc(uint32_t srcFac, uint32_t dstFac) {
	assert(blendFuncStackSize > 0);
	glBlendFunc(blendFuncStack[blendFuncStackSize - 1].srcFac = srcFac, blendFuncStack[blendFuncStackSize - 1].dstFac = dstFac);
}
void GL::AttribState::PushBlendFunc(uint32_t srcFac, uint32_t dstFac) {
	assert(blendFuncStackSize < blendFuncStack.size());
	blendFuncStack[blendFuncStackSize++] = {srcFac, dstFac};
	glBlendFunc(srcFac, dstFac);
}
void GL::AttribState::PopBlendFunc() {
	assert(blendFuncStackSize > 0);
	--blendFuncStackSize;
	glBlendFunc(blendFuncStack[blendFuncStackSize].srcFac, blendFuncStack[blendFuncStackSize].dstFac);
}


void GL::AttribState::BlendFlag(bool enable) {
	assert(blendFlagStackSize > 0);
	glSetStateFuncs[  blendFlagStack[blendFlagStackSize - 1] = enable  ](GL_BLEND);
}
void GL::AttribState::PushBlendFlag(bool enable) {
	assert(blendFlagStackSize < blendFlagStack.size());
	glSetStateFuncs[  blendFlagStack[blendFlagStackSize++] = enable  ](GL_BLEND);
}
void GL::AttribState::PopBlendFlag() {
	assert(blendFlagStackSize > 0);
	glSetStateFuncs[  blendFlagStack[--blendFlagStackSize]  ](GL_BLEND);
}


void GL::AttribState::StencilTest(bool enable) {
	assert(stencilTestStackSize > 0);
	glSetStateFuncs[  stencilTestStack[stencilTestStackSize - 1] = enable  ](GL_STENCIL_TEST);
}
void GL::AttribState::PushStencilTest(bool enable) {
	assert(stencilTestStackSize < stencilTestStack.size());
	glSetStateFuncs[  stencilTestStack[stencilTestStackSize++] = enable  ](GL_STENCIL_TEST);
}
void GL::AttribState::PopStencilTest() {
	assert(stencilTestStackSize > 0);
	glSetStateFuncs[  stencilTestStack[--stencilTestStackSize]  ](GL_STENCIL_TEST);
}


void GL::AttribState::ScissorTest(bool enable) {
	assert(scissorTestStackSize > 0);
	glSetStateFuncs[  scissorTestStack[scissorTestStackSize - 1] = enable  ](GL_SCISSOR_TEST);
}
void GL::AttribState::PushScissorTest(bool enable) {
	assert(scissorTestStackSize < scissorTestStack.size());
	glSetStateFuncs[  scissorTestStack[scissorTestStackSize++] = enable  ](GL_SCISSOR_TEST);
}
void GL::AttribState::PopScissorTest() {
	assert(scissorTestStackSize > 0);
	glSetStateFuncs[  scissorTestStack[--scissorTestStackSize]  ](GL_SCISSOR_TEST);
}


void GL::AttribState::PolygonMode(uint32_t side, uint32_t mode) {
	assert(polyModeStackSize > 0);
	glPolygonMode(polyModeStack[polyModeStackSize - 1].side = side, polyModeStack[polyModeStackSize - 1].mode = mode);
}
void GL::AttribState::PushPolygonMode(uint32_t side, uint32_t mode) {
	assert(polyModeStackSize < polyModeStack.size());
	polyModeStack[polyModeStackSize++] = {side, mode};
	glPolygonMode(side, mode);
}
void GL::AttribState::PopPolygonMode() {
	assert(polyModeStackSize > 0);
	--polyModeStackSize;
	glPolygonMode(polyModeStack[polyModeStackSize].side, polyModeStack[polyModeStackSize].mode);
}


void GL::AttribState::PolygonOffset(float factor, float units) {
	assert(pofsCtrlStackSize > 0);
	glPolygonOffset(pofsCtrlStack[pofsCtrlStackSize - 1].factor = factor, pofsCtrlStack[pofsCtrlStackSize - 1].units = units);
}
void GL::AttribState::PushPolygonOffset(float factor, float units) {
	assert(pofsCtrlStackSize < pofsCtrlStack.size());
	pofsCtrlStack[pofsCtrlStackSize++] = {factor, units};
	glPolygonOffset(factor, units);
}
void GL::AttribState::PopPolygonOffset() {
	assert(pofsCtrlStackSize > 0);
	--pofsCtrlStackSize;
	glPolygonOffset(pofsCtrlStack[pofsCtrlStackSize].factor, pofsCtrlStack[pofsCtrlStackSize].units);
}



void GL::AttribState::PolygonOffsetFill(bool enable) {
	assert(pofsFlagStackSize[0] < pofsFlagStack[0].size());
	glSetStateFuncs[  pofsFlagStack[0][  pofsFlagStackSize[0] - 1  ] = enable  ](GL_POLYGON_OFFSET_FILL);
}
void GL::AttribState::PushPolygonOffsetFill(bool enable) {
	assert(pofsFlagStackSize[0] < pofsFlagStack[0].size());
	glSetStateFuncs[  pofsFlagStack[0][  pofsFlagStackSize[0]++  ] = enable  ](GL_POLYGON_OFFSET_FILL);
}
void GL::AttribState::PopPolygonOffsetFill() {
	assert(pofsFlagStackSize[0] > 0);
	glSetStateFuncs[  pofsFlagStack[0][  --pofsFlagStackSize[0]  ]  ](GL_POLYGON_OFFSET_FILL);
}

#if 0
void GL::AttribState::PolygonOffsetPoint(bool enable) {
	assert(pofsFlagStackSize[1] < pofsFlagStack[1].size());
	glSetStateFuncs[  pofsFlagStack[1][  pofsFlagStackSize[1] - 1  ] = enable  ](GL_POLYGON_OFFSET_POINT);
}
void GL::AttribState::PushPolygonOffsetPoint(bool enable) {
	assert(pofsFlagStackSize[1] < pofsFlagStack[1].size());
	glSetStateFuncs[  pofsFlagStack[1][  pofsFlagStackSize[1]++  ] = enable  ](GL_POLYGON_OFFSET_POINT);
}
void GL::AttribState::PopPolygonOffsetPoint() {
	assert(pofsFlagStackSize[1] > 0);
	glSetStateFuncs[  pofsFlagStack[1][  --pofsFlagStackSize[1]  ]  ](GL_POLYGON_OFFSET_POINT);
}

void GL::AttribState::PolygonOffsetLine(bool enable) {
	assert(pofsFlagStackSize[2] < pofsFlagStack[2].size());
	glSetStateFuncs[  pofsFlagStack[2][  pofsFlagStackSize[2] - 1  ] = enable  ](GL_POLYGON_OFFSET_LINE);
}
void GL::AttribState::PushPolygonOffsetLine(bool enable) {
	assert(pofsFlagStackSize[2] < pofsFlagStack[2].size());
	glSetStateFuncs[  pofsFlagStack[2][  pofsFlagStackSize[2]++  ] = enable  ](GL_POLYGON_OFFSET_LINE);
}
void GL::AttribState::PopPolygonOffsetLine() {
	assert(pofsFlagStackSize[2] > 0);
	glSetStateFuncs[  pofsFlagStack[2][  --pofsFlagStackSize[2]  ]  ](GL_POLYGON_OFFSET_LINE);
}
#endif



void GL::AttribState::FrontFace(uint32_t face) {
	assert(frontFaceStackSize > 0);
	glFrontFace(frontFaceStack[frontFaceStackSize - 1] = face);
}
void GL::AttribState::PushFrontFace(uint32_t face) {
	assert(frontFaceStackSize < frontFaceStack.size());
	glFrontFace(frontFaceStack[frontFaceStackSize++] = face);
}
void GL::AttribState::PopFrontFace() {
	assert(frontFaceStackSize > 0);
	glFrontFace(frontFaceStack[--frontFaceStackSize]);
}


void GL::AttribState::CullFace(uint32_t face) {
	assert(cullFaceStackSize > 0);
	glCullFace(cullFaceStack[cullFaceStackSize - 1] = face);
}
void GL::AttribState::PushCullFace(uint32_t face) {
	assert(cullFaceStackSize < cullFaceStack.size());
	glCullFace(cullFaceStack[cullFaceStackSize++] = face);
}
void GL::AttribState::PopCullFace() {
	assert(cullFaceStackSize > 0);
	glCullFace(cullFaceStack[--cullFaceStackSize]);
}


void GL::AttribState::CullFlag(bool enable) {
	assert(cullFlagStackSize > 0);
	glSetStateFuncs[  cullFlagStack[cullFlagStackSize - 1] = enable  ](GL_CULL_FACE);
}
void GL::AttribState::PushCullFlag(bool enable) {
	assert(cullFlagStackSize < cullFlagStack.size());
	glSetStateFuncs[  cullFlagStack[cullFlagStackSize++] = enable  ](GL_CULL_FACE);
}
void GL::AttribState::PopCullFlag() {
	assert(cullFlagStackSize > 0);
	glSetStateFuncs[  cullFlagStack[--cullFlagStackSize]  ](GL_CULL_FACE);
}


void GL::AttribState::ColorMask(bool r, bool g, bool b, bool a) {
	assert(colorMaskStackSize > 0);
	glColorMask(colorMaskStack[colorMaskStackSize - 1].r = r, colorMaskStack[colorMaskStackSize - 1].g = g, colorMaskStack[colorMaskStackSize - 1].b = b, colorMaskStack[colorMaskStackSize - 1].a = a);
}
void GL::AttribState::PushColorMask(bool r, bool g, bool b, bool a) {
	assert(colorMaskStackSize < colorMaskStack.size());
	glColorMask(colorMaskStack[colorMaskStackSize].r = r, colorMaskStack[colorMaskStackSize].g = g, colorMaskStack[colorMaskStackSize].b = b, colorMaskStack[colorMaskStackSize].a = a);
	colorMaskStackSize++;
}
void GL::AttribState::PopColorMask() {
	assert(colorMaskStackSize > 0);
	--colorMaskStackSize;
	glColorMask(colorMaskStack[colorMaskStackSize].r, colorMaskStack[colorMaskStackSize].g, colorMaskStack[colorMaskStackSize].b, colorMaskStack[colorMaskStackSize].a);
}




GL::ScopedDepthFunc::~ScopedDepthFunc() { glDepthFunc(prevFunc); }
GL::ScopedDepthFunc::ScopedDepthFunc(uint32_t func) {
	glGetIntegerv(GL_DEPTH_FUNC, reinterpret_cast<GLint*>(&prevFunc));
	glDepthFunc(func);
}


GL::ScopedCullFace::~ScopedCullFace() { glCullFace(prevFace); }
GL::ScopedCullFace::ScopedCullFace(uint32_t face) {
	glGetIntegerv(GL_CULL_FACE_MODE, reinterpret_cast<GLint*>(&prevFace));
	glCullFace(face);
}

