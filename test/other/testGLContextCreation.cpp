#include <algorithm>
#include <numeric>

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL2/SDL.h>

#include "Rendering/GL/myGL.h"
#include "System/Log/ILog.h"
#include "System/type2.h"

#define CATCH_CONFIG_MAIN
#include "lib/catch.hpp"

#define SDL_BPP(fmt) SDL_BITSPERPIXEL((fmt))

static constexpr int WIN_SIZE_X = 640;
static constexpr int WIN_SIZE_Y = 480;
static constexpr int MSAA_LEVEL =   4;

static SDL_Window*   sdlWindow = nullptr;
static SDL_GLContext glContext = nullptr;


static void LogVersionInfo(const char* glVendor, const char* glRenderer)
{
	SDL_version svc;
	SDL_version svl;

	SDL_VERSION(&svc);
	SDL_GetVersion(&svl);

	if (glVendor == nullptr) {
		LOG("\tGL    version: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
		LOG("\tGLSL  version: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
		LOG(" ");
		LOG("\tGL     vendor: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
		LOG("\tGL   renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	} else {
		LOG(" ");
		LOG("\tGL     vendor: %s", glVendor);
		LOG("\tGL   renderer: %s", glRenderer);
	}

	LOG(" ");
	LOG("\tGLEW  version: %s", reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));
	LOG("\tSDL   version: %d.%d.%d (linked) / %d.%d.%d (compiled)", svl.major, svl.minor, svl.patch, svc.major, svc.minor, svc.patch);
}



bool CheckSDLVideoModes()
{
	LOG("[%s][1]", __func__);

	// get available fullscreen/hardware modes
	const int numDisplays = SDL_GetNumVideoDisplays();

	SDL_DisplayMode ddm = {0, 0, 0, 0, nullptr};
	SDL_DisplayMode cdm = {0, 0, 0, 0, nullptr};

	// ddm is virtual, contains all displays in multi-monitor setups
	// for fullscreen windows with non-native resolutions, ddm holds
	// the original screen mode and cdm is the changed mode
	SDL_GetDesktopDisplayMode(0, &ddm);
	SDL_GetCurrentDisplayMode(0, &cdm);

	LOG(
		"[%s][2] desktop={%ix%ix%ibpp@%iHz} current={%ix%ix%ibpp@%iHz}",
		__func__,
		ddm.w, ddm.h, SDL_BPP(ddm.format), ddm.refresh_rate,
		cdm.w, cdm.h, SDL_BPP(cdm.format), cdm.refresh_rate
	);

	for (int k = 0; k < numDisplays; ++k) {
		const int numModes = SDL_GetNumDisplayModes(k);

		if (numModes <= 0) {
			LOG("\tdisplay=%d bounds=N/A modes=N/A", k + 1);
			continue;
		}

		SDL_DisplayMode cm = {0, 0, 0, 0, nullptr};
		SDL_DisplayMode pm = {0, 0, 0, 0, nullptr};
		SDL_Rect db;
		SDL_GetDisplayBounds(k, &db);

		LOG("\tdisplay=%d modes=%d bounds={x=%d, y=%d, w=%d, h=%d}", k + 1, numModes, db.x, db.y, db.w, db.h);

		for (int i = 0; i < numModes; ++i) {
			SDL_GetDisplayMode(k, i, &cm);

			// show only the largest refresh-rate and bit-depth per resolution
			if (cm.w == pm.w && cm.h == pm.h && (SDL_BPP(cm.format) < SDL_BPP(pm.format) || cm.refresh_rate < pm.refresh_rate))
				continue;

			LOG("\t\t[%2i] %ix%ix%ibpp@%iHz", int(i + 1), cm.w, cm.h, SDL_BPP(cm.format), cm.refresh_rate);

			pm = cm;
		}
	}

	// we need at least 24bpp or window-creation will fail
	return (SDL_BPP(ddm.format) >= 24);
}


static void CheckGLContexts()
{
	LOG("[%s]", __func__);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	// only enumerate versions after the core/compat dichotomy
	constexpr int2 glCtxs[] = {{3, 2}, {3, 3},  {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}};

	char glVendor[128] = {0};
	char glRenderer[128] = {0};

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		SDL_GLContext tmpContext = nullptr;

		if ((tmpContext = SDL_GL_CreateContext(sdlWindow)) == nullptr) {
			LOG("\tOpenGL %d.%d context creation failed (\"%s\")", tmpCtx.x, tmpCtx.y, SDL_GetError());
			break;
		}

		LOG("\tOpenGL %d.%d context creation passed", tmpCtx.x, tmpCtx.y);
		LOG("\t\tGL   version: %s", glGetString(GL_VERSION));
		LOG("\t\tGLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

		if (tmpCtx == glCtxs[0]) {
			strncpy(glVendor  , reinterpret_cast<const char*>(glGetString(GL_VENDOR  )), sizeof(glVendor  ));
			strncpy(glRenderer, reinterpret_cast<const char*>(glGetString(GL_RENDERER)), sizeof(glRenderer));
		}

		SDL_GL_DeleteContext(tmpContext);
	}

	LogVersionInfo(glVendor, glRenderer);
}



static SDL_Window* CreateSDLWindow(const int2& size)
{
	SDL_Window* win = nullptr;

	const int aaLvls[] = {MSAA_LEVEL, MSAA_LEVEL / 2, MSAA_LEVEL / 4, MSAA_LEVEL / 8, MSAA_LEVEL / 16, MSAA_LEVEL / 32, 0};
	const int zbBits[] = {24, 32, 16};

	const char* frmts[2] = {
		"[%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer",
		"[%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\")",
	};

	for (size_t i = 0, k = sizeof(aaLvls) / sizeof(aaLvls[0]); i < k && (win == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]    );

		for (size_t j = 0, n = sizeof(zbBits) / sizeof(zbBits[0]); j < n; j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((win = SDL_CreateWindow("", 0, 0, size.x, size.y, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN)) == nullptr) {
				LOG(frmts[0], __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG(frmts[1], __func__, aaLvls[i], zbBits[j], SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(win)));
			break;
		}
	}

	return win;
}

static SDL_GLContext CreateGLContext(const int2& minCtx)
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);
	return (SDL_GL_CreateContext(sdlWindow));
}



static int QueryGLContextVersion()
{
	int2 curCtx = {0, 0};

	glGetIntegerv(GL_MAJOR_VERSION, &curCtx.x);
	glGetIntegerv(GL_MINOR_VERSION, &curCtx.y);

	return (curCtx.x * 10 + curCtx.y);
}

static int QueryGLStencilBufferBits()
{
	GLint ctxBufferBits = 0;

	// GL4.5, must be GL_STENCIL for default FB
	// glGetNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &ctxBufferBits);
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &ctxBufferBits);

	return ctxBufferBits;
}



