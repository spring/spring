/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cassert>
#include <cmath>
#include <cstring>

#include "AttribState.hpp"
#include "Rendering/GlobalRendering.h"
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
	attribBitsStack.Fill(0);
	depthRangeStack.Fill({0.0f, 1.0f});
	depthClampStack.Fill(GL_FALSE);
	depthTestStack.Fill(GL_FALSE);
	depthMaskStack.Fill(GL_FALSE);
	depthFuncStack.Fill(GL_INVALID_ENUM);
	alphaTestStack.Fill(GL_FALSE);
	alphaFuncStack.Fill({GL_INVALID_ENUM, 0.0f});
	blendFuncStack.Fill({GL_INVALID_ENUM, GL_INVALID_ENUM});
	blendMaskStack.Fill(GL_FALSE);
	scissorTestStack.Fill(GL_FALSE);
	stencilTestStack.Fill(GL_FALSE);
	stencilMaskStack.Fill(0);
	stencilFuncStack.Fill({GL_INVALID_ENUM,               0,               0});
	stencilOperStack.Fill({GL_INVALID_ENUM, GL_INVALID_ENUM, GL_INVALID_ENUM});
	polyModeStack.Fill({GL_INVALID_ENUM, GL_INVALID_ENUM});
	pofsCtrlStack.Fill({0.0f, 0.0f});
	pofsFlagStack[0].Fill(GL_FALSE);
	pofsFlagStack[1].Fill(GL_FALSE);
	pofsFlagStack[2].Fill(GL_FALSE);
	frontFaceStack.Fill(GL_INVALID_ENUM);
	cullFaceStack.Fill(GL_INVALID_ENUM);
	cullFlagStack.Fill(GL_FALSE);
	blendColorStack.Fill({0.0f, 0.0f, 0.0f, 0.0f});
	colorMaskStack.Fill({GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE});
	viewportStack.Fill({0, 0, 0, 0});
	scissorStack.Fill({0, 0, 0, 0});
}

void GL::AttribState::Init() {
	// avoid initializing twice if not mtLoading
	if (glSetStateFuncs[0] != nullptr)
		return;

	glSetStateFuncs[0] = glDisable;
	glSetStateFuncs[1] = glEnable;

	// copy initial GL state from GlobalRendering::InitGLState, stateless rendering stopgap
	// TODO: GL_{COLOR_LOGIC_OP,SAMPLE_ALPHA_TO_COVERAGE,PRIMITIVE_RESTART,CLIP_DISTANCEi}?
	PushDepthRange(glGetFloatT<DepthRangeState, 2>(GL_DEPTH_RANGE));
	PushDepthClamp(glIsEnabled(GL_DEPTH_CLAMP));
	PushDepthTest(glIsEnabled(GL_DEPTH_TEST));
	PushDepthMask(glGetIntT(GL_DEPTH_WRITEMASK));
	PushDepthFunc(glGetIntT(GL_DEPTH_FUNC));
	PushAlphaTest(glIsEnabled(GL_ALPHA_TEST));
	PushAlphaFunc(glGetIntT(GL_ALPHA_TEST_FUNC), glGetFloatT(GL_ALPHA_TEST_REF));
	// FIXME: does not return proper values until glBlendFunc has been called
	// PushBlendFunc(glGetIntT(GL_BLEND_SRC_ALPHA), glGetIntT(GL_BLEND_DST_ALPHA));
	PushBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	PushBlendMask(glIsEnabled(GL_BLEND));
	PushScissorTest(glIsEnabled(GL_SCISSOR_TEST));
	PushStencilTest(glIsEnabled(GL_STENCIL_TEST));
	// FIXME: return values are for front- and back-facing polygons resp.
	// PushPolygonMode(glGetIntT<PolyModeState, 2>(GL_POLYGON_MODE));
	PushPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	PushPolygonOffset(glGetFloatT(GL_POLYGON_OFFSET_FACTOR), glGetFloatT(GL_POLYGON_OFFSET_UNITS));
	PushPolygonOffsetFill(glIsEnabled(GL_POLYGON_OFFSET_FILL));
	PushPolygonOffsetPoint(glIsEnabled(GL_POLYGON_OFFSET_POINT));
	PushPolygonOffsetLine(glIsEnabled(GL_POLYGON_OFFSET_LINE));
	PushFrontFace(glGetIntT(GL_FRONT_FACE)); // GL_CCW
	PushCullFace(glGetIntT(GL_CULL_FACE_MODE)); // GL_BACK
	PushCullFlag(glIsEnabled(GL_CULL_FACE));
	PushBlendColor(glGetFloatT<BlendColorState, 4>(GL_BLEND_COLOR));
	PushColorMask(glGetIntT<ColorMaskState, 4>(GL_COLOR_WRITEMASK));
	PushViewPort(glGetIntT<ViewPortState, 4>(GL_VIEWPORT));
	PushScissor(glGetIntT<ScissorState, 4>(GL_SCISSOR_BOX));
}


