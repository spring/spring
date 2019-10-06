/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_RENDERDATABUFFER_HDR
#define GL_RENDERDATABUFFER_HDR

#include <array>
#include <cstring> // memcpy

#include "VAO.h"
#include "VBO.h"
#include "RenderDataBufferFwd.hpp"
#include "Rendering/Shaders/Shader.h"

#define SYNC_RENDER_BUFFERS 1

namespace GL {
	constexpr static int NUM_RENDER_BUFFERS = 3;


	static_assert(sizeof(VA_TYPE_0) == sizeof(float3), "");
	static_assert(sizeof(VA_TYPE_0) == (sizeof(float) * 3), "");
	const static std::array<Shader::ShaderInput, 1> VA_TYPE_0_ATTRS = {{
		{0,  3, GL_FLOAT,  (sizeof(float) * 3),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
	}};


	#if 0
	static_assert((VA_SIZE_C + 3) == 7, "");
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_C_ATTRS = {{
		{0,  3, GL_FLOAT,  7 * sizeof(float),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  4, GL_FLOAT,  7 * sizeof(float),  "a_color_rgba", VA_TYPE_OFFSET(float, 3)},
	}};
	#else
	static_assert(sizeof(VA_TYPE_C) == (sizeof(float) * 3 + sizeof(uint32_t)), "");
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_C_ATTRS = {{
		{0,  3, GL_FLOAT        ,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_color_rgba", VA_TYPE_OFFSET(float, 3)},
	}};

	// flat-shaded variant
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_FC_ATTRS = {{
		{0,  3, GL_FLOAT        ,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_vertex_xyz"     , VA_TYPE_OFFSET(float, 0)},
		{1,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_color_rgba_flat", VA_TYPE_OFFSET(float, 3)},
	}};
	#endif


	static_assert(sizeof(VA_TYPE_T) == (sizeof(float3) + sizeof(float) * 2), "");
	static_assert(sizeof(VA_TYPE_T) == (sizeof(float) * 5), ""); // 5 = 3 + 2
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_T_ATTRS = {{
		{0,  3, GL_FLOAT,  (sizeof(float) * 5),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 5),  "a_texcoor_st", VA_TYPE_OFFSET(float, 3)},
	}};


	static_assert(sizeof(VA_TYPE_T4) == (sizeof(float3) + sizeof(float4)), "");
	static_assert(sizeof(VA_TYPE_T4) == (sizeof(float) * 7), ""); // 7 = 3 + 4
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_T4_ATTRS = {{
		{0,  3, GL_FLOAT,  (sizeof(float) * 7),  "a_vertex_xyz"  , VA_TYPE_OFFSET(float, 0)},
		{1,  4, GL_FLOAT,  (sizeof(float) * 7),  "a_texcoor_stuv", VA_TYPE_OFFSET(float, 3)},
	}};


	static_assert(sizeof(VA_TYPE_TN) == (sizeof(float3) * 2 + sizeof(float) * 2), "");
	static_assert(sizeof(VA_TYPE_TN) == (sizeof(float) * 8), ""); // 8 = 3 + 2 + 3
	const static std::array<Shader::ShaderInput, 3> VA_TYPE_TN_ATTRS = {{
		{0,  3, GL_FLOAT,  (sizeof(float) * 8),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_texcoor_st", VA_TYPE_OFFSET(float, 3)},
		{2,  3, GL_FLOAT,  (sizeof(float) * 8),  "a_normal_xyz", VA_TYPE_OFFSET(float, 5)},
	}};


	#if 0
	static_assert((VA_SIZE_TC + 3) == 9, "");
	const static std::array<Shader::ShaderInput, 3> VA_TYPE_TC_ATTRS = {{
		{0,  3, GL_FLOAT,  (sizeof(float) * 9),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 9),  "a_texcoor_st", VA_TYPE_OFFSET(float, 3)},
		{2,  4, GL_FLOAT,  (sizeof(float) * 9),  "a_color_rgba", VA_TYPE_OFFSET(float, 5)},
	}};
	#else
	static_assert(sizeof(VA_TYPE_TC) == (sizeof(float) * (3 + 2) + sizeof(uint32_t)), "");
	const static std::array<Shader::ShaderInput, 3> VA_TYPE_TC_ATTRS = {{
		{0,  3, GL_FLOAT        ,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_vertex_xyz", VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT        ,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_texcoor_st", VA_TYPE_OFFSET(float, 3)},
		{2,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_color_rgba", VA_TYPE_OFFSET(float, 5)},
	}};
	#endif


	static_assert(sizeof(VA_TYPE_2d0) == (sizeof(float) * 2), "");
	const static std::array<Shader::ShaderInput, 1> VA_TYPE_2D0_ATTRS = {{
		{0,  2, GL_FLOAT,  (sizeof(float) * 2),  "a_vertex_xy", VA_TYPE_OFFSET(float, 0)},
	}};


	static_assert(sizeof(VA_TYPE_2dT) == (sizeof(float) * 4), ""); // 4 = 2 + 2
	const static std::array<Shader::ShaderInput, 2> VA_TYPE_2DT_ATTRS = {{
		{0,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_vertex_xy" , VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_texcoor_st", VA_TYPE_OFFSET(float, 2)},
	}};


	#if 0
	static_assert((VA_SIZE_2DTC + 3) == 8, "");
	const static std::array<Shader::ShaderInput, 3> VA_TYPE_2DTC_ATTRS = {{
		{0,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_vertex_xy" , VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_texcoor_st", VA_TYPE_OFFSET(float, 2)},
		{2,  4, GL_FLOAT,  (sizeof(float) * 8),  "a_color_rgba", VA_TYPE_OFFSET(float, 4)},
	}};
	#else
	static_assert(sizeof(VA_TYPE_2dTC) == (sizeof(float) * (2 + 2) + sizeof(uint32_t)), "");
	const static std::array<Shader::ShaderInput, 3> VA_TYPE_2DTC_ATTRS = {{
		{0,  2, GL_FLOAT        ,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_vertex_xy" , VA_TYPE_OFFSET(float, 0)},
		{1,  2, GL_FLOAT        ,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_texcoor_st", VA_TYPE_OFFSET(float, 2)},
		{2,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_color_rgba", VA_TYPE_OFFSET(float, 4)},
	}};
	#endif


	static_assert(sizeof(VA_TYPE_L) == (sizeof(float) * (4 + 3 + 4) + sizeof(uint32_t) * (1 + 1)), "");
	const static std::array<Shader::ShaderInput, 5> VA_TYPE_L_ATTRS = {{
		{0,  4, GL_FLOAT        ,  (sizeof(float) * 11 + sizeof(uint8_t) * 8),  "a_vertex_xyzw" , VA_TYPE_OFFSET(float,  0)},
		{1,  3, GL_FLOAT        ,  (sizeof(float) * 11 + sizeof(uint8_t) * 8),  "a_normal_xyz"  , VA_TYPE_OFFSET(float,  4)},
		{2,  4, GL_FLOAT        ,  (sizeof(float) * 11 + sizeof(uint8_t) * 8),  "a_texcoor_stuv", VA_TYPE_OFFSET(float,  7)},
		{3,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 11 + sizeof(uint8_t) * 8),  "a_color0_rgba" , VA_TYPE_OFFSET(float, 11)},
		{4,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 11 + sizeof(uint8_t) * 8),  "a_color1_rgba" , VA_TYPE_OFFSET(float, 12)},
	}};


	const static size_t NUM_VA_TYPE_0_ATTRS = VA_TYPE_0_ATTRS.size(); // (sizeof(VA_TYPE_0_ATTRS) / sizeof(VA_TYPE_0_ATTRS[0]));
	const static size_t NUM_VA_TYPE_C_ATTRS = VA_TYPE_C_ATTRS.size(); // (sizeof(VA_TYPE_C_ATTRS) / sizeof(VA_TYPE_C_ATTRS[0]));
	const static size_t NUM_VA_TYPE_FC_ATTRS = VA_TYPE_FC_ATTRS.size(); // (sizeof(VA_TYPE_FC_ATTRS) / sizeof(VA_TYPE_FC_ATTRS[0]));
	const static size_t NUM_VA_TYPE_T_ATTRS = VA_TYPE_T_ATTRS.size(); // (sizeof(VA_TYPE_T_ATTRS) / sizeof(VA_TYPE_T_ATTRS[0]));
	const static size_t NUM_VA_TYPE_T4_ATTRS = VA_TYPE_T4_ATTRS.size(); // (sizeof(VA_TYPE_T4_ATTRS) / sizeof(VA_TYPE_T4_ATTRS[0]));
	const static size_t NUM_VA_TYPE_TN_ATTRS = VA_TYPE_TN_ATTRS.size(); // (sizeof(VA_TYPE_TN_ATTRS) / sizeof(VA_TYPE_TN_ATTRS[0]));
	const static size_t NUM_VA_TYPE_TC_ATTRS = VA_TYPE_TC_ATTRS.size(); //  (sizeof(VA_TYPE_TC_ATTRS) / sizeof(VA_TYPE_TC_ATTRS[0]));
	const static size_t NUM_VA_TYPE_2D0_ATTRS = VA_TYPE_2D0_ATTRS.size(); // (sizeof(VA_TYPE_2D0_ATTRS) / sizeof(VA_TYPE_2D0_ATTRS[0]));
	const static size_t NUM_VA_TYPE_2DT_ATTRS = VA_TYPE_2DT_ATTRS.size(); // (sizeof(VA_TYPE_2DT_ATTRS) / sizeof(VA_TYPE_2DT_ATTRS[0]));
	const static size_t NUM_VA_TYPE_2DTC_ATTRS = VA_TYPE_2DTC_ATTRS.size(); // (sizeof(VA_TYPE_2DTC_ATTRS) / sizeof(VA_TYPE_2DTC_ATTRS[0]));
	const static size_t NUM_VA_TYPE_L_ATTRS = VA_TYPE_L_ATTRS.size(); // (sizeof(VA_TYPE_L_ATTRS) / sizeof(VA_TYPE_L_ATTRS[0]));


	struct RenderDataBuffer {
	public:
		RenderDataBuffer() = default;
		RenderDataBuffer(const RenderDataBuffer& rdb) = delete;
		RenderDataBuffer(RenderDataBuffer&& rdb) { *this = std::move(rdb); }

		RenderDataBuffer& operator = (const RenderDataBuffer& rdb) = delete;
		RenderDataBuffer& operator = (RenderDataBuffer&& rdb) {
			elems = std::move(rdb.elems);
			indcs = std::move(rdb.indcs);
			array = std::move(rdb.array);

			shader = std::move(rdb.shader);

			inited    = rdb.inited;
			mapped[0] = rdb.mapped[0];
			mapped[1] = rdb.mapped[1];
			return *this;
		}

		// must be called manually; allows {con,de}struction in global scope
		// VAO and VBO ctors do not call GL functions for this reason either
		bool Init(bool persistent = false, bool readable = false) {
			if (inited)
				return false;

			elems = std::move(VBO(GL_ARRAY_BUFFER, persistent, readable));
			indcs = std::move(VBO(GL_ELEMENT_ARRAY_BUFFER, persistent, readable));
			shader = std::move(Shader::GLSLProgramObject());

			elems.Generate();
			indcs.Generate();
			array.Generate();

			// defaults
			SetElemBufferUsage(GL_STATIC_DRAW);
			SetIndxBufferUsage(GL_STATIC_DRAW);

			inited    =  true;
			mapped[0] = false;
			mapped[1] = false;
			return true;
		}
		bool Kill() {
			if (!inited)
				return false;

			elems.Release();
			indcs.Release();
			array.Delete();
			// do not delete the attached objects
			shader.Release(false);

			inited    = false;
			mapped[0] = false;
			mapped[1] = false;
			return true;
		}

		void SetElemBufferUsage(unsigned int usage) { elems.usage = usage; }
		void SetIndxBufferUsage(unsigned int usage) { indcs.usage = usage; }

		void EnableShader() { shader.Enable(); }
		void DisableShader() { shader.Disable(); }

		void EnableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const;
		void DisableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const;

		static const char* GetShaderName(const char* s) {
			switch (hashString(s)) {
				case hashString("0"  ): case hashString("VA_TYPE_0"  ): { return "[VA_SHADER_0]"; } break;
				case hashString("C"  ): case hashString("VA_TYPE_C"  ): { return "[VA_SHADER_C]"; } break;
				case hashString("T"  ): case hashString("VA_TYPE_T"  ): { return "[VA_SHADER_T]"; } break;

				case hashString("T4" ): case hashString("VA_TYPE_T4" ): { return "[VA_SHADER_T4]"; } break;
				case hashString("TN" ): case hashString("VA_TYPE_TN" ): { return "[VA_SHADER_TN]"; } break;
				case hashString("TC" ): case hashString("VA_TYPE_TC" ): { return "[VA_SHADER_TC]"; } break;

				case hashString("2D0"): case hashString("VA_TYPE_2D0"): { return "[VA_SHADER_2D0]"; } break;
				case hashString("2DT"): case hashString("VA_TYPE_2DT"): { return "[VA_SHADER_2DT]"; } break;

				// VA_TYPE_L has no associated shader
				default: {
				} break;
			}

			return "";
		}

		static char* FormatShaderBase(
			char* buf,
			const char* end,
			const char* defines, // "#define PI 3.14159\n"
			const char* globals, // custom uniforms, consts, etc
			const char* type,
			const char* name
		);
		static char* FormatShaderType(
			char* buf,
			char* ptr,
			const char* end,
			size_t numAttrs,
			const Shader::ShaderInput* rawAttrs,
			const char* code, // body of main()
			const char* type, // "VS", "FS"
			const char* name  // "VA_TYPE_*"
		);

		static char* FormatShader0(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_0");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_0_ATTRS.size(), VA_TYPE_0_ATTRS.data(),  code, type, "VA_TYPE_0");
			return ptr;
		}
		static char* FormatShaderC(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_C");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_C_ATTRS.size(), VA_TYPE_C_ATTRS.data(),  code, type, "VA_TYPE_C");
			return ptr;
		}
		static char* FormatShaderFC(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_FC");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_FC_ATTRS.size(), VA_TYPE_FC_ATTRS.data(),  code, type, "VA_TYPE_FC");
			return ptr;
		}
		static char* FormatShaderT(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_T");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_T_ATTRS.size(), VA_TYPE_T_ATTRS.data(),  code, type, "VA_TYPE_T");
			return ptr;
		}
		static char* FormatShaderT4(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_T4");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_T4_ATTRS.size(), VA_TYPE_T4_ATTRS.data(),  code, type, "VA_TYPE_T4");
			return ptr;
		}
		static char* FormatShaderTN(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_TN");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_TN_ATTRS.size(), VA_TYPE_TN_ATTRS.data(),  code, type, "VA_TYPE_TN");
			return ptr;
		}
		static char* FormatShaderTC(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_TC");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_TC_ATTRS.size(), VA_TYPE_TC_ATTRS.data(),  code, type, "VA_TYPE_TC");
			return ptr;
		}
		static char* FormatShader2D0(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_2D0");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_2D0_ATTRS.size(), VA_TYPE_2D0_ATTRS.data(),  code, type, "VA_TYPE_2D0");
			return ptr;
		}
		static char* FormatShader2DT(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_2DT");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_2DT_ATTRS.size(), VA_TYPE_2DT_ATTRS.data(),  code, type, "VA_TYPE_2DT");
			return ptr;
		}
		static char* FormatShader2DTC(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_2DTC");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_2DTC_ATTRS.size(), VA_TYPE_2DTC_ATTRS.data(),  code, type, "VA_TYPE_2DTC");
			return ptr;
		}
		static char* FormatShaderL(char* buf, const char* end,  const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, end, defines, globals, type, "VA_TYPE_L");
			ptr = FormatShaderType(buf, ptr, end,  VA_TYPE_L_ATTRS.size(), VA_TYPE_L_ATTRS.data(),  code, type, "VA_TYPE_L");
			return ptr;
		}

		Shader::GLSLProgramObject* CreateShader(
			size_t numObjects,
			size_t numUniforms,
			Shader::GLSLShaderObject* objects,
			const Shader::ShaderInput* uniforms,
			const char* progName = ""
		);

		void Submit(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const;
		void SubmitInstanced(uint32_t primType, uint32_t dataIndx, uint32_t dataSize, uint32_t numInsts) const;
		void SubmitIndexed(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const;
		void SubmitIndexedInstanced(uint32_t primType, uint32_t dataIndx, uint32_t dataSize, uint32_t numInsts) const;
		void Upload(
			size_t numElems, // in bytes!
			size_t numIndcs, // in bytes!
			size_t numAttrs,
			const uint8_t* rawElems,
			const uint8_t* rawIndcs,
			const Shader::ShaderInput* rawAttrs
		);

		template<typename T> static const uint8_t* Cast(const T* p) { return (reinterpret_cast<const uint8_t*>(p)); }
		template<typename T, typename I, typename A> void TUpload(
			size_t numElems,
			size_t numIndcs,
			size_t numAttrs,
			const T* typedElems,
			const I* typedIndcs,
			const A* typedAttrs
		) {
			Upload((numElems * sizeof(T)), (numIndcs * sizeof(I)), numAttrs,  Cast(typedElems), Cast(typedIndcs), typedAttrs);
		}

		// typed versions
		void Upload0   (size_t numElems, size_t numIndcs,  const VA_TYPE_0*    e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_0_ATTRS.size()   ,  e, i, VA_TYPE_0_ATTRS.data()); }
		void UploadC   (size_t numElems, size_t numIndcs,  const VA_TYPE_C*    e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_C_ATTRS.size()   ,  e, i, VA_TYPE_C_ATTRS.data()); }
		void UploadFC  (size_t numElems, size_t numIndcs,  const VA_TYPE_C*    e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_FC_ATTRS.size()  ,  e, i, VA_TYPE_FC_ATTRS.data()); }
		void UploadT   (size_t numElems, size_t numIndcs,  const VA_TYPE_T*    e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_T_ATTRS.size()   ,  e, i, VA_TYPE_T_ATTRS.data()); }
		void UploadT4  (size_t numElems, size_t numIndcs,  const VA_TYPE_T4*   e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_T4_ATTRS.size()  ,  e, i, VA_TYPE_T4_ATTRS.data()); }
		void UploadTN  (size_t numElems, size_t numIndcs,  const VA_TYPE_TN*   e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_TN_ATTRS.size()  ,  e, i, VA_TYPE_TN_ATTRS.data()); }
		void UploadTC  (size_t numElems, size_t numIndcs,  const VA_TYPE_TC*   e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_TC_ATTRS.size()  ,  e, i, VA_TYPE_TC_ATTRS.data()); }
		void Upload2D0 (size_t numElems, size_t numIndcs,  const VA_TYPE_2d0*  e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_2D0_ATTRS.size() ,  e, i, VA_TYPE_2D0_ATTRS.data()); }
		void Upload2DT (size_t numElems, size_t numIndcs,  const VA_TYPE_2dT*  e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_2DT_ATTRS.size() ,  e, i, VA_TYPE_2DT_ATTRS.data()); }
		void Upload2DTC(size_t numElems, size_t numIndcs,  const VA_TYPE_2dTC* e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_2DTC_ATTRS.size(),  e, i, VA_TYPE_2DTC_ATTRS.data()); }
		void UploadL   (size_t numElems, size_t numIndcs,  const VA_TYPE_L*    e, const uint32_t* i) { TUpload(numElems, numIndcs, VA_TYPE_L_ATTRS.size()   ,  e, i, VA_TYPE_L_ATTRS.data()); }


		template<typename T> T* MapElems(bool bind, bool unbind, bool r = false, bool w = true) { mapped[0] = true; return (MapBuffer<T>(elems, bind, unbind, r, w)); }
		template<typename T> T* MapIndcs(bool bind, bool unbind, bool r = false, bool w = true) { mapped[1] = true; return (MapBuffer<T>(indcs, bind, unbind, r, w)); }

		void UnmapElems(bool unbind = false) { elems.UnmapBuffer(); if (unbind) { elems.Unbind(); } mapped[0] = false; }
		void UnmapIndcs(bool unbind = false) { indcs.UnmapBuffer(); if (unbind) { indcs.Unbind(); } mapped[1] = false; }

		VAO& GetArray() { return array; }
		VBO& GetElems() { return elems; }
		VBO& GetIndcs() { return indcs; }
		Shader::GLSLProgramObject& GetShader() { return shader; }

		template<typename T> size_t GetNumElems() const { return (elems.GetSize() / sizeof(T)); }
		template<typename T> size_t GetNumIndcs() const { return (indcs.GetSize() / sizeof(T)); }

		bool IsInited() const { return inited; }
		bool IsMapped() const { return (mapped[0] || mapped[1]); }
		bool IsPinned() const { return elems.immutableStorage; }

	private:
		template<typename T> static T* MapBuffer(VBO& vbo, bool bind, bool unbind, bool read, bool write) {
			if (bind)
				vbo.Bind();

			// read and write cannot both be false
			constexpr unsigned int flags[] = {0, GL_READ_ONLY, GL_WRITE_ONLY, GL_READ_WRITE};
			const     unsigned int flag    = flags[int(read) + int(write) * 2];

			T* ptr = reinterpret_cast<T*>(vbo.MapBuffer(flag));

			if (unbind)
				vbo.Unbind();

			return ptr;
		}

	private:
		VBO elems;
		VBO indcs;
		VAO array;

		Shader::GLSLProgramObject shader;

		bool inited    = false;
		bool mapped[2] = {false, false};
	};



	#ifdef HEADLESS
	template<typename VA_TYPE> struct TRenderDataBuffer {
	public:
		typedef VA_TYPE VertexArrayType;
		typedef uint32_t IndexArrayType;

		TRenderDataBuffer() = default;
		TRenderDataBuffer(const TRenderDataBuffer& trdb) = delete;
		TRenderDataBuffer(TRenderDataBuffer&& trdb) { *this = std::move(trdb); }

		TRenderDataBuffer& operator = (const TRenderDataBuffer& trdb) = delete;
		TRenderDataBuffer& operator = (TRenderDataBuffer&& trdb) { std::swap(rawBuffer, trdb.rawBuffer); return *this; }

		template<typename VertexAttribArray> void Setup(
			RenderDataBuffer* buffer,
			const VertexAttribArray* attrs,
			size_t numElems = 1 << 18,
			size_t numIndcs = 1 << 16
		) {
			rawBuffer = buffer;
		}
		template<typename VertexAttribArray> void SetupStatic(
			RenderDataBuffer* buffer,
			const VertexAttribArray* attrs,
			size_t numElems = 1 << 18,
			size_t numIndcs = 1 << 16
		) {
			rawBuffer = buffer;
		}


		VertexArrayType* BindMapElems(bool r = false, bool w = true) { return (static_cast<VertexArrayType*>(nullptr)); }
		 IndexArrayType* BindMapIndcs(bool r = false, bool w = true) { return (static_cast< IndexArrayType*>(nullptr)); }

		void UnmapUnbindElems() {}
		void UnmapUnbindIndcs() {}


		bool Wait() { return false; }
		bool Sync() { return false; }

		void Reset() { Reset(0, 0); }
		void Reset(size_t elemsPos, size_t indcsPos) {}


		bool CheckSizeE(size_t ne, size_t pos) const { return false; }
		bool CheckSizeI(size_t ni, size_t pos) const { return false; }


		void Update(const VertexArrayType& e,            size_t pos) {}
		void Update(const  IndexArrayType  i,            size_t pos) {}
		void Update(const VertexArrayType* e, size_t ne, size_t pos) {}
		void Update(const  IndexArrayType* i, size_t ni, size_t pos) {}

		bool SafeUpdate(const VertexArrayType& e,            size_t pos) { return false; }
		bool SafeUpdate(const  IndexArrayType  i,            size_t pos) { return false; }
		bool SafeUpdate(const VertexArrayType* e, size_t ne, size_t pos) { return false; }
		bool SafeUpdate(const  IndexArrayType* i, size_t ni, size_t pos) { return false; }


		void Append(const VertexArrayType& e           ) {}
		void Append(const  IndexArrayType  i           ) {}
		void Append(const VertexArrayType* e, size_t ne) {}
		void Append(const  IndexArrayType* i, size_t ni) {}

		bool SafeAppend(const VertexArrayType& e           ) { return false; }
		bool SafeAppend(const  IndexArrayType  i           ) { return false; }
		bool SafeAppend(const VertexArrayType* e, size_t ne) { return false; }
		bool SafeAppend(const  IndexArrayType* i, size_t ni) { return false; }


		void Submit(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const {}
		void Submit(uint32_t primType) {}
		void SubmitIndexed(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const {}
		void SubmitIndexed(uint32_t primType) {}

		size_t NumElems() const { return 0; }
		size_t NumIndcs() const { return 0; }
		size_t SumElems() const { return 0; }
		size_t SumIndcs() const { return 0; }
		size_t NumSubmits(bool indexed) const { return 0; }

		GL::RenderDataBuffer* GetBuffer() { return rawBuffer; }
		Shader::IProgramObject* GetShader() { return &(rawBuffer->GetShader()); }

		VertexArrayType* GetElemsMap() { return nullptr; }
		 IndexArrayType* GetIndcsMap() { return nullptr; }

	private:
		RenderDataBuffer* rawBuffer = nullptr;
	};

	#else

	// typed persistent wrapper
	template<typename VA_TYPE> struct TRenderDataBuffer {
	public:
		typedef VA_TYPE VertexArrayType;
		typedef uint32_t IndexArrayType; // GL_UNSIGNED_INT

		TRenderDataBuffer() = default;
		TRenderDataBuffer(const TRenderDataBuffer& trdb) = delete;
		TRenderDataBuffer(TRenderDataBuffer&& trdb) { *this = std::move(trdb); }

		TRenderDataBuffer& operator = (const TRenderDataBuffer& trdb) = delete;
		TRenderDataBuffer& operator = (TRenderDataBuffer&& trdb) {
			std::swap(rawBuffer, trdb.rawBuffer);

			std::swap(elemsMap, trdb.elemsMap);
			std::swap(indcsMap, trdb.indcsMap);

			std::swap(prvElemPos, trdb.prvElemPos);
			std::swap(curElemPos, trdb.curElemPos);
			std::swap(sumElemPos, trdb.sumElemPos);
			std::swap(prvIndxPos, trdb.prvIndxPos);
			std::swap(curIndxPos, trdb.curIndxPos);
			std::swap(sumIndxPos, trdb.sumIndxPos);

			std::swap(numSubmits[0], trdb.numSubmits[0]);
			std::swap(numSubmits[1], trdb.numSubmits[1]);

			std::swap(glSyncObj, trdb.glSyncObj);
			return *this;
		}

		// NOTE: potential mismatch between VertexArrayType and VertexAttribArray (std::array<T, N>)
		template<typename VertexAttribArray> void Setup(
			RenderDataBuffer* buffer,
			const VertexAttribArray* attribs,
			size_t numElems = 1 << 18,
			size_t numIndcs = 1 << 16
		) {
			rawBuffer = buffer;

			rawBuffer->Init(true);
			rawBuffer->TUpload<VertexArrayType, IndexArrayType, Shader::ShaderInput>(numElems, numIndcs, attribs->size(),  nullptr, nullptr, attribs->data());

			elemsMap = rawBuffer->MapElems<VertexArrayType>(true, true);
			indcsMap = rawBuffer->MapIndcs< IndexArrayType>(true, true); // null if numIndcs is 0
		}

		template<typename VertexAttribArray> void SetupStatic(
			RenderDataBuffer* buffer,
			const VertexAttribArray* attribs,
			size_t numElems = 1 << 18,
			size_t numIndcs = 1 << 16
		) {
			rawBuffer = buffer;

			rawBuffer->Init(false);
			rawBuffer->TUpload<VertexArrayType, IndexArrayType, Shader::ShaderInput>(numElems, numIndcs, attribs->size(),  nullptr, nullptr, attribs->data());
		}


		VertexArrayType* BindMapElems(bool r = false, bool w = true) { assert(!rawBuffer->IsPinned()); return (elemsMap = rawBuffer->MapElems<VertexArrayType>(true, false, r, w)); }
		 IndexArrayType* BindMapIndcs(bool r = false, bool w = true) { assert(!rawBuffer->IsPinned()); return (indcsMap = rawBuffer->MapIndcs< IndexArrayType>(true, false, r, w)); }

		void UnmapUnbindElems() { assert(!rawBuffer->IsPinned()); rawBuffer->UnmapElems(true); elemsMap = nullptr; }
		void UnmapUnbindIndcs() { assert(!rawBuffer->IsPinned()); rawBuffer->UnmapIndcs(true); indcsMap = nullptr; }


		static GLsync WaitSync(const GLsync& syncObj) {
			#if (SYNC_RENDER_BUFFERS == 1)
			#ifndef HEADLESS
			constexpr GLuint64 NANOSECS_PER_SEC = 1000000000;

			GLbitfield waitFlag = 0;
			GLuint64 waitTime = 0;

			assert(syncObj != 0);

			for (int i = 0; i < 10; i++) {
				const GLenum waitRet = glClientWaitSync(syncObj, waitFlag, waitTime);

				if (waitRet == GL_ALREADY_SIGNALED || waitRet == GL_CONDITION_SATISFIED)
					break;

				if (waitRet == GL_WAIT_FAILED) {
					assert(false);
					break;
				}

				// start flushing
				waitFlag = GL_SYNC_FLUSH_COMMANDS_BIT;
				waitTime = NANOSECS_PER_SEC;
			}

			glDeleteSync(syncObj);
			return {0};
			#endif
			#endif
		}


		// only wait on the first access this frame, and never indefinitely
		bool Wait() { return (glSyncObj == 0 || (glSyncObj = WaitSync(glSyncObj)) == 0); }
		bool Sync() { return (glSyncObj == 0 && (glSyncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)) != 0); }

		void Reset() {
			Reset(0, 0);

			numSubmits[0] = 0;
			numSubmits[1] = 0;
		}
		void Reset(size_t elemsPos, size_t indcsPos) {
			// there should never be data left unsubmitted
			assert(NumElems() == 0);
			assert(NumIndcs() == 0);

			prvElemPos = elemsPos;
			curElemPos = elemsPos;
			sumElemPos = elemsPos;

			prvIndxPos = indcsPos;
			curIndxPos = indcsPos;
			sumIndxPos = indcsPos;
		}


		bool CheckSizeE(size_t ne, size_t pos) const { return (ne > 0 && ((pos + (ne - 1)) < rawBuffer->GetNumElems<VertexArrayType>())); }
		bool CheckSizeI(size_t ni, size_t pos) const { return (ni > 0 && ((pos + (ni - 1)) < rawBuffer->GetNumIndcs< IndexArrayType>())); }

		void AssertSizeE(size_t ne, size_t pos) const { assert(CheckSizeE(ne, pos)); }
		void AssertSizeI(size_t ni, size_t pos) const { assert(CheckSizeI(ni, pos)); }


		void Update(const VertexArrayType& e,            size_t pos) {      Update(&e,  1, pos);                                                               }
		void Update(const  IndexArrayType  i,            size_t pos) {      Update(&i,  1, pos);                                                               }
		void Update(const VertexArrayType* e, size_t ne, size_t pos) { AssertSizeE(    ne, pos); std::memcpy(&elemsMap[pos], e, ne * sizeof(VertexArrayType)); }
		void Update(const  IndexArrayType* i, size_t ni, size_t pos) { AssertSizeI(    ni, pos); std::memcpy(&indcsMap[pos], i, ni * sizeof( IndexArrayType)); }

		bool SafeUpdate(const VertexArrayType& e,            size_t pos) { return SafeUpdate(&e, 1, pos); }
		bool SafeUpdate(const  IndexArrayType  i,            size_t pos) { return SafeUpdate(&i, 1, pos); }
		bool SafeUpdate(const VertexArrayType* e, size_t ne, size_t pos) {
			if (elemsMap == nullptr || !CheckSizeE(ne, pos))
				return false;
			return (Update(e, ne, pos), true);
		}
		bool SafeUpdate(const  IndexArrayType* i, size_t ni, size_t pos) {
			if (indcsMap == nullptr || !CheckSizeI(ni, pos))
				return false;
			return (Update(i, ni, pos), true);
		}


		void Append(const VertexArrayType& e           ) { Append(&e,  1            );                   }
		void Append(const  IndexArrayType  i           ) { Append(&i,  1            );                   }
		void Append(const VertexArrayType* e, size_t ne) { Update( e, ne, curElemPos); curElemPos += ne; }
		void Append(const  IndexArrayType* i, size_t ni) { Update( i, ni, curIndxPos); curIndxPos += ni; }

		bool SafeAppend(const VertexArrayType& e           ) { return SafeAppend(&e, 1); }
		bool SafeAppend(const VertexArrayType* e, size_t ne) {
			if (elemsMap == nullptr || !CheckSizeE(ne, curElemPos))
				return false;
			return (Append(e, ne), true);
		}
		bool SafeAppend(const  IndexArrayType  i           ) { return SafeAppend(&i, 1); }
		bool SafeAppend(const  IndexArrayType* i, size_t ni) {
			if (indcsMap == nullptr || !CheckSizeI(ni, curIndxPos))
				return false;
			return (Append(i, ni), true);
		}


		void Submit(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const { rawBuffer->Submit(primType, dataIndx, dataSize); }
		void Submit(uint32_t primType) {
			if (NumElems() > 0)
				rawBuffer->Submit(primType, prvElemPos, NumElems());

			numSubmits[0] += 1;
			sumElemPos += NumElems();
			prvElemPos = curElemPos;
		}
		void SubmitIndexed(uint32_t primType, uint32_t dataIndx, uint32_t dataSize) const { rawBuffer->SubmitIndexed(primType, dataIndx, dataSize); }
		void SubmitIndexed(uint32_t primType) {
			if (NumIndcs() > 0)
				rawBuffer->SubmitIndexed(primType, prvIndxPos, NumIndcs());

			// TODO: allow multiple batches with the same set of indices?
			numSubmits[1] += 1;
			sumElemPos += NumElems();
			prvElemPos = curElemPos;
			sumIndxPos += NumIndcs();
			prvIndxPos = curIndxPos;
		}

		size_t NumElems() const { return (curElemPos - prvElemPos); }
		size_t NumIndcs() const { return (curIndxPos - prvIndxPos); }
		size_t SumElems() const { return sumElemPos; }
		size_t SumIndcs() const { return sumIndxPos; }
		size_t NumSubmits(bool indexed) const { return numSubmits[indexed]; }

		GL::RenderDataBuffer* GetBuffer() { return rawBuffer; }
		Shader::IProgramObject* GetShader() { return &(rawBuffer->GetShader()); }

		VertexArrayType* GetElemsMap() { return elemsMap; }
		 IndexArrayType* GetIndcsMap() { return indcsMap; }

	private:
		RenderDataBuffer* rawBuffer = nullptr;

		VertexArrayType* elemsMap = nullptr;
		IndexArrayType* indcsMap = nullptr;

		// these must never exceed rawBuffer->GetNum{Elems,Indcs}<{Vertex,Index}ArrayType>()
		size_t prvElemPos = 0;
		size_t curElemPos = 0;
		size_t sumElemPos = 0;

		size_t prvIndxPos = 0;
		size_t curIndxPos = 0;
		size_t sumIndxPos = 0;

		// [0] := non-indexed, [1] := indexed
		size_t numSubmits[2] = {0, 0};

		GLsync glSyncObj = 0;
	};
	#endif


	void InitRenderBuffers();
	void KillRenderBuffers();
	void SwapRenderBuffers();


	RenderDataBuffer0* GetRenderBuffer0();
	RenderDataBufferC* GetRenderBufferC();
	RenderDataBufferC* GetRenderBufferFC();
	RenderDataBufferT* GetRenderBufferT();

	RenderDataBufferT4* GetRenderBufferT4();
	RenderDataBufferTN* GetRenderBufferTN();
	RenderDataBufferTC* GetRenderBufferTC();

	RenderDataBuffer2D0* GetRenderBuffer2D0();
	RenderDataBuffer2DT* GetRenderBuffer2DT();

	RenderDataBufferL* GetRenderBufferL();
};

#endif

