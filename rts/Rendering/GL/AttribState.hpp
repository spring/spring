/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_ATTRIB_STATE_H
#define GL_ATTRIB_STATE_H

#include <array>

namespace GL {
	struct AttribState {
	public:
		AttribState();

		void Init();

		void Enable(uint32_t attrib); // GLenum
		void Disable(uint32_t attrib);

		void EnableDepthMask  ();
		void EnableDepthClamp ();
		void EnableDepthTest  ();
		void EnableAlphaTest  ();
		void EnableBlendMask  ();
		void EnableScissorTest();
		void EnableStencilTest();
		void EnableCullFace   ();
		void EnablePolyOfsFill();
		void EnablePolyOfsPoint();
		void EnablePolyOfsLine();

		void DisableDepthMask  ();
		void DisableDepthClamp ();
		void DisableDepthTest  ();
		void DisableAlphaTest  ();
		void DisableBlendMask  ();
		void DisableScissorTest();
		void DisableStencilTest();
		void DisableCullFace   ();
		void DisablePolyOfsFill();
		void DisablePolyOfsPoint();
		void DisablePolyOfsLine();


		void PushAllBits();
		void PushBits(uint32_t attribBits);
		void PopBits();

		void PushEnableBit();
		void PushCurrentBit();
		void PushPolygonBit();
		void PushColorBufferBit();
		void PushDepthBufferBit();
		void PushStencilBufferBit();
		void PushTextureBit();
		void PushViewPortBit();
		void PushScissorBit();


		void DepthRange(float zn, float zf);
		void PushDepthRange(float zn, float zf);
		void PushDepthRange() { PushDepthRange(depthRangeStack.Top()); }
		void PopDepthRange();

		// depth-clamp (GLboolean)
		void DepthClamp(bool enable);
		void PushDepthClamp(bool enable);
		void PushDepthClamp() { PushDepthClamp(depthClampStack.Top()); }
		void PopDepthClamp();

		// depth-test (GLboolean)
		void DepthTest(bool enable);
		void PushDepthTest(bool enable);
		void PushDepthTest() { PushDepthTest(depthTestStack.Top()); }
		void PopDepthTest();

		// depth-mask (GLboolean)
		void DepthMask(bool enable);
		void PushDepthMask(bool enable);
		void PushDepthMask() { PushDepthMask(depthMaskStack.Top()); }
		void PopDepthMask();

		// depth-func (GLenum)
		void DepthFunc(uint32_t func);
		void PushDepthFunc(uint32_t func);
		void PushDepthFunc() { PushDepthFunc(depthFuncStack.Top()); }
		void PopDepthFunc();

		// alpha-test (GLboolean)
		void AlphaTest(bool enable);
		void PushAlphaTest(bool enable);
		void PushAlphaTest() { PushAlphaTest(alphaTestStack.Top()); }
		void PopAlphaTest();

		// alpha-func (GLenum, GLfloat)
		void AlphaFunc(uint32_t func, float rval);
		void PushAlphaFunc(uint32_t func, float rval);
		void PushAlphaFunc() { PushAlphaFunc(alphaFuncStack.Top()); }
		void PopAlphaFunc();

		// blend-func (GLenum, GLenum)
		void BlendFunc(uint32_t srcFac, uint32_t dstFac);
		void PushBlendFunc(uint32_t srcFac, uint32_t dstFac);
		void PushBlendFunc() { PushBlendFunc(blendFuncStack.Top()); }
		void PopBlendFunc();

		// blending flag (GLboolean)
		void BlendMask(bool enable);
		void PushBlendMask(bool enable);
		void PushBlendMask() { PushBlendMask(blendMaskStack.Top()); }
		void PopBlendMask();

		// scissor-test (GLboolean)
		void ScissorTest(bool enable);
		void PushScissorTest(bool enable);
		void PushScissorTest() { PushScissorTest(scissorTestStack.Top()); }
		void PopScissorTest();

