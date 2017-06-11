/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <array>
#include <vector>
#include <string>
#include <cmath>

#include <SDL.h>
#if (!defined(HEADLESS) && !defined(WIN32) && !defined(__APPLE__))
// need this for glXQueryCurrentRendererIntegerMESA (glxext)
#include <GL/glxew.h>
#endif

#include "myGL.h"
#include "VertexArray.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/MessageBox.h"
#include "System/type2.h"

#define SDL_BPP(fmt) SDL_BITSPERPIXEL((fmt))

static std::array<CVertexArray, 2> vertexArrays;
static int currentVertexArray = 0;


/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
	currentVertexArray = (currentVertexArray + 1) % vertexArrays.size();
	return &vertexArrays[currentVertexArray];
}


/******************************************************************************/

bool CheckAvailableVideoModes()
{
	// Get available fullscreen/hardware modes
	const int numDisplays = SDL_GetNumVideoDisplays();

	SDL_DisplayMode ddm = {0, 0, 0, 0, nullptr};
	SDL_DisplayMode cdm = {0, 0, 0, 0, nullptr};

	// ddm is virtual, contains all displays in multi-monitor setups
	// for fullscreen windows with non-native resolutions, ddm holds
	// the original screen mode and cdm is the changed mode
	SDL_GetDesktopDisplayMode(0, &ddm);
	SDL_GetCurrentDisplayMode(0, &cdm);

	LOG(
		"[GL::%s] desktop={%ix%ix%ibpp@%iHz} current={%ix%ix%ibpp@%iHz}",
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

			const float r0 = (cm.w *  9.0f) / cm.h;
			const float r1 = (cm.w * 10.0f) / cm.h;
			const float r2 = (cm.w * 16.0f) / cm.h;

			// skip legacy (3:2, 4:3, 5:4, ...) and weird (10:6, ...) ratios
			if (r0 != 16.0f && r1 != 16.0f && r2 != 25.0f)
				continue;
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



#ifndef HEADLESS
static bool GetVideoMemInfoNV(GLint* memInfo)
{
	#if (defined(GLEW_NVX_gpu_memory_info))
	if (!GLEW_NVX_gpu_memory_info)
		return false;

	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &memInfo[0]);
	glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &memInfo[1]);
	return true;
	#else
	return false;
	#endif
}

static bool GetVideoMemInfoATI(GLint* memInfo)
{
	#if (defined(GL_ATI_meminfo) || defined(GLEW_ATI_meminfo))
	if (!GL_ATI_meminfo && !GLEW_ATI_meminfo)
		return false;

	for (uint32_t param: {GL_VBO_FREE_MEMORY_ATI, GL_TEXTURE_FREE_MEMORY_ATI, GL_RENDERBUFFER_FREE_MEMORY_ATI}) {
		glGetIntegerv(param, &memInfo[0]);

		memInfo[4] += (memInfo[0] + memInfo[2]); // total main plus aux. memory free in pool
		memInfo[5] += (memInfo[1] + memInfo[3]); // largest main plus aux. free block in pool
	}

	memInfo[0] = memInfo[4]; // return the VBO/RBO/TEX free sum
	memInfo[1] = memInfo[4]; // sic, just assume total >= free
	return true;
	#else
	return false;
	#endif
}

static bool GetVideoMemInfoMESA(GLint* memInfo)
{
	#if (defined(GLX_MESA_query_renderer))
	if (!GLXEW_MESA_query_renderer)
		return false;

	typedef PFNGLXQUERYCURRENTRENDERERINTEGERMESAPROC QCRIProc;

	static constexpr const GLubyte* qcriProcName = (const GLubyte*) "glXQueryCurrentRendererIntegerMESA";
	static           const QCRIProc qcriProcAddr = (QCRIProc) glXGetProcAddress(qcriProcName);

	if (qcriProcAddr == nullptr)
		return false;

	// note: unlike the others, this value is returned in megabytes
	qcriProcAddr(GLX_RENDERER_VIDEO_MEMORY_MESA, reinterpret_cast<unsigned int*>(&memInfo[0]));

	memInfo[0] *= 1024;
	memInfo[1] = memInfo[0];
	return true;
	#else
	return false;
	#endif
}
#endif