void GL::AttribState::Enable(uint32_t attrib) {
	switch (attrib) {
		case GL_DEPTH_CLAMP         : { EnableDepthClamp  (); return; } break; // unofficial
		case GL_DEPTH_TEST          : { EnableDepthTest   (); return; } break;
		case GL_ALPHA_TEST          : { EnableAlphaTest   (); return; } break;
		case GL_BLEND               : { EnableBlendMask   (); return; } break;
		case GL_SCISSOR_TEST        : { EnableScissorTest (); return; } break;
		case GL_STENCIL_TEST        : { EnableStencilTest (); return; } break;
		case GL_CULL_FACE           : { EnableCullFace    (); return; } break;
		case GL_POLYGON_OFFSET_FILL : { EnablePolyOfsFill (); return; } break;
		case GL_POLYGON_OFFSET_POINT: { EnablePolyOfsPoint(); return; } break;
		case GL_POLYGON_OFFSET_LINE : { EnablePolyOfsLine (); return; } break;
		default                     : { glEnable(attrib);             } break; // untracked
	}
}

void GL::AttribState::Disable(uint32_t attrib) {
	switch (attrib) {
		case GL_DEPTH_CLAMP         : { DisableDepthClamp  (); return; } break; // unofficial
		case GL_DEPTH_TEST          : { DisableDepthTest   (); return; } break;
		case GL_ALPHA_TEST          : { DisableAlphaTest   (); return; } break;
		case GL_BLEND               : { DisableBlendMask   (); return; } break;
		case GL_SCISSOR_TEST        : { DisableScissorTest (); return; } break;
		case GL_STENCIL_TEST        : { DisableStencilTest (); return; } break;
		case GL_CULL_FACE           : { DisableCullFace    (); return; } break;
		case GL_POLYGON_OFFSET_FILL : { DisablePolyOfsFill (); return; } break;
		case GL_POLYGON_OFFSET_POINT: { DisablePolyOfsPoint(); return; } break;
		case GL_POLYGON_OFFSET_LINE : { DisablePolyOfsLine (); return; } break;
		default                     : { glDisable(attrib);             } break; // untracked
	}
}


void GL::AttribState::EnableDepthMask   () {          DepthMask(GL_TRUE); }
void GL::AttribState::EnableDepthClamp  () {         DepthClamp(GL_TRUE); }
void GL::AttribState::EnableDepthTest   () {          DepthTest(GL_TRUE); }
void GL::AttribState::EnableAlphaTest   () {          AlphaTest(GL_TRUE); }
void GL::AttribState::EnableBlendMask   () {          BlendMask(GL_TRUE); }
void GL::AttribState::EnableScissorTest () {        ScissorTest(GL_TRUE); }
void GL::AttribState::EnableStencilTest () {        StencilTest(GL_TRUE); }
void GL::AttribState::EnableCullFace    () {           CullFlag(GL_TRUE); }
void GL::AttribState::EnablePolyOfsFill () { PolygonOffsetFill (GL_TRUE); }
void GL::AttribState::EnablePolyOfsPoint() { PolygonOffsetPoint(GL_TRUE); }
void GL::AttribState::EnablePolyOfsLine () { PolygonOffsetLine (GL_TRUE); }