		// stencil-test (GLboolean)
		void StencilTest(bool enable);
		void PushStencilTest(bool enable);
		void PushStencilTest() { PushStencilTest(stencilTestStack.Top()); }
		void PopStencilTest();

		// stencil-mask (GLuint)
		void StencilMask(uint32_t mask);
		void PushStencilMask(uint32_t mask);
		void PushStencilMask() { PushStencilMask(stencilMaskStack.Top()); }
		void PopStencilMask();

		// stencil-func (GLenum, GLint, GLuint)
		void StencilFunc(uint32_t func, int32_t rval, uint32_t mask);
		void PushStencilFunc(uint32_t func, int32_t rval, uint32_t mask);
		void PushStencilFunc() { PushStencilFunc(stencilFuncStack.Top()); }
		void PopStencilFunc();

		// stencil-op (GLenum, GLenum, GLenum)
		void StencilOper(uint32_t sfail, uint32_t zfail, uint32_t zpass);
		void PushStencilOper(uint32_t sfail, uint32_t zfail, uint32_t zpass);
		void PushStencilOper() { PushStencilOper(stencilOperStack.Top()); }
		void PopStencilOper();

		// polygon-mode (GLenum, GLenum)
		void PolygonMode(uint32_t side, uint32_t mode);
		void PushPolygonMode(uint32_t side, uint32_t mode);
		void PushPolygonMode() { PushPolygonMode(polyModeStack.Top()); }
		void PopPolygonMode();

		// polygon-offset params (GLfloat, GLfloat)
		void PolygonOffset(float factor, float units);
		void PushPolygonOffset(float factor, float units);
		void PushPolygonOffset() { PushPolygonOffset(pofsCtrlStack.Top()); }
		void PopPolygonOffset();

		// polygon-offset {fill,point,line} flag (GLenum, GLboolean)
		#if 0
		void PolygonOffsetType(uint32_t type, bool enable);
		void PushPolygonOffsetType(uint32_t type, bool enable);
		void PopPolygonOffsetType(uint32_t type);
		#else
		void PolygonOffsetFill(bool enable);
		void PushPolygonOffsetFill(bool enable);
		void PushPolygonOffsetFill() { PushPolygonOffsetFill(pofsFlagStack[0].Top()); }
		void PopPolygonOffsetFill();

		void PolygonOffsetPoint(bool enable);
		void PushPolygonOffsetPoint(bool enable);
		void PushPolygonOffsetPoint() { PushPolygonOffsetPoint(pofsFlagStack[1].Top()); }
		void PopPolygonOffsetPoint();

		void PolygonOffsetLine(bool enable);
		void PushPolygonOffsetLine(bool enable);
		void PushPolygonOffsetLine() { PushPolygonOffsetLine(pofsFlagStack[2].Top()); }
		void PopPolygonOffsetLine();
		#endif

		// viewport (GLint)
		void ViewPort(int32_t x, int32_t y, int32_t w, int32_t h);
		void PushViewPort(int32_t x, int32_t y, int32_t w, int32_t h);
		void PushViewPort() { PushViewPort(viewportStack.Top()); }
		void PopViewPort();

		// scissor (GLint)
		void Scissor(int32_t x, int32_t y, int32_t w, int32_t h);
		void PushScissor(int32_t x, int32_t y, int32_t w, int32_t h);
		void PushScissor() { PushScissor(scissorStack.Top()); }
		void PopScissor();

		// front-face (GLuint)
		void FrontFace(uint32_t face);
		void PushFrontFace(uint32_t face);
		void PushFrontFace() { PushFrontFace(frontFaceStack.Top()); }
		void PopFrontFace();

		// cull-face (GLenum)
		void CullFace(uint32_t face);
		void PushCullFace(uint32_t face);
		void PushCullFace() { PushCullFace(cullFaceStack.Top()); }
		void PopCullFace();