bool GetAvailableVideoRAM(GLint* memory, const char* glVendor)
{
	#ifdef HEADLESS
	return false;
	#else
	GLint memInfo[4 + 2] = {-1, -1, -1, -1, 0, 0};

	switch (glVendor[0]) {
		case 'N': { if (!GetVideoMemInfoNV  (memInfo)) return false; } break; // "NVIDIA"
		case 'A': { if (!GetVideoMemInfoATI (memInfo)) return false; } break; // "ATI" or "AMD"
		case 'X': { if (!GetVideoMemInfoMESA(memInfo)) return false; } break; // "X.org"
		case 'M': { if (!GetVideoMemInfoMESA(memInfo)) return false; } break; // "Mesa"
		case 'I': {                                    return false; } break; // "Intel"
		case 'T': {                                    return false; } break; // "Tungsten"
		case 'V': {                                    return false; } break; // "VMware"
		default : {                                    return false; } break;
	}

	// callers assume [0]=total and [1]=free
	memory[0] = std::max(memInfo[0], memInfo[1]);
	memory[1] = std::min(memInfo[0], memInfo[1]);
	return true;
	#endif
}



bool ShowDriverWarning(const char* glVendor, const char* glRenderer)
{
#ifndef DEBUG
	assert(glVendor != nullptr);
	assert(glRenderer != nullptr);

	// print out warnings for really crappy graphic cards/drivers
	const std::string& gpuVendor = glVendor;
	const std::string& gpuModel  = glRenderer;

	constexpr size_t np = std::string::npos;

	if (gpuVendor.find("Microsoft") != np || gpuVendor.find("unknown") != np) {
		const char* msg =
			"No OpenGL drivers installed on your system. Please visit your\n"
			"GPU vendor's website (listed below) and download these first.\n\n"
			" * Nvidia: http://www.nvidia.com\n"
			" * AMD: http://support.amd.com\n"
			" * Intel: http://downloadcenter.intel.com";

		LOG_L(L_WARNING, "%s", msg);
		Platform::MsgBox(msg, "Warning", MBF_EXCL);
		return false;
	}

	if ((gpuVendor == "SiS") || (gpuModel.find("Intel") != np && (gpuModel.find(" 945G") != np || gpuModel.find(" 915G") != np))) {
		const char* msg =
			"Your graphics card is insufficient to properly run the Spring engine.\n\n"
			"If you experience crashes or poor performance, upgrade to better hardware "
			"or try \"spring --safemode\" to test if your issues are caused by wrong "
			"settings.\n";

		LOG_L(L_WARNING, "%s", msg);
		Platform::MsgBox(msg, "Warning", MBF_EXCL);
		return true;
	}

	if (gpuModel.find(  "mesa ") != np || gpuModel.find("gallium ") != np) {
		const char* msg =
			"You are using an open-source (Mesa / Gallium) graphics card driver, "
			"which may not work well with the Spring engine.\n\nIf you experience "
			"problems, switch to proprietary drivers or try \"spring --safemode\".\n";

		LOG_L(L_WARNING, "%s", msg);
		Platform::MsgBox(msg, "Warning", MBF_EXCL);
		return true;
	}
#endif

	return true;
}


/******************************************************************************/

void WorkaroundATIPointSizeBug()
{
	if (!globalRendering->atiHacks)
		return;
	if (!globalRendering->haveGLSL)
		return;

	GLboolean pointSpritesEnabled = false;
	glGetBooleanv(GL_POINT_SPRITE, &pointSpritesEnabled);
	if (pointSpritesEnabled)
		return;

	GLfloat atten[3] = {1.0f, 0.0f, 0.0f};
	glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);
	glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 1.0f);
}

/******************************************************************************/

void glSaveTexture(const GLuint textureID, const char* filename)
{
	const GLenum target = GL_TEXTURE_2D;
	GLenum format = GL_RGBA8;
	int sizeX, sizeY;

	int bits = 0;
	{
		glBindTexture(GL_TEXTURE_2D, textureID);

		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sizeX);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sizeY);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&format);

		GLint _cbits;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_RED_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_GREEN_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_BLUE_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &_cbits); bits += _cbits;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH_SIZE, &_cbits); bits += _cbits;
	}
	assert(bits == 32);
	assert(format == GL_RGBA8);

	CBitmap bmp;
	bmp.channels = 4;
	bmp.Alloc(sizeX,sizeY);
	glGetTexImage(target,0,GL_RGBA,GL_UNSIGNED_BYTE, &bmp.mem[0]);
	bmp.Save(filename, false);
}


