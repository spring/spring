/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <SDL.h>

#if defined(WIN32) && !defined(HEADLESS) && !defined(_MSC_VER)
// for APIENTRY
#include <windef.h>
#endif

#include "myGL.h"
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/TimeProfiler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/CrashHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/Platform/Threading.h"


CONFIG(bool, DisableCrappyGPUWarning).defaultValue(false).description("Disables the warning an user will receive if (s)he attempts to run Spring on an outdated and underpowered video card.");
CONFIG(bool, DebugGL).defaultValue(false).description("Enables _driver_ debug feedback. (see GL_ARB_debug_output)");
CONFIG(bool, DebugGLStacktraces).defaultValue(false).description("Create a stacktrace when an OpenGL error occurs");


static CVertexArray* vertexArray1 = NULL;
static CVertexArray* vertexArray2 = NULL;
static CVertexArray* currentVertexArray = NULL;
//BOOL gmlVertexArrayEnable = 0;
/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
	if (currentVertexArray == vertexArray1){
		currentVertexArray = vertexArray2;
	} else {
		currentVertexArray = vertexArray1;
	}
	return currentVertexArray;
}


/******************************************************************************/

void PrintAvailableResolutions()
{
	std::string modes;

	// Get available fullscreen/hardware modes
	//FIXME this checks only the main screen
	const int nummodes = SDL_GetNumDisplayModes(0);
	for (int i = (nummodes-1); i >= 0; --i) { // reverse order to print them from low to high
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(0, i, &mode);
		if (!modes.empty()) {
			modes += ", ";
		}
		modes += IntToString(mode.w) + "x" + IntToString(mode.h);
	}
	if (nummodes < 1) {
		modes = "NONE";
	}

	LOG("Supported Video modes: %s", modes.c_str());
}

#ifdef GL_ARB_debug_output
#if defined(WIN32) && !defined(HEADLESS)
	#if defined(_MSC_VER) && _MSC_VER >= 1600
		#define _APIENTRY __stdcall
	#else
		#define _APIENTRY APIENTRY
	#endif
#else
	#define _APIENTRY
#endif
void _APIENTRY OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	std::string sourceStr;
	std::string typeStr;
	std::string severityStr;
	std::string messageStr(message, length);

	switch (source) {
		case GL_DEBUG_SOURCE_API_ARB:
			sourceStr = "API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
			sourceStr = "WindowSystem";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
			sourceStr = "Shader";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
			sourceStr = "3rd Party";
			break;
		case GL_DEBUG_SOURCE_APPLICATION_ARB:
			sourceStr = "Application";
			break;
		case GL_DEBUG_SOURCE_OTHER_ARB:
			sourceStr = "other";
			break;
		default:
			sourceStr = "unknown";
	}

	switch (type) {
		case GL_DEBUG_TYPE_ERROR_ARB:
			typeStr = "error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
			typeStr = "deprecated";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
			typeStr = "undefined";
			break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:
			typeStr = "portability";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:
			typeStr = "peformance";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
			typeStr = "other";
			break;
		default:
			typeStr = "unknown";
	}

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH_ARB:
			severityStr = "high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM_ARB:
			severityStr = "medium";
			break;
		case GL_DEBUG_SEVERITY_LOW_ARB:
			severityStr = "low";
			break;
		default:
			severityStr = "unknown";
	}

	LOG_L(L_ERROR, "OpenGL: source<%s> type<%s> id<%u> severity<%s>:\n%s",
			sourceStr.c_str(), typeStr.c_str(), id, severityStr.c_str(),
			messageStr.c_str());

	if (configHandler->GetBool("DebugGLStacktraces")) {
		CrashHandler::Stacktrace(Threading::GetCurrentThread(), "rendering", LOG_LEVEL_WARNING);
	}
}
#endif // GL_ARB_debug_output


static bool GetAvailableVideoRAM(GLint* memory)
{
#if defined(HEADLESS) || !defined(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX) || !defined(GL_TEXTURE_FREE_MEMORY_ATI)
	return false;
#else
	// check free video ram (all values in kB)
	if (GLEW_NVX_gpu_memory_info) {
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &memory[0]);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &memory[1]);
	} else
	if (GLEW_ATI_meminfo) {
		GLint texMemInfo[4];
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, texMemInfo);

		memory[0] = texMemInfo[1];
		memory[1] = texMemInfo[0];
	} else