static bool CheckGLStencilBufferBits(int minBufferBits)
{
	return (QueryGLStencilBufferBits() >= minBufferBits);
}

static bool CheckGLContextVersion(int minCtx)
{
	return (QueryGLContextVersion() >= minCtx);
}

static bool CheckGLEWContextVersion(int curCtx)
{
	int tmpCtx = 0;

	// GLEW should know this GL version exists
	switch (curCtx) {
		#ifdef GLEW_VERSION_3_0
		case 30: { tmpCtx = curCtx * GLEW_VERSION_3_0; } break;
		#endif
		#ifdef GLEW_VERSION_3_1
		case 31: { tmpCtx = curCtx * GLEW_VERSION_3_1; } break;
		#endif
		#ifdef GLEW_VERSION_3_2
		case 32: { tmpCtx = curCtx * GLEW_VERSION_3_2; } break;
		#endif
		#ifdef GLEW_VERSION_3_3
		case 33: { tmpCtx = curCtx * GLEW_VERSION_3_3; } break;
		#endif
		#ifdef GLEW_VERSION_4_0
		case 40: { tmpCtx = curCtx * GLEW_VERSION_4_0; } break;
		#endif
		#ifdef GLEW_VERSION_4_1
		case 41: { tmpCtx = curCtx * GLEW_VERSION_4_1; } break;
		#endif
		#ifdef GLEW_VERSION_4_2
		case 42: { tmpCtx = curCtx * GLEW_VERSION_4_2; } break;
		#endif
		#ifdef GLEW_VERSION_4_3
		case 43: { tmpCtx = curCtx * GLEW_VERSION_4_3; } break;
		#endif
		#ifdef GLEW_VERSION_4_4
		case 44: { tmpCtx = curCtx * GLEW_VERSION_4_4; } break;
		#endif
		#ifdef GLEW_VERSION_4_5
		case 45: { tmpCtx = curCtx * GLEW_VERSION_4_5; } break;
		#endif
		#ifdef GLEW_VERSION_4_6
		case 46: { tmpCtx = curCtx * GLEW_VERSION_4_6; } break;
		#endif
		default: {} break;
	}

	return (tmpCtx >= curCtx);
}

