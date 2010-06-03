/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <string>
#include <ostream>
#include <fstream>
#include <SDL.h>
#include "mmgr.h"

#include "myGL.h"
#include "Rendering/glFont.h"
#include "VertexArray.h"
#include "VertexArrayRange.h"
#include "FileSystem/FileHandler.h"
#include "Game/GameVersion.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "Platform/errorhandler.h"
#include "ConfigHandler.h"
#include "LogOutput.h"
#include "FPUCheck.h"
#include "GlobalUnsynced.h"
#include "Util.h"
#include "Exceptions.h"

#include "FBO.h"

using namespace std;


static CVertexArray* vertexArray1 = NULL;
static CVertexArray* vertexArray2 = NULL;
static CVertexArray* currentVertexArray = NULL;

static GLuint     startupTexture = 0;
static float      startupTexture_aspectRatio = 1.0; // x/y
// make this var configurable if we need streching load-screens
static const bool startupTexture_keepAspectRatio = true;

#ifdef USE_GML
static CVertexArray vertexArrays1[GML_MAX_NUM_THREADS];
static CVertexArray vertexArrays2[GML_MAX_NUM_THREADS];
static CVertexArray* currentVertexArrays[GML_MAX_NUM_THREADS];
#endif
//BOOL gmlVertexArrayEnable=0;
/******************************************************************************/
/******************************************************************************/

CVertexArray* GetVertexArray()
{
#ifdef USE_GML // each thread gets its own array to avoid conflicts
	int thread=gmlThreadNumber;
	if(currentVertexArrays[thread]==&vertexArrays1[thread]){
		currentVertexArrays[thread]=&vertexArrays2[thread];
	} else {
		currentVertexArrays[thread]=&vertexArrays1[thread];
	}
	return currentVertexArrays[thread];
#else
	if(currentVertexArray==vertexArray1){
		currentVertexArray=vertexArray2;
	} else {
		currentVertexArray=vertexArray1;
	}
	return currentVertexArray;
#endif
}


/******************************************************************************/

void PrintAvailableResolutions()
{
	// Get available fullscreen/hardware modes
	SDL_Rect **modes=SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);
	if (modes == (SDL_Rect **)0) {
		logOutput.Print("Supported Video modes: No modes available!\n");
	}else if (modes == (SDL_Rect **)-1) {
		logOutput.Print("Supported Video modes: All modes available.\n");
	}else{
		char buffer[1024];
		unsigned char n = 0;
		for(int i=0;modes[i];++i) {
			n += SNPRINTF(&buffer[n], 1024-n, "%dx%d, ", modes[i]->w, modes[i]->h);
		}
		// remove last comma
		if (n>=2) {
			buffer[n-2] = NULL;
		}
		logOutput.Print("Supported Video modes: %s\n",buffer);
	}
}
  
  
void LoadExtensions()
{
	glewInit();

	// log some useful version info
	logOutput.Print("SDL:  %d.%d.%d\n",
		SDL_Linked_Version()->major,
		SDL_Linked_Version()->minor,
		SDL_Linked_Version()->patch);
	logOutput.Print("GL:   %s\n", glGetString(GL_VERSION));
	logOutput.Print("GL:   %s\n", glGetString(GL_VENDOR));
	logOutput.Print("GL:   %s\n", glGetString(GL_RENDERER));
	logOutput.Print("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	logOutput.Print("GLEW: %s\n", glewGetString(GLEW_VERSION));

#if       !defined DEBUG
	// Print out warnings for really crappy graphic cards/drivers
	{
		const std::string gfxCard_vendor = (const char*) glGetString(GL_VENDOR);
		const std::string gfxCard_model  = (const char*) glGetString(GL_RENDERER);
		bool gfxCard_isWorthATry = true;
		if (gfxCard_vendor == "SiS") {
			gfxCard_isWorthATry = false;
		} else if (gfxCard_model.find("Intel") != std::string::npos) {
			// the vendor does not have to be Intel
			if (gfxCard_model.find(" 945G") != std::string::npos) {
				gfxCard_isWorthATry = false;
			} else if (gfxCard_model.find(" 915G") != std::string::npos) {
				gfxCard_isWorthATry = false;
			}
		}

		if (!gfxCard_isWorthATry) {
			logOutput.Print("o_O\n");
			logOutput.Print("WW     WWW     WW    AAA     RRRRR   NNN  NN  II  NNN  NN   GGGGG  \n");
			logOutput.Print(" WW   WW WW   WW    AA AA    RR  RR  NNNN NN  II  NNNN NN  GG      \n");
			logOutput.Print("  WW WW   WW WW    AAAAAAA   RRRRR   NN NNNN  II  NN NNNN  GG   GG \n");
			logOutput.Print("   WWW     WWW    AA     AA  RR  RR  NN  NNN  II  NN  NNN   GGGGG  \n");
			logOutput.Print("(warning)\n");
			logOutput.Print("Your graphic card is ...\n");
			logOutput.Print("well, you know ...\n");
			logOutput.Print("insufficient\n");
			logOutput.Print("(in case you are not using a horribly wrong driver).\n");
			logOutput.Print("If the game crashes, looks ugly or runs slow, buy a better card!\n");
			logOutput.Print(".\n");
		}
	}
#endif // !defined DEBUG

	std::string s = (char*)glGetString(GL_EXTENSIONS);
	for (unsigned int i=0; i<s.length(); i++)
		if (s[i]==' ') s[i]='\n';

	std::string missingExts = "";
	if(!GLEW_ARB_multitexture) {
		missingExts += " GL_ARB_multitexture";
	}
	if(!GLEW_ARB_texture_env_combine) {
		missingExts += " GL_ARB_texture_env_combine";
	}
	if(!GLEW_ARB_texture_compression) {
		missingExts += " GL_ARB_texture_compression";
	}

	if(!missingExts.empty()) {
		static const unsigned int errorMsg_maxSize = 2048;
		char errorMsg[errorMsg_maxSize];
		SNPRINTF(errorMsg, errorMsg_maxSize,
				"Needed OpenGL extension(s) not found:\n"
				"%s\n"
				"Update your graphic-card driver!\n"
				"graphic card:   %s\n"
				"OpenGL version: %s\n",
				missingExts.c_str(), (const char*)glGetString(GL_RENDERER),
				(const char*)glGetString(GL_RENDERER));
		handleerror(0, errorMsg, "Update graphic drivers", 0);
		exit(0);
	}

	vertexArray1 = new CVertexArray;
	vertexArray2 = new CVertexArray;
}


void UnloadExtensions()
{
	delete vertexArray1;
	delete vertexArray2;
}


/******************************************************************************/

void glBuildMipmaps(const GLenum target,GLint internalFormat,const GLsizei width,const GLsizei height,const GLenum format,const GLenum type,const void *data)
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

	if (glGenerateMipmapEXT_NONGML && !globalRendering->atiHacks) {
		// newest method
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
		if (globalRendering->atiHacks) {
			glEnable(target);
			glGenerateMipmapEXT(target);
			glDisable(target);
		}else{
			glGenerateMipmapEXT(target);
		}
	}else if (GLEW_VERSION_1_4) {
		// This required GL-1.4
		// instead of using glu, we rely on glTexImage2D to create the Mipmaps.
		glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	} else
		gluBuild2DMipmaps(target, internalFormat, width, height, format, type, data);
}