#ifdef GLX_MESA_query_renderer
	if (GLXEW_MESA_query_renderer) {
		glGetIntegerv(GLX_RENDERER_VIDEO_MEMORY_MESA, &memory[0]);
		memory[1] = texMemInfo[0];
	} else
#endif
	{
		memory[0] = 0;
		memory[1] = memory[0]; // not available
	}

	return true;
#endif
}


static void ShowCrappyGpuWarning(const char* glVendor, const char* glRenderer)
{
#ifdef DEBUG
	{ return; }
#endif

	// Print out warnings for really crappy graphic cards/drivers
	const std::string gfxCardVendor = (glVendor != NULL)? glVendor: "UNKNOWN";
	const std::string gfxCardModel  = (glRenderer != NULL)? glRenderer: "UNKNOWN";
	bool gfxCardIsCrap = false;
	bool msDrivers = false;

	if (gfxCardVendor == "SiS") {
		gfxCardIsCrap = true;
	} else if (gfxCardModel.find("Intel") != std::string::npos) {
		// the vendor does not have to be Intel
		if (gfxCardModel.find(" 945G") != std::string::npos) {
			gfxCardIsCrap = true;
		} else if (gfxCardModel.find(" 915G") != std::string::npos) {
			gfxCardIsCrap = true;
		}
	} else if (gfxCardVendor.find("Microsoft") != std::string::npos) {
		msDrivers = true;
	}

	if (gfxCardIsCrap) {
		LOG_L(L_WARNING, "WW     WWW     WW    AAA     RRRRR   NNN  NN  II  NNN  NN   GGGGG ");
		LOG_L(L_WARNING, " WW   WW WW   WW    AA AA    RR  RR  NNNN NN  II  NNNN NN  GG     ");
		LOG_L(L_WARNING, "  WW WW   WW WW    AAAAAAA   RRRRR   NN NNNN  II  NN NNNN  GG   GG");
		LOG_L(L_WARNING, "   WWW     WWW    AA     AA  RR  RR  NN  NNN  II  NN  NNN   GGGGG ");
		LOG_L(L_WARNING, "(warning)");
		LOG_L(L_WARNING, "Your graphic card is ...");
		LOG_L(L_WARNING, "well, you know ...");
		LOG_L(L_WARNING, "insufficient");
		LOG_L(L_WARNING, "(in case you are not using a horribly wrong driver).");
		LOG_L(L_WARNING, "If the game crashes, looks ugly or runs slow, buy a better card!");
		LOG_L(L_WARNING, ".");
	}
	if (msDrivers) {
		LOG_L(L_WARNING, "WW     WWW     WW    AAA     RRRRR   NNN  NN  II  NNN  NN   GGGGG ");
		LOG_L(L_WARNING, " WW   WW WW   WW    AA AA    RR  RR  NNNN NN  II  NNNN NN  GG     ");
		LOG_L(L_WARNING, "  WW WW   WW WW    AAAAAAA   RRRRR   NN NNNN  II  NN NNNN  GG   GG");
		LOG_L(L_WARNING, "   WWW     WWW    AA     AA  RR  RR  NN  NNN  II  NN  NNN   GGGGG ");
		LOG_L(L_WARNING, "(warning)");
		LOG_L(L_WARNING, "No OpenGL drivers installed.");
		LOG_L(L_WARNING, "Please go to your GPU vendor's website and download their drivers.");
		LOG_L(L_WARNING, ".");
	}

	if (!configHandler->GetBool("DisableCrappyGPUWarning")) {
		if (gfxCardIsCrap) {
			const std::string msg =
				"Warning!\n"
				"Your graphics card is insufficient to play Spring.\n\n"
				"If the game crashes, looks ugly or runs slow, buy a better card!\n"
				"You may try \"spring --safemode\" to test if some of your issues are related to wrong settings.\n"
				"\nHint: You can disable this MessageBox by appending \"DisableCrappyGPUWarning = 1\" to \"" + configHandler->GetConfigFile() + "\".";
			LOG_L(L_WARNING, "%s", msg.c_str());
			Platform::MsgBox(msg, "Warning: Your GPU is not supported", MBF_EXCL);
		} else if (globalRendering->haveMesa) {
			const std::string mesa_msg =
				"Warning!\n"
				"OpenSource graphics card drivers detected.\n"
				"MesaGL/Gallium drivers don't work well with Spring. Try to switch to proprietary drivers.\n\n"
				"You may try \"spring --safemode\".\n"
				"\nHint: You can disable this MessageBox by appending \"DisableCrappyGPUWarning = 1\" to \"" + configHandler->GetConfigFile() + "\".";
			LOG_L(L_WARNING, "%s", mesa_msg.c_str());
			Platform::MsgBox(mesa_msg, "Warning: Your GPU driver is not supported", MBF_EXCL);
		} else if (msDrivers) {
			const std::string mesa_msg =
				"Warning!\n"
				"No OpenGL drivers installed.\n"
				"Please go to your GPU vendor's website and download their drivers:\n"
				" * Nvidia: http://www.nvidia.com\n"
				" * AMD: http://support.amd.com\n"
				" * Intel: http://downloadcenter.intel.com";
			LOG_L(L_WARNING, "%s", mesa_msg.c_str());
			Platform::MsgBox(mesa_msg, "Warning: No OpenGL drivers found", MBF_EXCL);
		}
	}
}