void GL::AttribState::DisableDepthMask   () {          DepthMask(GL_FALSE); }
void GL::AttribState::DisableDepthClamp  () {         DepthClamp(GL_FALSE); }
void GL::AttribState::DisableDepthTest   () {          DepthTest(GL_FALSE); }
void GL::AttribState::DisableAlphaTest   () {          AlphaTest(GL_FALSE); }
void GL::AttribState::DisableBlendMask   () {          BlendMask(GL_FALSE); }
void GL::AttribState::DisableScissorTest () {        ScissorTest(GL_FALSE); }
void GL::AttribState::DisableStencilTest () {        StencilTest(GL_FALSE); }
void GL::AttribState::DisableCullFace    () {           CullFlag(GL_FALSE); }
void GL::AttribState::DisablePolyOfsFill () { PolygonOffsetFill (GL_FALSE); }
void GL::AttribState::DisablePolyOfsPoint() { PolygonOffsetPoint(GL_FALSE); }
void GL::AttribState::DisablePolyOfsLine () { PolygonOffsetLine (GL_FALSE); }


void GL::AttribState::PushAllBits() { PushBits(GL_ALL_ATTRIB_BITS); }
void GL::AttribState::PushBits(uint32_t attribBits) {
	attribBitsStack.Push(attribBits);

	// NOTE:
	//   depth-test is also part of GL_DEPTH_BUFFER_BIT, stencil-test of GL_STENCIL_BUFFER_BIT, etc
	//   this potential overlap means the {alpha, depth, ...}-tests have to be handled individually
	const bool    pushAlphaTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_COLOR_BUFFER_BIT) != 0);
	const bool    pushDepthTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_DEPTH_BUFFER_BIT) != 0);
	const bool  pushScissorTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_SCISSOR_BIT) != 0);
	const bool  pushStencilTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits & GL_STENCIL_BUFFER_BIT) != 0);
	const bool pushCullFaceFlag = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_POLYGON_BIT) != 0);
	const bool  pushPolyOfsFlag = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_POLYGON_BIT) != 0);
	const bool    pushBlendMask = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_COLOR_BUFFER_BIT) != 0);

	if (pushAlphaTest)
		PushAlphaTest();

	#if 0
	// FIXME: just copying the state should be sufficient
	if (pushDepthTest)
		depthTestStack.Push(depthTestStack.Top());
	#else
	if (pushDepthTest)
		PushDepthTest();
	#endif

	if (pushScissorTest)
		PushScissorTest();

	if (pushStencilTest)
		PushStencilTest();

	if (pushCullFaceFlag)
		PushCullFlag();

	if (pushPolyOfsFlag) {
		PushPolygonOffsetFill();
		PushPolygonOffsetPoint();
		PushPolygonOffsetLine();
	}

	if (pushBlendMask)
		PushBlendMask();


	if ((attribBits & GL_POLYGON_BIT) != 0) {
		PushPolygonMode();
		PushPolygonOffset();
		PushCullFace();
		PushFrontFace();
	}

	// TODO: clear-value?
	if ((attribBits & GL_DEPTH_BUFFER_BIT) != 0) {
		PushDepthFunc();
		PushDepthMask();
	}

	// TODO: logic-op, clear-color?
	if ((attribBits & GL_COLOR_BUFFER_BIT) != 0) {
		PushAlphaFunc();
		PushBlendFunc();
		PushBlendColor();
		PushColorMask();
	}

	if ((attribBits & GL_STENCIL_BUFFER_BIT) != 0) {
		PushStencilMask();
		PushStencilFunc();
		PushStencilOper();
	}

	#if 0
	if ((attribBits & GL_TEXTURE_BIT) != 0) {
	}
	#endif

	if ((attribBits & GL_VIEWPORT_BIT) != 0) {
		PushViewPort();
		PushDepthRange();
	}

	if ((attribBits & GL_SCISSOR_BIT) != 0)
		PushScissor();

}