/******************************************************************************/

static void AppendStringVec(vector<string>& dst, const vector<string>& src)
{
	for (int i = 0; i < (int)src.size(); i++) {
		dst.push_back(src[i]);
	}
}


static string SelectPicture(const std::string& dir, const std::string& prefix)
{
	vector<string> pics;

	AppendStringVec(pics, CFileHandler::FindFiles(dir, prefix + "*.bmp"));
	AppendStringVec(pics, CFileHandler::FindFiles(dir, prefix + "*.jpg"));

	// add 'allside_' pictures if we have a prefix
	if (!prefix.empty()) {
		AppendStringVec(pics, CFileHandler::FindFiles(dir, "allside_*.bmp"));
		AppendStringVec(pics, CFileHandler::FindFiles(dir, "allside_*.jpg"));
	}

	if (pics.empty()) {
		return "";
	}

	return pics[gu->usRandInt() % pics.size()];
}

void RandomStartPicture(const std::string& sidePref)
{
	if (startupTexture)
		return;
	const string picDir = "bitmaps/loadpictures/";

	string name = "";
	if (!sidePref.empty()) {
		name = SelectPicture(picDir, sidePref + "_");
	}
	if (name.empty()) {
		name = SelectPicture(picDir, "");
	}
	if (name.empty()) {
		return; // no valid pictures
	}
	LoadStartPicture(name);
}

void LoadStartPicture(const std::string& name)
{
	CBitmap bm;
	if (!bm.Load(name)) {
		throw content_error("Could not load startpicture from file " + name);
	}

	startupTexture_aspectRatio = (float) bm.xsize / bm.ysize;

	// HACK Really big load pictures made GLU choke.
	if ((bm.xsize > globalRendering->viewSizeX) || (bm.ysize > globalRendering->viewSizeY)) {
		float newX = globalRendering->viewSizeX;
		float newY = globalRendering->viewSizeY;

		if (startupTexture_keepAspectRatio) {
			// Make smaller but preserve aspect ratio.
			// The resulting resolution will make it fill one axis of the
			// screen, and be smaller or equal to the screen on the other axis.
			const float screen_aspectRatio = globalRendering->aspectRatio;
			const float ratioComp = screen_aspectRatio / startupTexture_aspectRatio;
			if (ratioComp > 1.0f) {
				newX = newX / ratioComp;
			} else {
				newY = newY * ratioComp;
			}
		}

		bm = bm.CreateRescaled((int) newX, (int) newY);
	}

	startupTexture = bm.CreateTexture(false);
}