void glSpringBindTextures(GLuint first, GLsizei count, const GLuint* textures)
{
#ifdef GLEW_ARB_multi_bind
	if (GLEW_ARB_multi_bind) {
		glBindTextures(first, count, textures);
	} else
#endif
	{
		for (int i = 0; i < count; ++i) {
			const GLuint texture = (textures == nullptr) ? 0 : textures[i];
			glActiveTexture(GL_TEXTURE0 + first + i);
			glBindTexture(GL_TEXTURE_2D, texture);
		}
		glActiveTexture(GL_TEXTURE0);

	}
}


void glSpringTexStorage2D(const GLenum target, GLint levels, const GLint internalFormat, const GLsizei width, const GLsizei height)
{
#ifdef GLEW_ARB_texture_storage
	if (levels < 0)
		levels = std::ceil(std::log((float)(std::max(width, height) + 1)));

	if (GLEW_ARB_texture_storage) {
		glTexStorage2D(target, levels, internalFormat, width, height);
	} else
#endif
	{
		GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
		switch (internalFormat) {
			case GL_RGBA8: format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
			case GL_RGB8:  format = GL_RGB;  type = GL_UNSIGNED_BYTE; break;
			default: /*LOG_L(L_ERROR, "[%s] Couldn't detect format type for %i", __FUNCTION__, internalFormat);*/
			break;
		}
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, nullptr);
	}
}


void glBuildMipmaps(const GLenum target, GLint internalFormat, const GLsizei width, const GLsizei height, const GLenum format, const GLenum type, const void* data)
{
	if (globalRendering->compressTextures) {
		switch (internalFormat) {
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


void glSpringMatrix2dProj(const int sizex, const int sizey)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0,sizex,0,sizey);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/******************************************************************************/

void ClearScreen()
{
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
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

	if (tempProg == 0)
		return false;

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

		if (aluInstrs > maxAluInstrs)
			LOG_L(L_ERROR, "[%s] too many ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, aluInstrs, maxAluInstrs);

		if (texInstrs > maxTexInstrs)
			LOG_L(L_ERROR, "[%s] too many texture instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, texInstrs, maxTexInstrs);

		if (texIndirs > maxTexIndirs)
			LOG_L(L_ERROR, "[%s] too many texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, texIndirs, maxTexIndirs);

		if (nativeTexIndirs > maxNativeTexIndirs)
			LOG_L(L_ERROR, "[%s] too many native texture indirections in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeTexIndirs, maxNativeTexIndirs);

		if (nativeAluInstrs > maxNativeAluInstrs)
			LOG_L(L_ERROR, "[%s] too many native ALU instructions in %s (%d, max. %d)\n", __FUNCTION__, filename, nativeAluInstrs, maxNativeAluInstrs);

		return true;
	}

	return false;
}


static unsigned int LoadProgram(GLenum target, const char* filename, const char* program_type)
{
	GLuint ret = 0;

	if (!GLEW_ARB_vertex_program)
		return ret;
	if (target == GL_FRAGMENT_PROGRAM_ARB && !GLEW_ARB_fragment_program)
		return ret;

	CFileHandler file(std::string("shaders/") + filename);
	if (!file.FileExists ()) {
		char c[512];
		SNPRINTF(c, 512, "[myGL::LoadProgram] Cannot find %s-program file '%s'", program_type, filename);
		throw content_error(c);
	}

	std::vector<char> fbuf(file.FileSize() + 1);

	file.Read(fbuf.data(), fbuf.size() - 1);
	fbuf.back() = '\0';

	glGenProgramsARB(1, &ret);
	glBindProgramARB(target, ret);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, fbuf.size() - 1, fbuf.data());

	if (CheckParseErrors(target, filename, fbuf.data()))
		ret = 0;

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
	if (!GLEW_ARB_vertex_program || (program == 0))
		return;

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

void SetTexGen(const float scaleX, const float scaleZ, const float offsetX, const float offsetZ)
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