void GL::AttribState::PopBits() {
	const uint32_t attribBits = attribBitsStack.Pop(false);

	const bool    popAlphaTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_COLOR_BUFFER_BIT) != 0);
	const bool    popDepthTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_DEPTH_BUFFER_BIT) != 0);
	const bool  popScissorTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_SCISSOR_BIT) != 0);
	const bool  popStencilTest = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits & GL_STENCIL_BUFFER_BIT) != 0);
	const bool popCullFaceFlag = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_POLYGON_BIT) != 0);
	const bool  popPolyOfsFlag = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &        GL_POLYGON_BIT) != 0);
	const bool    popBlendMask = ((attribBits & GL_ENABLE_BIT) != 0 || (attribBits &   GL_COLOR_BUFFER_BIT) != 0);

	if (popAlphaTest)
		PopAlphaTest();

	if (popDepthTest)
		PopDepthTest();

	if (popScissorTest)
		PopScissorTest();

	if (popStencilTest)
		PopStencilTest();

	if (popCullFaceFlag)
		PopCullFlag();

	if (popPolyOfsFlag) {
		PopPolygonOffsetFill();
		PopPolygonOffsetPoint();
		PopPolygonOffsetLine();
	}

	if (popBlendMask)
		PopBlendMask();


	if ((attribBits & GL_POLYGON_BIT) != 0) {
		PopPolygonMode();
		PopPolygonOffset();
		PopCullFace();
		PopFrontFace();
	}

	if ((attribBits & GL_DEPTH_BUFFER_BIT) != 0) {
		PopDepthFunc();
		PopDepthMask();
	}

	if ((attribBits & GL_COLOR_BUFFER_BIT) != 0) {
		PopAlphaFunc();
		PopBlendFunc();
		PopBlendColor();
		PopColorMask();
	}

	if ((attribBits & GL_STENCIL_BUFFER_BIT) != 0) {
		PopStencilMask();
		PopStencilFunc();
		PopStencilOper();
	}

	#if 0
	if ((attribBits & GL_TEXTURE_BIT) != 0) {
	}
	#endif

	if ((attribBits & GL_VIEWPORT_BIT) != 0) {
		PopViewPort();
		PopDepthRange();
	}

	if ((attribBits & GL_SCISSOR_BIT) != 0)
		PopScissor();

}


void GL::AttribState::PushEnableBit() { PushBits(GL_ENABLE_BIT); }
void GL::AttribState::PushCurrentBit() { PushBits(GL_CURRENT_BIT); }
void GL::AttribState::PushPolygonBit() { PushBits(GL_POLYGON_BIT); }
void GL::AttribState::PushColorBufferBit() { PushBits(GL_COLOR_BUFFER_BIT); }
void GL::AttribState::PushDepthBufferBit() { PushBits(GL_DEPTH_BUFFER_BIT); }
void GL::AttribState::PushStencilBufferBit() { PushBits(GL_STENCIL_BUFFER_BIT); }
void GL::AttribState::PushTextureBit() { PushBits(GL_TEXTURE_BIT); }
void GL::AttribState::PushViewPortBit() { PushBits(GL_VIEWPORT_BIT); }
void GL::AttribState::PushScissorBit() { PushBits(GL_SCISSOR_BIT); }


void GL::AttribState::DepthRange(float zn, float zf) {
	glDepthRangef((depthRangeStack.Top()).zn = zn, (depthRangeStack.Top()).zf = zf);
}
void GL::AttribState::PushDepthRange(float zn, float zf) {
	DepthRange(depthRangeStack.Push({zn, zf}));
}
void GL::AttribState::PopDepthRange() {
	DepthRange(depthRangeStack.Pop(true));
}


void GL::AttribState::DepthClamp(bool enable) {
	glSetStateFuncs[depthClampStack.Top() = enable](GL_DEPTH_CLAMP);
}
void GL::AttribState::PushDepthClamp(bool enable) {
	glSetStateFuncs[depthClampStack.Push(enable)](GL_DEPTH_CLAMP);
}
void GL::AttribState::PopDepthClamp() {
	glSetStateFuncs[depthClampStack.Pop(true)](GL_DEPTH_CLAMP);
}


void GL::AttribState::DepthTest(bool enable) {
	glSetStateFuncs[depthTestStack.Top() = enable](GL_DEPTH_TEST);
}
void GL::AttribState::PushDepthTest(bool enable) {
	glSetStateFuncs[depthTestStack.Push(enable)](GL_DEPTH_TEST);
}
void GL::AttribState::PopDepthTest() {
	glSetStateFuncs[depthTestStack.Pop(true)](GL_DEPTH_TEST);
}