//FIXME move most of this to globalRendering's ctor?
void LoadExtensions()
{
	glewInit();

	SDL_version sdlVersionCompiled;
	SDL_version sdlVersionLinked;

	SDL_VERSION(&sdlVersionCompiled);
	SDL_GetVersion(&sdlVersionLinked);
	const char* glVersion = (const char*) glGetString(GL_VERSION);
	const char* glVendor = (const char*) glGetString(GL_VENDOR);
	const char* glRenderer = (const char*) glGetString(GL_RENDERER);
	const char* glslVersion = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	const char* glewVersion = (const char*) glewGetString(GLEW_VERSION);

	char glVidMemStr[64] = "unknown";
	GLint vidMemBuffer[2] = {0, 0};

	if (GetAvailableVideoRAM(vidMemBuffer)) {
		const GLint totalMemMB = vidMemBuffer[0] / 1024;
		const GLint availMemMB = vidMemBuffer[1] / 1024;

		if (totalMemMB > 0 && availMemMB > 0) {
			const char* memFmtStr = "total %iMB, available %iMB";
			SNPRINTF(glVidMemStr, sizeof(glVidMemStr), memFmtStr, totalMemMB, availMemMB);
		}
	}

	// log some useful version info
	LOG("SDL version:  linked %d.%d.%d; compiled %d.%d.%d", sdlVersionLinked.major, sdlVersionLinked.minor, sdlVersionLinked.patch, sdlVersionCompiled.major, sdlVersionCompiled.minor, sdlVersionCompiled.patch);
	LOG("GL version:   %s", glVersion);
	LOG("GL vendor:    %s", glVendor);
	LOG("GL renderer:  %s", glRenderer);
	LOG("GLSL version: %s", glslVersion);
	LOG("GLEW version: %s", glewVersion);
	LOG("Video RAM:    %s", glVidMemStr);

	ShowCrappyGpuWarning(glVendor, glRenderer);

	std::string missingExts = "";
	if (!GLEW_ARB_multitexture) {
		missingExts += " GL_ARB_multitexture";
	}
	if (!GLEW_ARB_texture_env_combine) {
		missingExts += " GL_ARB_texture_env_combine";
	}
	if (!GLEW_ARB_texture_compression) {
		missingExts += " GL_ARB_texture_compression";
	}

	if (!missingExts.empty()) {
		static const unsigned int errorMsg_maxSize = 2048;
		char errorMsg[errorMsg_maxSize];
		SNPRINTF(errorMsg, errorMsg_maxSize,
				"Needed OpenGL extension(s) not found:\n"
				"  %s\n\n"
				"Update your graphic-card driver!\n"
				"  Graphic card:   %s\n"
				"  OpenGL version: %s\n",
				missingExts.c_str(),
				glRenderer,
				glVersion);
		throw unsupported_error(errorMsg);
	}

	// install OpenGL DebugMessageCallback
#if defined(GL_ARB_debug_output) && !defined(HEADLESS)
	if (GLEW_ARB_debug_output && configHandler->GetBool("DebugGL")) {
		LOG("Installing OpenGL-DebugMessageHandler");
		glDebugMessageCallbackARB(&OpenGLDebugMessageCallback, NULL);

		if (configHandler->GetBool("DebugGLStacktraces")) {
			// The callback should happen in the thread that made the gl call
			// so we get proper stacktraces.
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		}
	}
#endif

	vertexArray1 = new CVertexArray;
	vertexArray2 = new CVertexArray;
}


