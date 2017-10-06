/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GL_RENDERDATABUFFER_HDR
#define GL_RENDERDATABUFFER_HDR

#include "VAO.h"
#include "VBO.h"
#include "VertexArrayTypes.h"
#include "Rendering/Shaders/Shader.h"

namespace GL {
	// clang does not like reinterpret_cast inside constexpr's
	// it apparently also can not handle 0-pointer arithmetic
	#if (defined(__clang__)) || (defined(_MSC_VER))
	#define constqual const
	// #define OFFSET(T, n) (static_cast<uint8_t*>(nullptr) + sizeof(T) * (n))
	#define OFFSET(T, n) (reinterpret_cast<void*>(sizeof(T) * (n)))
	#else
	#define constqual constexpr
	#define OFFSET(T, n) ((const T*)(0) + (n))
	#endif

	static_assert(sizeof(VA_TYPE_0) == sizeof(float3), "");
	static_assert(sizeof(VA_TYPE_0) == (sizeof(float) * 3), "");
	constqual static Shader::ShaderInput VA_TYPE_0_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 3),  "a_vertex_xyz", OFFSET(float, 0)},
	};


	static_assert(sizeof(VA_TYPE_N) == (sizeof(float3) * 2), "");
	static_assert(sizeof(VA_TYPE_N) == (sizeof(float) * 6), ""); // 6 = 3 + 3
	constqual static Shader::ShaderInput VA_TYPE_N_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 6),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  3, GL_FLOAT,  (sizeof(float) * 6),  "a_normal_xyz", OFFSET(float, 3)},
	};


	#if 0
	static_assert((VA_SIZE_C + 3) == 7, "");
	constqual static Shader::ShaderInput VA_TYPE_C_ATTRS[] = {
		{0,  3, GL_FLOAT,  7 * sizeof(float),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  4, GL_FLOAT,  7 * sizeof(float),  "a_color_rgba", OFFSET(float, 3)},
	};
	#else
	static_assert(sizeof(VA_TYPE_C) == (sizeof(float) * 3 + sizeof(uint32_t)), "");
	constqual static Shader::ShaderInput VA_TYPE_C_ATTRS[] = {
		{0,  3, GL_FLOAT        ,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 3 + sizeof(uint8_t) * 4),  "a_color_rgba", OFFSET(float, 3)},
	};
	#endif


	static_assert(sizeof(VA_TYPE_T) == (sizeof(float3) + sizeof(float) * 2), "");
	static_assert(sizeof(VA_TYPE_T) == (sizeof(float) * 5), ""); // 5 = 3 + 2
	constqual static Shader::ShaderInput VA_TYPE_T_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 5),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 5),  "a_texcoor_st", OFFSET(float, 3)},
	};


	static_assert(sizeof(VA_TYPE_TN) == (sizeof(float3) * 2 + sizeof(float) * 2), "");
	static_assert(sizeof(VA_TYPE_TN) == (sizeof(float) * 8), ""); // 8 = 3 + 2 + 3
	constqual static Shader::ShaderInput VA_TYPE_TN_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 8),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_texcoor_st", OFFSET(float, 3)},
		{2,  3, GL_FLOAT,  (sizeof(float) * 8),  "a_normal_xyz", OFFSET(float, 5)},
	};


	#if 0
	static_assert((VA_SIZE_TC + 3) == 9, "");
	constqual static Shader::ShaderInput VA_TYPE_TC_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 9),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 9),  "a_texcoor_st", OFFSET(float, 3)},
		{2,  4, GL_FLOAT,  (sizeof(float) * 9),  "a_color_rgba", OFFSET(float, 5)},
	};
	#else
	static_assert(sizeof(VA_TYPE_TC) == (sizeof(float) * (3 + 2) + sizeof(uint32_t)), "");
	constqual static Shader::ShaderInput VA_TYPE_TC_ATTRS[] = {
		{0,  3, GL_FLOAT        ,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_vertex_xyz", OFFSET(float, 0)},
		{1,  2, GL_FLOAT        ,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_texcoor_st", OFFSET(float, 3)},
		{2,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 5 + sizeof(uint8_t) * 4),  "a_color_rgba", OFFSET(float, 5)},
	};
	#endif


	static_assert(sizeof(VA_TYPE_TNT) == (sizeof(float3) * 4 + sizeof(float) * 2), "");
	static_assert(sizeof(VA_TYPE_TNT) == (sizeof(float) * 14), ""); // 14 = 3 + 2 + 3 + 3 + 3
	constqual static Shader::ShaderInput VA_TYPE_TNT_ATTRS[] = {
		{0,  3, GL_FLOAT,  (sizeof(float) * 14),  "a_vertex_xyz" , OFFSET(float,  0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 14),  "a_texcoor_st" , OFFSET(float,  3)},
		{2,  3, GL_FLOAT,  (sizeof(float) * 14),  "a_normal_xyz" , OFFSET(float,  5)},
		{3,  3, GL_FLOAT,  (sizeof(float) * 14),  "a_texcoor_uv1", OFFSET(float,  8)},
		{4,  3, GL_FLOAT,  (sizeof(float) * 14),  "a_texcoor_uv2", OFFSET(float, 11)},
	};


	static_assert(sizeof(VA_TYPE_2d0) == (sizeof(float) * 2), "");
	constqual static Shader::ShaderInput VA_TYPE_2D0_ATTRS[] = {
		{0,  2, GL_FLOAT,  (sizeof(float) * 2),  "a_vertex_xy", OFFSET(float, 0)},
	};


	static_assert(sizeof(VA_TYPE_2dT) == (sizeof(float) * 4), ""); // 4 = 2 + 2
	constqual static Shader::ShaderInput VA_TYPE_2DT_ATTRS[] = {
		{0,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_vertex_xy" , OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 4),  "a_texcoor_st", OFFSET(float, 2)},
	};


	#if 0
	static_assert((VA_SIZE_2DTC + 3) == 8, "");
	constqual static Shader::ShaderInput VA_TYPE_2DTC_ATTRS[] = {
		{0,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_vertex_xy" , OFFSET(float, 0)},
		{1,  2, GL_FLOAT,  (sizeof(float) * 8),  "a_texcoor_st", OFFSET(float, 2)},
		{2,  4, GL_FLOAT,  (sizeof(float) * 8),  "a_color_rgba", OFFSET(float, 4)},
	};
	#else
	static_assert(sizeof(VA_TYPE_2dTC) == (sizeof(float) * (2 + 2) + sizeof(uint32_t)), "");
	constqual static Shader::ShaderInput VA_TYPE_2DTC_ATTRS[] = {
		{0,  2, GL_FLOAT        ,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_vertex_xy" , OFFSET(float, 0)},
		{1,  2, GL_FLOAT        ,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_texcoor_st", OFFSET(float, 2)},
		{2,  4, GL_UNSIGNED_BYTE,  (sizeof(float) * 4 + sizeof(uint8_t) * 4),  "a_color_rgba", OFFSET(float, 4)},
	};
	#endif
	#undef OFFSET


	constqual static size_t NUM_VA_TYPE_0_ATTRS = (sizeof(VA_TYPE_0_ATTRS) / sizeof(VA_TYPE_0_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_N_ATTRS = (sizeof(VA_TYPE_N_ATTRS) / sizeof(VA_TYPE_N_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_C_ATTRS = (sizeof(VA_TYPE_C_ATTRS) / sizeof(VA_TYPE_C_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_T_ATTRS = (sizeof(VA_TYPE_T_ATTRS) / sizeof(VA_TYPE_T_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_TN_ATTRS = (sizeof(VA_TYPE_TN_ATTRS) / sizeof(VA_TYPE_TN_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_TC_ATTRS = (sizeof(VA_TYPE_TC_ATTRS) / sizeof(VA_TYPE_TC_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_TNT_ATTRS = (sizeof(VA_TYPE_TNT_ATTRS) / sizeof(VA_TYPE_TNT_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_2D0_ATTRS = (sizeof(VA_TYPE_2D0_ATTRS) / sizeof(VA_TYPE_2D0_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_2DT_ATTRS = (sizeof(VA_TYPE_2DT_ATTRS) / sizeof(VA_TYPE_2DT_ATTRS[0]));
	constqual static size_t NUM_VA_TYPE_2DTC_ATTRS = (sizeof(VA_TYPE_2DTC_ATTRS) / sizeof(VA_TYPE_2DTC_ATTRS[0]));


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
			return *this;
		}

		// must be called manually; allows {con,de}struction in global scope
		// VAO and VBO ctors do not call GL functions for this reason either
		void Init() {
			if (inited)
				return;

			elems = std::move(VBO(GL_ARRAY_BUFFER));
			indcs = std::move(VBO(GL_ELEMENT_ARRAY_BUFFER));
			shader = std::move(Shader::GLSLProgramObject());

			elems.Generate();
			indcs.Generate();
			array.Generate();

			inited = true;
			mapped = false;
		}
		void Kill() {
			if (!inited)
				return;

			elems.UnmapIf();
			indcs.UnmapIf();
			elems.Delete();
			indcs.Delete();
			array.Delete();
			// do not delete the attached objects
			shader.Release(false);

			inited = false;
			mapped = false;
		}

		void EnableShader() { shader.Enable(); }
		void DisableShader() { shader.Disable(); }

		void EnableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const;
		void DisableAttribs(size_t numAttrs, const Shader::ShaderInput* rawAttrs) const;

		static char* FormatShaderBase(
			char (&buf)[65536],
			const char* defines, // "#define PI 3.14159\n"
			const char* globals, // custom uniforms, consts, etc
			const char* type,
			const char* name
		);
		static char* FormatShaderType(
			char (&buf)[65536],
			char* ptr,
			size_t numAttrs,
			const Shader::ShaderInput* rawAttrs,
			const char* code, // body of main()
			const char* type, // "VS", "FS"
			const char* name  // "VA_TYPE_*"
		);

		static char* FormatShader0(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_0");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_0_ATTRS, VA_TYPE_0_ATTRS,  code, type, "VA_TYPE_0");
			return ptr;
		}
		static char* FormatShaderN(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_N");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_N_ATTRS, VA_TYPE_N_ATTRS,  code, type, "VA_TYPE_N");
			return ptr;
		}
		static char* FormatShaderC(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_C");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_C_ATTRS, VA_TYPE_C_ATTRS,  code, type, "VA_TYPE_C");
			return ptr;
		}
		static char* FormatShaderT(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_T");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_T_ATTRS, VA_TYPE_T_ATTRS,  code, type, "VA_TYPE_T");
			return ptr;
		}
		static char* FormatShaderTN(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_TN");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_TN_ATTRS, VA_TYPE_TN_ATTRS,  code, type, "VA_TYPE_TN");
			return ptr;
		}
		static char* FormatShaderTC(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_TC");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_TC_ATTRS, VA_TYPE_TC_ATTRS,  code, type, "VA_TYPE_TC");
			return ptr;
		}
		static char* FormatShaderTNT(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_TNT");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_TNT_ATTRS, VA_TYPE_TNT_ATTRS,  code, type, "VA_TYPE_TNT");
			return ptr;
		}
		static char* FormatShader2D0(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_2D0");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_2D0_ATTRS, VA_TYPE_2D0_ATTRS,  code, type, "VA_TYPE_2D0");
			return ptr;
		}
		static char* FormatShader2DT(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_2DT");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_2DT_ATTRS, VA_TYPE_2DT_ATTRS,  code, type, "VA_TYPE_2DT");
			return ptr;
		}
		static char* FormatShader2DTC(char (&buf)[65536], const char* defines, const char* globals, const char* code, const char* type) {
			char* ptr = &buf[0];
			ptr = FormatShaderBase(buf, defines, globals, type, "VA_TYPE_2DTC");
			ptr = FormatShaderType(buf, ptr,  NUM_VA_TYPE_2DTC_ATTRS, VA_TYPE_2DTC_ATTRS,  code, type, "VA_TYPE_2DTC");
			return ptr;
		}

		void CreateShader(size_t numObjects, size_t numUniforms, Shader::GLSLShaderObject* objects, const Shader::ShaderInput* uniforms);

		void Submit(uint32_t primType, uint32_t dataSize, uint32_t dataType);
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
		void Upload0   (size_t numElems, size_t numIndcs,  const VA_TYPE_0*    e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_0_ATTRS   ,  e, i, VA_TYPE_0_ATTRS); }
		void UploadN   (size_t numElems, size_t numIndcs,  const VA_TYPE_N*    e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_N_ATTRS   ,  e, i, VA_TYPE_N_ATTRS); }
		void UploadC   (size_t numElems, size_t numIndcs,  const VA_TYPE_C*    e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_C_ATTRS   ,  e, i, VA_TYPE_C_ATTRS); }
		void UploadT   (size_t numElems, size_t numIndcs,  const VA_TYPE_T*    e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_T_ATTRS   ,  e, i, VA_TYPE_T_ATTRS); }
		void UploadTN  (size_t numElems, size_t numIndcs,  const VA_TYPE_TN*   e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_TN_ATTRS  ,  e, i, VA_TYPE_TN_ATTRS); }
		void UploadTC  (size_t numElems, size_t numIndcs,  const VA_TYPE_TC*   e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_TC_ATTRS  ,  e, i, VA_TYPE_TC_ATTRS); }
		void UploadTNT (size_t numElems, size_t numIndcs,  const VA_TYPE_TNT*  e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_TNT_ATTRS ,  e, i, VA_TYPE_TNT_ATTRS); }
		void Upload2D0 (size_t numElems, size_t numIndcs,  const VA_TYPE_2d0*  e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_2D0_ATTRS ,  e, i, VA_TYPE_2D0_ATTRS); }
		void Upload2DT (size_t numElems, size_t numIndcs,  const VA_TYPE_2dT*  e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_2DT_ATTRS ,  e, i, VA_TYPE_2DT_ATTRS); }
		void Upload2DTC(size_t numElems, size_t numIndcs,  const VA_TYPE_2dTC* e, const uint32_t* i) { TUpload(numElems, numIndcs, NUM_VA_TYPE_2DTC_ATTRS,  e, i, VA_TYPE_2DTC_ATTRS); }
		#if 0
		void Upload0   (size_t numElems, size_t numIndcs,  const VA_TYPE_0*    e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_0_ATTRS   ,  &e[0].p.x, i, VA_TYPE_0_ATTRS); }
		void UploadN   (size_t numElems, size_t numIndcs,  const VA_TYPE_N*    e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_N_ATTRS   ,  &e[0].p.x, i, VA_TYPE_N_ATTRS); }
		void UploadC   (size_t numElems, size_t numIndcs,  const VA_TYPE_C*    e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_C_ATTRS   ,  &e[0].p.x, i, VA_TYPE_C_ATTRS); }
		void UploadT   (size_t numElems, size_t numIndcs,  const VA_TYPE_T*    e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_T_ATTRS   ,  &e[0].p.x, i, VA_TYPE_T_ATTRS); }
		void UploadTN  (size_t numElems, size_t numIndcs,  const VA_TYPE_TN*   e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_TN_ATTRS  ,  &e[0].p.x, i, VA_TYPE_TN_ATTRS); }
		void UploadTC  (size_t numElems, size_t numIndcs,  const VA_TYPE_TC*   e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_TC_ATTRS  ,  &e[0].p.x, i, VA_TYPE_TC_ATTRS); }
		void UploadTNT (size_t numElems, size_t numIndcs,  const VA_TYPE_TNT*  e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_TNT_ATTRS ,  &e[0].p.x, i, VA_TYPE_TNT_ATTRS); }
		void Upload2D0 (size_t numElems, size_t numIndcs,  const VA_TYPE_2d0*  e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_2D0_ATTRS ,  &e[0].x  , i, VA_TYPE_2D0_ATTRS); }
		void Upload2DT (size_t numElems, size_t numIndcs,  const VA_TYPE_2dT*  e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_2DT_ATTRS ,  &e[0].x  , i, VA_TYPE_2DT_ATTRS); }
		void Upload2DTC(size_t numElems, size_t numIndcs,  const VA_TYPE_2dTC* e, const uint32_t* i) { TUpload<float, uint32_t>(numElems, numIndcs, NUM_VA_TYPE_2DTC_ATTRS,  &e[0].x  , i, VA_TYPE_2DTC_ATTRS); }
		#endif


		template<typename T> T* MapElems() { mapped = true; return (reinterpret_cast<T*>(elems.MapBuffer())); }
		template<typename T> T* MapIndcs() { mapped = true; return (reinterpret_cast<T*>(indcs.MapBuffer())); }

		void UnmapElems() { elems.UnmapBuffer(); mapped = false; }
		void UnmapIndcs() { indcs.UnmapBuffer(); mapped = false; }

		Shader::GLSLProgramObject& GetShader() { return shader; }

		bool IsInited() const { return inited; }
		bool IsMapped() const { return mapped; }

	private:
		VBO elems;
		VBO indcs;
		VAO array;

		Shader::GLSLProgramObject shader;

		bool inited = false;
		bool mapped = false;
	};
};

#endif