void GL::AttribState::DepthMask(bool enable) {
	glDepthMask(depthMaskStack.Top() = enable);
}
void GL::AttribState::PushDepthMask(bool enable) {
	glDepthMask(depthMaskStack.Push(enable));
}
void GL::AttribState::PopDepthMask() {
	glDepthMask(depthMaskStack.Pop(true));
}


void GL::AttribState::DepthFunc(uint32_t func) {
	glDepthFunc(depthFuncStack.Top() = func);
}
void GL::AttribState::PushDepthFunc(uint32_t func) {
	glDepthFunc(depthFuncStack.Push(func));
}
void GL::AttribState::PopDepthFunc() {
	glDepthFunc(depthFuncStack.Pop(true));
}


void GL::AttribState::AlphaTest(bool enable) {
	return;
	glSetStateFuncs[alphaTestStack.Top() = enable](GL_ALPHA_TEST);
}
void GL::AttribState::PushAlphaTest(bool enable) {
	return;
	glSetStateFuncs[alphaTestStack.Push(enable)](GL_ALPHA_TEST);
}
void GL::AttribState::PopAlphaTest() {
	return;
	glSetStateFuncs[alphaTestStack.Pop(true)](GL_ALPHA_TEST);
}


void GL::AttribState::AlphaFunc(uint32_t func, float rval) {
	return;
	glAlphaFunc(
		(alphaFuncStack.Top()).func = func,
		(alphaFuncStack.Top()).rval = rval
	);
}
void GL::AttribState::PushAlphaFunc(uint32_t func, float rval) {
	return;
	AlphaFunc(alphaFuncStack.Push({func, rval}));
}
void GL::AttribState::PopAlphaFunc() {
	return;
	AlphaFunc(alphaFuncStack.Pop(true));
}


void GL::AttribState::BlendFunc(uint32_t srcFac, uint32_t dstFac) {
	glBlendFunc(
		(blendFuncStack.Top()).srcFac = srcFac,
		(blendFuncStack.Top()).dstFac = dstFac
	);
}
void GL::AttribState::PushBlendFunc(uint32_t srcFac, uint32_t dstFac) {
	BlendFunc(blendFuncStack.Push({srcFac, dstFac}));
}
void GL::AttribState::PopBlendFunc() {
	BlendFunc(blendFuncStack.Pop(true));
}


void GL::AttribState::BlendMask(bool enable) {
	glSetStateFuncs[blendMaskStack.Top() = enable](GL_BLEND);
}
void GL::AttribState::PushBlendMask(bool enable) {
	glSetStateFuncs[blendMaskStack.Push(enable)](GL_BLEND);
}
void GL::AttribState::PopBlendMask() {
	glSetStateFuncs[blendMaskStack.Pop(true)](GL_BLEND);
}


void GL::AttribState::ScissorTest(bool enable) {
	glSetStateFuncs[scissorTestStack.Top() = enable](GL_SCISSOR_TEST);
}
void GL::AttribState::PushScissorTest(bool enable) {
	glSetStateFuncs[scissorTestStack.Push(enable)](GL_SCISSOR_TEST);
}
void GL::AttribState::PopScissorTest() {
	glSetStateFuncs[scissorTestStack.Pop(true)](GL_SCISSOR_TEST);
}


void GL::AttribState::StencilTest(bool enable) {
	glSetStateFuncs[stencilTestStack.Top() = enable](GL_STENCIL_TEST);
}
void GL::AttribState::PushStencilTest(bool enable) {
	glSetStateFuncs[stencilTestStack.Push(enable)](GL_STENCIL_TEST);
}
void GL::AttribState::PopStencilTest() {
	glSetStateFuncs[stencilTestStack.Pop(true)](GL_STENCIL_TEST);
}


void GL::AttribState::StencilMask(uint32_t mask) {
	glStencilMask(stencilMaskStack.Top() = mask);
}
void GL::AttribState::PushStencilMask(uint32_t mask) {
	glStencilMask(stencilMaskStack.Push(mask));
}
void GL::AttribState::PopStencilMask() {
	glStencilMask(stencilMaskStack.Pop(true));
}