void UnloadExtensions()
{
	delete vertexArray1;
	delete vertexArray2;
	vertexArray1 = NULL;
	vertexArray2 = NULL;
}

/******************************************************************************/

void WorkaroundATIPointSizeBug()
{
	if (globalRendering->atiHacks && globalRendering->haveGLSL) {
		GLboolean pointSpritesEnabled = false;
		glGetBooleanv(GL_POINT_SPRITE, &pointSpritesEnabled);
		if (pointSpritesEnabled)
			return;

		GLfloat atten[3] = { 1.0f, 0.0f, 0.0f };
		glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);
		glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
	}
}

/******************************************************************************/

void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height)
{
#ifdef GLEW_ARB_texture_storage
	if (levels < 0)
		levels = std::ceil(std::log(std::max(width, height) + 1));

	if (GLEW_ARB_texture_storage) {
		glTexStorage2D(target, levels, internalFormat, width, height);
	} else
#endif
	{
		GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
		switch (internalFormat) {
			case GL_RGBA8: format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
			case GL_RGB8:  format = GL_RGB;  type = GL_UNSIGNED_BYTE; break;
			default: LOG_L(L_ERROR, "[%s] Couldn't detct format& type for %i", __FUNCTION__, internalFormat);
		}
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, nullptr);
	}
}


void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data)
{
	if (globalRendering->compressTextures) {
		switch ( internalFormat ) {
			case 4:
			case GL_RGBA8 :
			case GL_RGBA :  internalFormat = GL_COMPRESSED_RGBA_ARB; break;

			case 3:
			case GL_RGB8 :
			case GL_RGB :   internalFormat = GL_COMPRESSED_RGB_ARB; break;

			case GL_LUMINANCE: internalFormat = GL_COMPRESSED_LUMINANCE_ARB; break;
		}
	}

	// create mipmapped texture

	if (IS_GL_FUNCTION_AVAILABLE(glGenerateMipmap) && !globalRendering->atiHacks) {
		// newest method
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
		if (globalRendering->atiHacks) {
			glEnable(target);
			glGenerateMipmap(target);
			glDisable(target);
		} else {
			glGenerateMipmap(target);
		}
	} else if (GLEW_VERSION_1_4) {
		// This required GL-1.4
		// instead of using glu, we rely on glTexImage2D to create the Mipmaps.
		glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	} else {
		gluBuild2DMipmaps(target, internalFormat, width, height, format, type, data);
	}
}


/******************************************************************************/

void ClearScreen()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity();            // Reset The Projection Matrix
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);  // Select The Modelview Matrix

	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
}


/******************************************************************************/

static unsigned int LoadProgram(GLenum, const char*, const char*);

/**
 * True if the program in DATADIR/shaders/filename is
 * loadable and can run inside our graphics server.
 *
 * @param target glProgramStringARB target: GL_FRAGMENT_PROGRAM_ARB GL_VERTEX_PROGRAM_ARB
 * @param filename Name of the file under shaders with the program in it.
 */

bool ProgramStringIsNative(GLenum target, const char* filename)
{
	// clear any current GL errors so that the following check is valid
	glClearErrors();

	const GLuint tempProg = LoadProgram(target, filename, (target == GL_VERTEX_PROGRAM_ARB? "vertex": "fragment"));

	if (tempProg == 0) {
		return false;
	}

	glSafeDeleteProgram(tempProg);
	return true;
}


/**
 * Presumes the last GL operation was to load a vertex or
 * fragment program.
 *
 * If it was invalid, display an error message about
 * what and where the problem in the program source is.
 *
 * @param filename Only substituted in the message.
 * @param program The program text (used to enhance the message)
 */