		// culling flag (GLboolean)
		void CullFlag(bool enable);
		void PushCullFlag(bool enable);
		void PushCullFlag() { PushCullFlag(cullFlagStack.Top()); }
		void PopCullFlag();

		// blend-color (GLfloat)
		void BlendColor(float r, float g, float b, float a);
		void PushBlendColor(float r, float g, float b, float a);
		void PushBlendColor() { PushBlendColor(blendColorStack.Top()); }
		void PopBlendColor();

		// color-mask (GLboolean)
		void ColorMask(bool r, bool g, bool b, bool a);
		void PushColorMask(bool r, bool g, bool b, bool a);
		void PushColorMask() { PushColorMask(colorMaskStack.Top()); }
		void PopColorMask();

		// uncaptured state
		void Clear(uint32_t bits);
		void ClearAccum(float r, float g, float b, float a);
		void ClearColor(float r, float g, float b, float a);
		void ClearColor(const float* c) { ClearColor(c[0], c[1], c[2], c[3]); }
		void ClearDepth(float depth);
		void ClearStencil(uint32_t rval);

	private:
		struct DepthRangeState {
			float zn;
			float zf;
		};
		struct AlphaFuncState {
			uint32_t func;
			float rval;
		};
		struct BlendFuncState {
			uint32_t srcFac;
			uint32_t dstFac;
		};
		struct BlendColorState {
			float r;
			float g;
			float b;
			float a;
		};
		struct ColorMaskState {
			uint32_t r;
			uint32_t g;
			uint32_t b;
			uint32_t a;
		};
		struct StencilFuncState {
			uint32_t func;
			 int32_t rval;
			uint32_t mask;
		};
		struct StencilOperState {
			uint32_t sfail;
			uint32_t zfail;
			uint32_t zpass;
		};
		struct PolyModeState {
			uint32_t side;
			uint32_t mode;
		};
		struct PolyOffsetState {
			float factor;
			float units;
		};
		struct ViewPortState {
			int32_t x;
			int32_t y;
			int32_t w;
			int32_t h;
		};
		struct ScissorState {
			int32_t x;
			int32_t y;
			int32_t w;
			int32_t h;
		};

		void DepthRange(const DepthRangeState& v) { DepthRange(v.zn, v.zf); }
		void AlphaFunc(const AlphaFuncState& v) { AlphaFunc(v.func, v.rval); }
		void BlendFunc(const BlendFuncState& v) { BlendFunc(v.srcFac, v.dstFac); }
		void BlendColor(const BlendColorState& v) { BlendColor(v.r, v.g, v.b, v.a); }
		void ColorMask(const ColorMaskState& v) { ColorMask(v.r, v.g, v.b, v.a); }
		void StencilFunc(const StencilFuncState& v) { StencilFunc(v.func, v.rval, v.mask); }
		void StencilOper(const StencilOperState& v) { StencilOper(v.sfail, v.zfail, v.zpass); }
		void PolygonMode(const PolyModeState& v) { PolygonMode(v.side, v.mode); }
		void PolygonOffset(const PolyOffsetState& v) { PolygonOffset(v.factor, v.units); }
		void ViewPort(const ViewPortState& v) { ViewPort(v.x, v.y, v.w, v.h); }
		void Scissor(const ScissorState& v) { Scissor(v.x, v.y, v.w, v.h); }