void GL::AttribState::StencilFunc(uint32_t func, int32_t rval, uint32_t mask) {
	glStencilFunc(
		(stencilFuncStack.Top()).func = func,
		(stencilFuncStack.Top()).rval = rval,
		(stencilFuncStack.Top()).mask = mask
	);
}
void GL::AttribState::PushStencilFunc(uint32_t func, int32_t rval, uint32_t mask) {
	StencilFunc(stencilFuncStack.Push({func, rval, mask}));
}
void GL::AttribState::PopStencilFunc() {
	StencilFunc(stencilFuncStack.Pop(true));
}

void GL::AttribState::StencilOper(uint32_t sfail, uint32_t zfail, uint32_t zpass) {
	glStencilOp(
		(stencilOperStack.Top()).sfail = sfail,
		(stencilOperStack.Top()).zfail = zfail,
		(stencilOperStack.Top()).zpass = zpass
	);
}
void GL::AttribState::PushStencilOper(uint32_t sfail, uint32_t zfail, uint32_t zpass) {
	StencilOper(stencilOperStack.Push({sfail, zfail, zpass}));
}
void GL::AttribState::PopStencilOper() {
	StencilOper(stencilOperStack.Top());
}


void GL::AttribState::PolygonMode(uint32_t side, uint32_t mode) {
	glPolygonMode(
		(polyModeStack.Top()).side = side,
		(polyModeStack.Top()).mode = mode
	);
}
void GL::AttribState::PushPolygonMode(uint32_t side, uint32_t mode) {
	PolygonMode(polyModeStack.Push({side, mode}));
}
void GL::AttribState::PopPolygonMode() {
	PolygonMode(polyModeStack.Pop(true));
}


void GL::AttribState::PolygonOffset(float factor, float units) {
	glPolygonOffset(
		(pofsCtrlStack.Top()).factor = factor,
		(pofsCtrlStack.Top()).units  = units
	);
}
void GL::AttribState::PushPolygonOffset(float factor, float units) {
	PolygonOffset(pofsCtrlStack.Push({factor, units}));
}
void GL::AttribState::PopPolygonOffset() {
	PolygonOffset(pofsCtrlStack.Pop(true));
}



void GL::AttribState::PolygonOffsetFill(bool enable) {
	glSetStateFuncs[pofsFlagStack[0].Top() = enable](GL_POLYGON_OFFSET_FILL);
}
void GL::AttribState::PushPolygonOffsetFill(bool enable) {
	glSetStateFuncs[pofsFlagStack[0].Push(enable)](GL_POLYGON_OFFSET_FILL);
}
void GL::AttribState::PopPolygonOffsetFill() {
	glSetStateFuncs[pofsFlagStack[0].Pop(true)](GL_POLYGON_OFFSET_FILL);
}

void GL::AttribState::PolygonOffsetPoint(bool enable) {
	glSetStateFuncs[pofsFlagStack[1].Top() = enable](GL_POLYGON_OFFSET_POINT);
}
void GL::AttribState::PushPolygonOffsetPoint(bool enable) {
	glSetStateFuncs[pofsFlagStack[1].Push(enable)](GL_POLYGON_OFFSET_POINT);
}
void GL::AttribState::PopPolygonOffsetPoint() {
	glSetStateFuncs[pofsFlagStack[1].Pop(true)](GL_POLYGON_OFFSET_POINT);
}

void GL::AttribState::PolygonOffsetLine(bool enable) {
	glSetStateFuncs[pofsFlagStack[2].Top() = enable](GL_POLYGON_OFFSET_LINE);
}
void GL::AttribState::PushPolygonOffsetLine(bool enable) {
	glSetStateFuncs[pofsFlagStack[2].Push(enable)](GL_POLYGON_OFFSET_LINE);
}
void GL::AttribState::PopPolygonOffsetLine() {
	glSetStateFuncs[pofsFlagStack[2].Pop(true)](GL_POLYGON_OFFSET_LINE);
}