void UnloadStartPicture()
{
	if (startupTexture) {
		glDeleteTextures(1, &startupTexture);
	}
	startupTexture = 0;
}


/******************************************************************************/

void ClearScreen()
{
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity();            // Reset The Projection Matrix
	gluOrtho2D(0,1,0,1);
	glMatrixMode(GL_MODELVIEW);  // Select The Modelview Matrix

	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
}

void PrintLoadMsg(const char* text, bool swapbuffers)
{
	static char prevText[100];

	// Stuff that needs to be done regularly while loading.
	// Totally unrelated to the task the name of this function implies.

	// Check to prevent infolog spam by CPreGame which uses this function
	// to render the screen background each frame.
	if (strcmp(prevText, text)) {
		logOutput.Print("%s",text);
		strncpy(prevText, text, sizeof(prevText));
		prevText[sizeof(prevText) - 1] = 0;
	}

	good_fpu_control_registers(text);

	float xDiv = 0.0f;
	float yDiv = 0.0f;
	if (startupTexture_keepAspectRatio) {
		const float screen_aspectRatio = globalRendering->aspectRatio;
		const float ratioComp = screen_aspectRatio / startupTexture_aspectRatio;
		if ((ratioComp > 0.99f) && (ratioComp < 1.01f)) { // ~= 1
			// show Load-Screen full screen
			// nothing to do
		} else if (ratioComp > 1.0f) {
			// show Load-Screen on part of the screens X-Axis only
			xDiv = (1 - (1 / ratioComp)) / 2;
		} else {
			// show Load-Screen on part of the screens Y-Axis only
			yDiv = (1 - ratioComp) / 2;
		}
	}

	// Draw loading screen & print load msg.
	ClearScreen();
	if (startupTexture) {
		glBindTexture(GL_TEXTURE_2D,startupTexture);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f + xDiv, 0.0f + yDiv);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f + xDiv, 1.0f - yDiv);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f - xDiv, 1.0f - yDiv);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f - xDiv, 0.0f + yDiv);
		glEnd();
	}

	font->SetOutlineColor(0.0f,0.0f,0.0f,0.65f);
	font->SetTextColor(1.0f,1.0f,1.0f,1.0f);

	font->glPrint(0.5f,0.5f,   globalRendering->viewSizeY / 15.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			text);
#ifdef USE_GML
	font->glFormat(0.5f,0.06f, globalRendering->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"Spring %s (%d threads)", SpringVersion::GetFull().c_str(), gmlThreadCount);
#else
	font->glFormat(0.5f,0.06f, globalRendering->viewSizeY / 35.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"Spring %s", SpringVersion::GetFull().c_str());
#endif
	font->glFormat(0.5f,0.02f, globalRendering->viewSizeY / 50.0f, FONT_OUTLINE | FONT_CENTER | FONT_NORM,
			"This program is distributed under the GNU General Public License, see license.html for more info");

	if (swapbuffers) {
		SDL_GL_SwapBuffers();
	}
}





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

	GLuint tempProg = LoadProgram(target, filename, (target == GL_VERTEX_PROGRAM_ARB? "vertex": "fragment"));

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
	if (glGetError() != GL_INVALID_OPERATION)
		return false;

	// Find the error position
	GLint errorPos = -1;
	GLint isNative =  0;

	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
	glGetProgramivARB(target, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &isNative);

	// Print implementation-dependent program
	// errors and warnings string.
	const GLubyte* errString = glGetString(GL_PROGRAM_ERROR_STRING_ARB);

	logOutput.Print(
		"[myGL::CheckParseErrors] Shader compilation error at index "
		"%d (near \"%.30s\") when loading %s-program file %s:\n%s",
		errorPos, program + errorPos,
		(target == GL_VERTEX_PROGRAM_ARB? "vertex": "fragment"),
		filename, (const char*) errString
	);

	return true;
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

	char* fbuf = new char[file.FileSize()];
	file.Read(fbuf, file.FileSize());

	glGenProgramsARB(1, &ret);
	glBindProgramARB(target, ret);
	glProgramStringARB(target, GL_PROGRAM_FORMAT_ASCII_ARB, file.FileSize(), fbuf);

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

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_S, GL_EYE_PLANE, planeX);
	glEnable(GL_TEXTURE_GEN_S);

	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGenfv(GL_T, GL_EYE_PLANE, planeZ);
	glEnable(GL_TEXTURE_GEN_T);
}

/******************************************************************************/