static bool CheckGLExtensions()
{
	const auto CheckExt = [](const char* extName, bool haveExt, bool needExt) {
		if (haveExt)
			return;
		if (needExt) {
			LOG("[%s] required OpenGL extension \"%s\" not supported", __func__, extName);
			throw extName;
		}

		LOG("[%s] optional OpenGL extension \"%s\" not supported", __func__, extName);
	};

	try {
		#define CHECK_REQ_EXT(ext) CheckExt(#ext, ext,  true)
		#define CHECK_OPT_EXT(ext) CheckExt(#ext, ext, false)

		CHECK_REQ_EXT(GLEW_ARB_multisample); // 1.3 (MSAA)

		CHECK_REQ_EXT(GLEW_ARB_vertex_buffer_object); // 1.5 (VBO)
		CHECK_REQ_EXT(GLEW_ARB_pixel_buffer_object); // 2.1 (PBO)
		CHECK_REQ_EXT(GLEW_ARB_framebuffer_object); // 3.0 (FBO)
		CHECK_REQ_EXT(GLEW_ARB_vertex_array_object); // 3.0 (VAO; core in 4.x)
		CHECK_REQ_EXT(GLEW_ARB_uniform_buffer_object); // 3.1 (UBO)

		#ifdef GLEW_ARB_buffer_storage
		CHECK_REQ_EXT(GLEW_ARB_buffer_storage); // 4.4 (immutable storage)
		#endif
		CHECK_REQ_EXT(GLEW_ARB_draw_buffers); // 2.0 (MRT)
		CHECK_REQ_EXT(GLEW_ARB_copy_buffer); // 3.1 (glCopyBufferSubData)
		CHECK_REQ_EXT(GLEW_ARB_map_buffer_range); // 3.0 (glMapBufferRange[ARB])
		CHECK_REQ_EXT(GLEW_EXT_framebuffer_multisample); // 3.0 (multi-sampled FB's)
		CHECK_REQ_EXT(GLEW_EXT_framebuffer_blit); // 3.0 (glBlitFramebuffer[EXT])

		// not yet mandatory
		#ifdef GLEW_ARB_multi_bind
		CHECK_OPT_EXT(GLEW_ARB_multi_bind); // 4.4
		#endif
		CHECK_OPT_EXT(GLEW_ARB_texture_storage); // 4.2
		CHECK_OPT_EXT(GLEW_ARB_program_interface_query); // 4.3
		CHECK_OPT_EXT(GLEW_EXT_direct_state_access); // 3.3 (core in 4.5)
		CHECK_OPT_EXT(GLEW_ARB_invalidate_subdata); // 4.3 (glInvalidateBufferData)
		CHECK_OPT_EXT(GLEW_ARB_shader_storage_buffer_object); // 4.3 (glShaderStorageBlockBinding)
		CHECK_REQ_EXT(GLEW_ARB_get_program_binary); // 4.1 (gl{Get}ProgramBinary)
		CHECK_REQ_EXT(GLEW_ARB_separate_shader_objects); // 4.1 (glProgramParameteri)

		CHECK_REQ_EXT(GLEW_ARB_texture_compression);
		CHECK_REQ_EXT(GLEW_EXT_texture_compression_s3tc);
		CHECK_OPT_EXT(GLEW_EXT_texture_compression_dxt1); // for some reason AMD doesn't list this as supported even though it does
		CHECK_REQ_EXT(GLEW_ARB_texture_float); // 3.0 (FP{16,32} textures)
		CHECK_REQ_EXT(GLEW_ARB_texture_non_power_of_two); // 2.0 (NPOT textures)
		CHECK_REQ_EXT(GLEW_ARB_texture_rectangle); // 3.0 (rectangular textures)
		CHECK_REQ_EXT(GLEW_EXT_texture_filter_anisotropic); // 3.3 (AF; core in 4.6!)
		//CHECK_REQ_EXT(GLEW_ARB_imaging); // 1.2 (imaging subset; texture_*_clamp [GL_CLAMP_TO_EDGE] etc)
		CHECK_OPT_EXT(GLEW_EXT_texture_edge_clamp); // 1.2
		CHECK_OPT_EXT(GLEW_ARB_texture_border_clamp); // 1.3

		CHECK_REQ_EXT(GLEW_EXT_blend_func_separate); // 1.4
		CHECK_REQ_EXT(GLEW_EXT_blend_equation_separate); // 2.0
		CHECK_OPT_EXT(GLEW_EXT_stencil_two_side); // 2.0 (may also be an issue on AMD)

		CHECK_REQ_EXT(GLEW_ARB_occlusion_query); // 1.5
		CHECK_REQ_EXT(GLEW_ARB_occlusion_query2); // 3.3 (glBeginConditionalRender)
		CHECK_REQ_EXT(GLEW_ARB_timer_query); // 3.3

		CHECK_REQ_EXT(GLEW_ARB_depth_clamp); // 3.2

		CHECK_REQ_EXT(GLEW_NV_primitive_restart); // 3.1
		CHECK_REQ_EXT(GLEW_ARB_transform_feedback3); // 4.0 (VTF v3)
		CHECK_REQ_EXT(GLEW_ARB_explicit_attrib_location); // 3.3

		CHECK_REQ_EXT(GLEW_ARB_vertex_program); // VS-ARB
		CHECK_OPT_EXT(GLEW_ARB_fragment_program); // FS-ARB
		CHECK_OPT_EXT(GLEW_ARB_shading_language_100); // 2.0
		CHECK_REQ_EXT(GLEW_ARB_vertex_shader); // 1.5 (VS-GLSL; core in 2.0)
		CHECK_REQ_EXT(GLEW_ARB_fragment_shader); // 1.5 (FS-GLSL; core in 2.0)
		CHECK_REQ_EXT(GLEW_ARB_geometry_shader4); // GS v4 (GL3.2)

		#undef CHECK_OPT_EXT
		#undef CHECK_REQ_EXT

		return true;
	} catch (...) {
	}

	return false;
}