static bool CheckParseErrors(GLenum target, const char* filename, const char* program)
{
	GLint errorPos = -1;
	GLint isNative =  0;

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	if (errorPos != -1) {
		const char* fmtString =
			"[%s] shader compilation error at index %d (near "
			"\"%.30s\") when loading %s-program file %s:\n%s";
		const char* tgtString = (target == GL_VERTEX_PROGRAM_ARB)? "vertex": "fragment";
		const char* errString = (const char*) glGetString(GL_PROGRAM_ERROR_STRING_ARB);

		if (errString != NULL) {
			LOG_L(L_ERROR, fmtString, __FUNCTION__, errorPos, program + errorPos, tgtString, filename, errString);
		} else {
			LOG_L(L_ERROR, fmtString, __FUNCTION__, errorPos, program + errorPos, tgtString, filename, "(null)");
		}

		return true;
	}

	if (isNative != 1) {
		GLint aluInstrs, maxAluInstrs;
		GLint texInstrs, maxTexInstrs;
		GLint texIndirs, maxTexIndirs;
		GLint nativeTexIndirs, maxNativeTexIndirs;
		GLint nativeAluInstrs, maxNativeAluInstrs;

		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ALU_INSTRUCTIONS_ARB,            &aluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB,        &maxAluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INSTRUCTIONS_ARB,            &texInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB,        &maxTexInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_TEX_INDIRECTIONS_ARB,            &texIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB,        &maxTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB,     &nativeTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, &maxNativeTexIndirs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB,     &nativeAluInstrs);
		glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, &maxNativeAluInstrs);

		if (aluInstrs > maxAluInstrs) {
			LOG_L(L_ERROR, "[%s] too many ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, aluInstrs, maxAluInstrs);
		}
		if (texInstrs > maxTexInstrs) {
			LOG_L(L_ERROR, "[%s] too many texture instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, texInstrs, maxTexInstrs);
		}
		if (texIndirs > maxTexIndirs) {
			LOG_L(L_ERROR, "[%s] too many texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, texIndirs, maxTexIndirs);
		}
		if (nativeTexIndirs > maxNativeTexIndirs) {
			LOG_L(L_ERROR, "[%s] too many native texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeTexIndirs, maxNativeTexIndirs);
		}
		if (nativeAluInstrs > maxNativeAluInstrs) {
			LOG_L(L_ERROR, "[%s] too many native ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeAluInstrs, maxNativeAluInstrs);
		}

		return true;
	}

	return false;
}


static unsigned int LoadProgram(GLenum target, const char* filename, const char* program_type)
{
	GLuint ret = 0;

	if (!GLEW_ARB_vertex_program) {
		return ret;
	}
	if (target == GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program) {
		return ret;
	}

	CFileHandler file(std::string("shaders/") + filename);
	if (!file.FileExists ()) {
		char c[512];
		SNPRINTF(c, 512, "[myGL::LoadProgram] Cannot find %s-program file '%s'", program_type, filename);
		throw content_error(c);
	}

	int fSize = file.FileSize();
	char* fbuf = new char[fSize + 1];
	file.Read(fbuf, fSize);
	fbuf[fSize] = '\0';

	glGenProgramsARB(1, &ret);
	glBindProgramARB(target, ret);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, fSize, fbuf);

	if (CheckParseErrors(target, filename, fbuf)) {
		ret = 0;
	}

	delete[] fbuf;
	return ret;
}

unsigned int LoadVertexProgram(const char* filename)
{
	return LoadProgram(GL_VERTEX_PROGRAM_ARB, filename, "vertex");
}

unsigned int LoadFragmentProgram(const char* filename)
{

	return LoadProgram(GL_FRAGMENT_PROGRAM_ARB, filename, "fragment");
}


void glSafeDeleteProgram(GLuint program)
{
	if (!GLEW_ARB_vertex_program || (program == 0)) {
		return;
	}
	glDeleteProgramsARB(1, &program);
}


/******************************************************************************/

void glClearErrors()
{
	int safety = 0;
	while ((glGetError() != GL_NO_ERROR) && (safety < 1000)) {
		safety++;
	}
}


/******************************************************************************/

void SetTexGen(const float& scaleX, const float& scaleZ, const float& offsetX, const float& offsetZ)
{
	const GLfloat planeX[] = {scaleX, 0.0f,   0.0f,  offsetX};
	const GLfloat planeZ[] = {  0.0f, 0.0f, scaleZ,  offsetZ};

	//BUG: Nvidia drivers take the current texcoord into account when TexGen is used!
	// You MUST reset the coords before using TexGen!
	//glMultiTexCoord4f(target, 1.0f,1.0f,1.0f,1.0f);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, planeX);
	glEnable(GL_TEXTURE_GEN_S);

	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, planeZ);
	glEnable(GL_TEXTURE_GEN_T);
}

/******************************************************************************/

