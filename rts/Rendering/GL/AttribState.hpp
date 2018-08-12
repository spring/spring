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

		void Push(uint32_t attribBits);
		void Pop();

		// depth-clamp (GLboolean)
		void DepthClamp(bool enable);
		void PushDepthClamp(bool enable);
		void PopDepthClamp();

		// depth-test (GLboolean)
		void DepthTest(bool enable);
		void PushDepthTest(bool enable);
		void PopDepthTest();

		// depth-mask (GLboolean)
		void DepthMask(bool enable);
		void PushDepthMask(bool enable);
		void PopDepthMask();

		// depth-func (GLenum)
		void DepthFunc(uint32_t dtFunc);
		void PushDepthFunc(uint32_t dtFunc);
		void PopDepthFunc();

		// alpha-test (GLboolean)
		void AlphaTest(bool enable);
		void PushAlphaTest(bool enable);
		void PopAlphaTest();

		// alpha-func (GLenum, GLfloat)
		void AlphaFunc(uint32_t atFunc, float refVal);
		void PushAlphaFunc(uint32_t atFunc, float refVal);
		void PopAlphaFunc();

		// blend-func (GLenum, GLenum)
		void BlendFunc(uint32_t srcFac, uint32_t dstFac);
		void PushBlendFunc(uint32_t srcFac, uint32_t dstFac);
		void PopBlendFunc();

		// blending flag (GLboolean)
		void BlendFlag(bool enable);
		void PushBlendFlag(bool enable);
		void PopBlendFlag();

		// stencil-test (GLboolean)
		void StencilTest(bool enable);
		void PushStencilTest(bool enable);
		void PopStencilTest();

		// scissor-test (GLboolean)
		void ScissorTest(bool enable);
		void PushScissorTest(bool enable);
		void PopScissorTest();

		// polygon-mode (GLenum, GLenum)
		void PolygonMode(uint32_t side, uint32_t mode);
		void PushPolygonMode(uint32_t side, uint32_t mode);
		void PopPolygonMode();

		// polygon-offset params (GLfloat, GLfloat)
		void PolygonOffset(float factor, float units);
		void PushPolygonOffset(float factor, float units);
		void PopPolygonOffset();

		// polygon-offset {fill,point,line} flag (GLenum, GLboolean)
		#if 0
		void PolygonOffsetType(uint32_t type, bool enable);
		void PushPolygonOffsetType(uint32_t type, bool enable);
		void PopPolygonOffsetType(uint32_t type);
		#else
		void PolygonOffsetFill(bool enable);
		void PushPolygonOffsetFill(bool enable);
		void PopPolygonOffsetFill();

		void PolygonOffsetPoint(bool enable);
		void PushPolygonOffsetPoint(bool enable);
		void PopPolygonOffsetPoint();

		void PolygonOffsetLine(bool enable);
		void PushPolygonOffsetLine(bool enable);
		void PopPolygonOffsetLine();
		#endif

		// front-face (GLuint)
		void FrontFace(uint32_t face);
		void PushFrontFace(uint32_t face);
		void PopFrontFace();

		// cull-face (GLenum)
		void CullFace(uint32_t face);
		void PushCullFace(uint32_t face);
		void PopCullFace();

		// culling flag (GLboolean)
		void CullFlag(bool enable);
		void PushCullFlag(bool enable);
		void PopCullFlag();

		// color-mask (GLboolean)
		void ColorMask(bool r, bool g, bool b, bool a);
		void PushColorMask(bool r, bool g, bool b, bool a);
		void PopColorMask();

	private:
		struct AlphaFuncState {
			uint32_t atFunc;
			float refVal;
		};
		struct BlendFuncState {
			uint32_t srcFac;
			uint32_t dstFac;
		};
		struct ColorMaskState {
			uint32_t r;
			uint32_t g;
			uint32_t b;
			uint32_t a;
		};
		struct PolyModeState {
			uint32_t side;
			uint32_t mode;
		};
		struct PolyOffsetState {
			float factor;
			float units;
		};

		void AlphaFunc(const AlphaFuncState& v) { AlphaFunc(v.atFunc, v.refVal); }
		void BlendFunc(const BlendFuncState& v) { BlendFunc(v.srcFac, v.dstFac); }
		void ColorMask(const ColorMaskState& v) { ColorMask(v.r, v.g, v.b, v.a); }
		void PolygonMode(const PolyModeState& v) { PolygonMode(v.side, v.mode); }
		void PolygonOffset(const PolyOffsetState& v) { PolygonOffset(v.factor, v.units); }

		void PushAlphaFunc(const AlphaFuncState& v) { PushAlphaFunc(v.atFunc, v.refVal); }
		void PushBlendFunc(const BlendFuncState& v) { PushBlendFunc(v.srcFac, v.dstFac); }
		void PushColorMask(const ColorMaskState& v) { PushColorMask(v.r, v.g, v.b, v.a); }
		void PushPolygonMode(const PolyModeState& v) { PushPolygonMode(v.side, v.mode); }
		void PushPolygonOffset(const PolyOffsetState& v) { PushPolygonOffset(v.factor, v.units); }

	private:
		std::array<uint32_t       , 64>  attribBitsStack;
		std::array<bool           , 64>  depthClampStack;
		std::array<bool           , 64>   depthTestStack;
		std::array<bool           , 64>   depthMaskStack;
		std::array<uint32_t       , 64>   depthFuncStack;
		std::array<bool           , 64>   alphaTestStack;
		std::array<AlphaFuncState , 64>   alphaFuncStack;
		std::array<BlendFuncState , 64>   blendFuncStack;
		std::array<bool           , 64>   blendFlagStack;
		std::array<bool           , 64> stencilTestStack;
		std::array<bool           , 64> scissorTestStack;
		std::array<PolyModeState  , 64>    polyModeStack;
		std::array<PolyOffsetState, 64>    pofsCtrlStack;
		std::array<bool           , 64>    pofsFlagStack[1];
		std::array<uint32_t       , 64>   frontFaceStack;
		std::array<uint32_t       , 64>    cullFaceStack;
		std::array<bool           , 64>    cullFlagStack;
		std::array<ColorMaskState , 64>   colorMaskStack;


		uint8_t  attribBitsStackSize    = {0};
		uint8_t  depthClampStackSize    = {0};
		uint8_t   depthTestStackSize    = {0};
		uint8_t   depthMaskStackSize    = {0};
		uint8_t   depthFuncStackSize    = {0};
		uint8_t   alphaTestStackSize    = {0};
		uint8_t   alphaFuncStackSize    = {0};
		uint8_t   blendFuncStackSize    = {0};
		uint8_t   blendFlagStackSize    = {0};
		uint8_t stencilTestStackSize    = {0};
		uint8_t scissorTestStackSize    = {0};
		uint8_t    polyModeStackSize    = {0};
		uint8_t    pofsCtrlStackSize    = {0};
		uint8_t    pofsFlagStackSize[1] = {0};
		uint8_t   frontFaceStackSize    = {0};
		uint8_t    cullFaceStackSize    = {0};
		uint8_t    cullFlagStackSize    = {0};
		uint8_t   colorMaskStackSize    = {0};
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