static bool CreateWindowAndContext(const int2& minCtx)
{
	LOG("[%s]", __func__);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);


	if ((sdlWindow = CreateSDLWindow({WIN_SIZE_X, WIN_SIZE_Y})) == nullptr) {
		LOG("[%s] could not create SDL-window", __func__);
		return false;
	}

	// enumerate
	if (minCtx.x < 0) {
		CheckGLContexts();
		return true;
	}

	if ((glContext = CreateGLContext(minCtx)) == nullptr) {
		LOG("[%s] requested minimum OpenGL context (%d.%d) not supported", __func__, minCtx.x, minCtx.y);
		return false;
	}

	{
		glewExperimental = true;

		glewInit();
		glGetError();
	}

	// "program requests >= N, driver returns < N" never happens without explicit fallback paths
	if (!CheckGLContextVersion(minCtx.x * 10 + minCtx.y)) {
		LOG("[%s] requested minimum OpenGL context %d.%d not obtained", __func__, minCtx.x, minCtx.y);
		return false;
	}

	// always require a proper stencil-buffer
	if (!CheckGLStencilBufferBits(8)) {
		LOG("[%s] insufficient OpenGL stencil-buffer support", __func__);
		return false;
	}

	LogVersionInfo(nullptr, nullptr);

	if (CheckGLEWContextVersion(QueryGLContextVersion()))
		return (CheckGLExtensions());

	LOG("[%s] GLEW version outdated", __func__);
	return false;
}

static void DestroyWindowAndContext(SDL_Window* window, SDL_GLContext context)
{
	LOG("[%s]", __func__);

	SDL_GL_MakeCurrent(window, nullptr);
	SDL_DestroyWindow(window);
	SDL_GL_DeleteContext(context);
}



int fakemain(int argc, char** argv)
{
	const int glMajorVersion = (argc > 1)? std::atoi(argv[1]): -1;
	const int glMinorVersion = (argc > 2)? std::atoi(argv[2]):  0;

	LOG("[%s][SDL_Init]", __func__);

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		LOG("[%s] error \"%s\" initializing SDL", __func__, SDL_GetError());
		return 0;
	}

	if (!CheckSDLVideoModes()) {
		LOG("[%s] desktop color-depth should be at least 24 bits per pixel", __func__);
	} else {
		CreateWindowAndContext({glMajorVersion, glMinorVersion});
		DestroyWindowAndContext(sdlWindow, glContext);
	}

	LOG("[%s][SDL_Quit*]", __func__);

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	return 0;
}

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return fakemain(__argc, __argv);
}
#endif



TEST_CASE("CreateContext")
{
	#ifdef WIN32
	// FIXME: SDL_Init triggers SIGABRT on Linux buildbots
	fakemain(0, nullptr);
	#endif
}