		void PushDepthRange(const DepthRangeState& v) { PushDepthRange(v.zn, v.zf); }
		void PushAlphaFunc(const AlphaFuncState& v) { PushAlphaFunc(v.func, v.rval); }
		void PushBlendFunc(const BlendFuncState& v) { PushBlendFunc(v.srcFac, v.dstFac); }
		void PushBlendColor(const BlendColorState& v) { PushBlendColor(v.r, v.g, v.b, v.a); }
		void PushColorMask(const ColorMaskState& v) { PushColorMask(v.r, v.g, v.b, v.a); }
		void PushStencilFunc(const StencilFuncState& v) { PushStencilFunc(v.func, v.rval, v.mask); }
		void PushStencilOper(const StencilOperState& v) { PushStencilOper(v.sfail, v.zfail, v.zpass); }
		void PushPolygonMode(const PolyModeState& v) { PushPolygonMode(v.side, v.mode); }
		void PushPolygonOffset(const PolyOffsetState& v) { PushPolygonOffset(v.factor, v.units); }
		void PushViewPort(const ViewPortState& v) { PushViewPort(v.x, v.y, v.w, v.h); }
		void PushScissor(const ScissorState& v) { PushScissor(v.x, v.y, v.w, v.h); }

	private:
		template<typename T, size_t S> struct ArrayStack {
		static_assert((S & (S - 1)) == 0, "stack size must be a power of two");
		public:
			void Fill(const T& v) { stack.fill(v); }

			const T& Push(const T& v) {
				// return the top element post-push
				// assert(size < S);

				size += (size < S);
				// size &= (S - 1);

				return (stack[size - 1] = v);
			}
			const T& Pop(bool post) {
				// return the top element pre- or post-pop
				// assert(size > post);

				post &= (size > 1);
				size -= (size > 0);
				// size &= (S - 1);

				return stack[size - post];
			}

			#if 1
			const T& Top() const { return stack[(size - 1) & (S - 1)]; }
			      T& Top()       { return stack[(size - 1) & (S - 1)]; }
			#else
			const T& Top() const { return stack[size - 1]; }
			      T& Top()       { return stack[size - 1]; }
			#endif

		private:
			std::array<T, S> stack;

			uint8_t size = 0;
		};

		ArrayStack<uint32_t        , 64>  attribBitsStack;
		ArrayStack<DepthRangeState , 64>  depthRangeStack;
		ArrayStack<bool            , 64>  depthClampStack;
		ArrayStack<bool            , 64>   depthTestStack;
		ArrayStack<bool            , 64>   depthMaskStack;
		ArrayStack<uint32_t        , 64>   depthFuncStack;
		ArrayStack<bool            , 64>   alphaTestStack;
		ArrayStack<AlphaFuncState  , 64>   alphaFuncStack;
		ArrayStack<BlendFuncState  , 64>   blendFuncStack;
		ArrayStack<bool            , 64>   blendMaskStack;
		ArrayStack<bool            , 64> scissorTestStack;
		ArrayStack<bool            , 64> stencilTestStack;
		ArrayStack<uint32_t        , 64> stencilMaskStack;
		ArrayStack<StencilFuncState, 64> stencilFuncStack;
		ArrayStack<StencilOperState, 64> stencilOperStack;
		ArrayStack<PolyModeState   , 64>    polyModeStack;
		ArrayStack<PolyOffsetState , 64>    pofsCtrlStack;
		ArrayStack<bool            , 64>    pofsFlagStack[3];
		ArrayStack<uint32_t        , 64>   frontFaceStack;
		ArrayStack<uint32_t        , 64>    cullFaceStack;
		ArrayStack<bool            , 64>    cullFlagStack;
		ArrayStack<BlendColorState , 64>  blendColorStack;
		ArrayStack<ColorMaskState  , 64>   colorMaskStack;
		ArrayStack<ViewPortState   , 64>    viewportStack;
		ArrayStack< ScissorState   , 64>     scissorStack;
	};


	struct ScopedDepthFunc {
	public:
		ScopedDepthFunc(uint32_t func);
		~ScopedDepthFunc();

	private:
		uint32_t prevFunc = 0;
	};


	struct ScopedCullFace {
	public:
		ScopedCullFace(uint32_t face);
		~ScopedCullFace();

	private:
		uint32_t prevFace = 0;
	};


	AttribState* SetAttribStatePointer(bool mainThread);
	AttribState* GetAttribStatePointer();
}

#define glAttribStatePtr GL::GetAttribStatePointer()
#endif