void GL::AttribState::ViewPort(int32_t x, int32_t y, int32_t w, int32_t h) {
	glViewport(
		(viewportStack.Top()).x = x,
		(viewportStack.Top()).y = y,
		(viewportStack.Top()).w = w,
		(viewportStack.Top()).h = h
	);
}
void GL::AttribState::PushViewPort(int32_t x, int32_t y, int32_t w, int32_t h) {
	ViewPort(viewportStack.Push({x, y, w, h}));
}
void GL::AttribState::PopViewPort() {
	ViewPort(viewportStack.Pop(true));
}



void GL::AttribState::Scissor(int32_t x, int32_t y, int32_t w, int32_t h) {
	glScissor(
		(scissorStack.Top()).x = x,
		(scissorStack.Top()).y = y,
		(scissorStack.Top()).w = w,
		(scissorStack.Top()).h = h
	);
}
void GL::AttribState::PushScissor(int32_t x, int32_t y, int32_t w, int32_t h) {
	Scissor(scissorStack.Push({x, y, w, h}));
}
void GL::AttribState::PopScissor() {
	Scissor(scissorStack.Pop(true));
}


void GL::AttribState::FrontFace(uint32_t face) {
	glFrontFace(frontFaceStack.Top() = face);
}
void GL::AttribState::PushFrontFace(uint32_t face) {
	glFrontFace(frontFaceStack.Push(face));
}
void GL::AttribState::PopFrontFace() {
	glFrontFace(frontFaceStack.Pop(true));
}


void GL::AttribState::CullFace(uint32_t face) {
	glCullFace(cullFaceStack.Top() = face);
}
void GL::AttribState::PushCullFace(uint32_t face) {
	glCullFace(cullFaceStack.Push(face));
}
void GL::AttribState::PopCullFace() {
	glCullFace(cullFaceStack.Pop(true));
}


void GL::AttribState::CullFlag(bool enable) {
	glSetStateFuncs[cullFlagStack.Top() = enable](GL_CULL_FACE);
}
void GL::AttribState::PushCullFlag(bool enable) {
	glSetStateFuncs[cullFlagStack.Push(enable)](GL_CULL_FACE);
}
void GL::AttribState::PopCullFlag() {
	glSetStateFuncs[cullFlagStack.Pop(true)](GL_CULL_FACE);
}


void GL::AttribState::BlendColor(float r, float g, float b, float a) {
	glBlendColor(
		(blendColorStack.Top()).r = r,
		(blendColorStack.Top()).g = g,
		(blendColorStack.Top()).b = b,
		(blendColorStack.Top()).a = a
	);
}
void GL::AttribState::PushBlendColor(float r, float g, float b, float a) {
	BlendColor(blendColorStack.Push({r, g, b, a}));
}
void GL::AttribState::PopBlendColor() {
	BlendColor(blendColorStack.Pop(true));
}


void GL::AttribState::ColorMask(bool r, bool g, bool b, bool a) {
	glColorMask(
		(colorMaskStack.Top()).r = r,
		(colorMaskStack.Top()).g = g,
		(colorMaskStack.Top()).b = b,
		(colorMaskStack.Top()).a = a
	);
}
void GL::AttribState::PushColorMask(bool r, bool g, bool b, bool a) {
	ColorMask(colorMaskStack.Push({r, g, b, a}));
}
void GL::AttribState::PopColorMask() {
	ColorMask(colorMaskStack.Pop(true));
}


void GL::AttribState::Clear(uint32_t bits) {
	glClear(bits);
}
void GL::AttribState::ClearAccum(float r, float g, float b, float a) { glClearAccum(r, g, b, a); }
void GL::AttribState::ClearColor(float r, float g, float b, float a) {
	const float gammaExp = globalRendering->gammaExponent;

	// easier to apply correction here than at every caller
	r = std::pow(r, gammaExp);
	g = std::pow(g, gammaExp);
	b = std::pow(b, gammaExp);

	glClearColor(r, g, b, a);
}
void GL::AttribState::ClearDepth(float depth) {
	glClearDepth(depth);
}
void GL::AttribState::ClearStencil(uint32_t rval) {
	glClearStencil(rval);
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

