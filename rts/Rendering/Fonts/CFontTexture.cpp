/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CFontTexture.h"
#include "FontLogSection.h"

#include <cstring> // for memset, memcpy
#include <string>
#include <vector>

#ifndef HEADLESS
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#ifdef USE_FONTCONFIG
		#include <fontconfig/fontconfig.h>
		#include <fontconfig/fcfreetype.h>
	#endif
	#include "LanguageBlocksDefs.h"
#endif // HEADLESS

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/SpringThreading.h"
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "System/UnorderedMap.hpp"
#include "System/float4.h"
#include "System/bitops.h"


#ifndef HEADLESS
	#undef __FTERRORS_H__
	#define FT_ERRORDEF( e, v, s )  { e, s },
	#define FT_ERROR_START_LIST     {
	#define FT_ERROR_END_LIST       { 0, 0 } };
	struct ErrorString {
		int          err_code;
		const char*  err_msg;
	} static errorTable[] =
	#include FT_ERRORS_H


	static const char* GetFTError(FT_Error e) {
		for (int a = 0; errorTable[a].err_msg; ++a) {
			if (errorTable[a].err_code == e)
				return errorTable[a].err_msg;
		}
		return "Unknown error";
	}
#endif // HEADLESS




#ifdef HEADLESS
typedef unsigned char FT_Byte;
#endif

struct SP_Byte { //wrapper to allow usage as shared_ptr
	SP_Byte(size_t size) {
		vec.resize(size);
	}
	FT_Byte* data() {
		return vec.data();
	}
private:
	std::vector<FT_Byte> vec;
};

struct FontFace {
	FontFace(FT_Face f, std::shared_ptr<SP_Byte>& mem) : face(f), memory(mem) { }
	~FontFace() {
	#ifndef HEADLESS
		FT_Done_Face(face);
	#endif
	}
	operator FT_Face() { return this->face; }

	FT_Face face;
	std::shared_ptr<SP_Byte> memory;
};

static spring::unsynced_set<CFontTexture*> allFonts;
static spring::unsynced_map<std::string, std::weak_ptr<FontFace>> fontCache;
static spring::unsynced_map<std::string, std::weak_ptr<SP_Byte>> fontMemCache;
static spring::recursive_mutex m;




#ifndef HEADLESS
class FtLibraryHandler {
public:
	FtLibraryHandler() {
		const FT_Error error = FT_Init_FreeType(&lib);

		if (error)
			throw std::runtime_error(std::string("FT_Init_FreeType failed:") + GetFTError(error));

        #ifdef USE_FONTCONFIG
		if (!FcInit())
			throw std::runtime_error("FontConfig failed");
		#endif
	}

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);
		#ifdef USE_FONTCONFIG
		FcFini();
		#endif
	}

	#ifdef USE_FONTCONFIG
	bool CheckFontConfig() const { return (FcConfigUptoDate(nullptr)); }
	bool BuildFontConfig(const char* osFontsDir) const {
		// Windows users most likely don't have a fontconfig configuration file
		// so manually add windows fonts dir and engine fonts dir to fontconfig
		// so it can use them as fallback.
		FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>(osFontsDir));
		FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>("fonts"));
		FcConfigBuildFonts(nullptr);
		return true;
	}
	#else
	bool CheckFontConfig() const { return false; }
	bool BuildFontConfig(const char*) const { return false; }
	#endif

	static FT_Library& GetLibrary() {
		#ifndef WIN32
		std::call_once(flag, []() {
			singleton.reset(new FtLibraryHandler());
		});

		#else

		std::lock_guard<spring::recursive_mutex> lk(m);
		if (flag) {
			singleton.reset(new FtLibraryHandler());
			flag = false;
		}
		#endif

		return singleton->lib;
	};

private:
	FT_Library lib;

	#ifndef WIN32
	static std::once_flag flag;
	#else
	static bool flag;
	#endif

	static std::unique_ptr<FtLibraryHandler> singleton;
};


#ifndef WIN32
std::once_flag FtLibraryHandler::flag;
#else
bool FtLibraryHandler::flag = true;
#endif

std::unique_ptr<FtLibraryHandler> FtLibraryHandler::singleton = nullptr;
#endif




/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

#ifndef HEADLESS
static inline uint32_t GetKerningHash(char32_t lchar, char32_t rchar)
{
	if (lchar < 128 && rchar < 128) {
		return (lchar << 7) | rchar; // 14bit used
	}
	return (lchar << 16) | rchar; // 32bit used
}


static std::shared_ptr<FontFace> GetFontFace(const std::string& fontfile, const int size)
{
	std::lock_guard<spring::recursive_mutex> lk(m);

	//TODO add support to load fonts by name (needs fontconfig)

	auto it = fontCache.find(fontfile + IntToString(size));
	if (it != fontCache.end() && !it->second.expired())
		return it->second.lock();

	// get the file (no need to cache, takes too less time)
	std::string fontPath(fontfile);
	CFileHandler* f = new CFileHandler(fontPath);
	if (!f->FileExists()) {
		// check in 'fonts/', too
		if (fontPath.substr(0,6) != "fonts/") {
			delete f;
			fontPath = "fonts/" + fontPath;
			f = new CFileHandler(fontPath);
		}

		if (!f->FileExists()) {
			delete f;
			throw content_error("Couldn't find font '" + fontfile + "'.");
		}
	}

	// we need to keep a copy of the memory
	const int filesize = f->FileSize();
	std::weak_ptr<SP_Byte>& fontMemWeak = fontMemCache[fontPath];
	std::shared_ptr<SP_Byte> fontMem = fontMemWeak.lock();
	if (fontMemWeak.expired()) {
		fontMem = std::make_shared<SP_Byte>(SP_Byte(filesize));
		f->Read(fontMem.get()->data(), filesize);
		fontMemWeak = fontMem;
	}
	delete f;

	// load the font
	FT_Face face = NULL;
	FT_Error error = FT_New_Memory_Face(FtLibraryHandler::GetLibrary(), fontMem.get()->data(), filesize, 0, &face);
	auto shFace = std::make_shared<FontFace>(face, fontMem);
	if (error) {
		std::string msg = fontfile + ": FT_New_Face failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	// set render size
	error = FT_Set_Pixel_Sizes(face, 0, size);
	if (error) {
		std::string msg = fontfile + ": FT_Set_Pixel_Sizes failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	// select unicode charmap
	error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if (error) {
		std::string msg = fontfile + ": FT_Select_Charmap failed: ";
		msg += GetFTError(error);
		throw content_error(msg);
	}

	fontCache[fontfile + IntToString(size)] = shFace;
	return shFace;
}
#endif


#ifndef HEADLESS
static std::shared_ptr<FontFace> GetFontForCharacters(const std::vector<char32_t>& characters, const FT_Face origFace, const int origSize)
{
#if defined(USE_FONTCONFIG)
	if (characters.empty())
		return nullptr;

	// create list of wanted characters
	FcCharSet* cset = FcCharSetCreate();
	for (auto c: characters) {
		FcCharSetAddChar(cset, c);
	}

	// create properties of the wanted font
	FcPattern* pattern = FcPatternCreate();
	{
		FcValue v;
		v.type = FcTypeBool;
		v.u.b = FcTrue;
		FcPatternAddWeak(pattern, FC_ANTIALIAS, v, FcFalse);

		FcPatternAddCharSet(pattern, FC_CHARSET, cset);
		FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

		int weight = FC_WEIGHT_NORMAL;
		int slant  = FC_SLANT_ROMAN;
		{
			const FcChar8* ftname = reinterpret_cast<const FcChar8*>("not used");
			FcBlanks* blanks = FcBlanksCreate();
			FcPattern* origPattern = FcFreeTypeQueryFace(origFace, ftname, 0, blanks);
			FcBlanksDestroy(blanks);
			if (origPattern) {
				FcPatternGetInteger(origPattern, FC_WEIGHT, 0, &weight);
				FcPatternGetInteger(origPattern, FC_SLANT,  0, &slant);
				FcPatternDestroy(origPattern);
			}
		}
		FcPatternAddInteger(pattern, FC_WEIGHT, weight);
		FcPatternAddInteger(pattern, FC_SLANT, slant);
	}

	// search fonts that fit our request
	FcResult res;
	FcFontSet* fs = FcFontSort(nullptr, pattern, FcFalse, nullptr, &res);

	// dtors
	auto del = [&](FcFontSet* fs){ FcFontSetDestroy(fs); };
	std::unique_ptr<FcFontSet, decltype(del)> fs_(fs, del);
	FcPatternDestroy(pattern);
	FcCharSetDestroy(cset);

	if (fs == nullptr)
		return nullptr;
	if (res != FcResultMatch)
		return nullptr;

	// iterate returned font list
	for (int i = 0; i < fs->nfont; ++i) {
		FcPattern* font = fs->fonts[i];
		FcChar8* cFilename = nullptr;
		FcResult r = FcPatternGetString(font, FC_FILE, 0, &cFilename);
		if (r != FcResultMatch || cFilename == nullptr) continue;

		const std::string filename = reinterpret_cast<char*>(cFilename);
		try {
			return GetFontFace(filename, origSize);
		} catch(const content_error& ex) {
			LOG_L(L_DEBUG, "%s: %s", filename.c_str(), ex.what());
		}
	}
	return nullptr;
#else
	return nullptr;
#endif
}
#endif


/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/


CFontTexture::CFontTexture(const std::string& fontfile, int size, int _outlinesize, float  _outlineweight)
	: outlineSize(_outlinesize)
	, outlineWeight(_outlineweight)
	, lineHeight(0)
	, fontDescender(0)
	, fontSize(size)
	, texWidth(0)
	, texHeight(0)
	, wantedTexWidth(0)
	, wantedTexHeight(0)
	, texture(0)
	, textureSpaceMatrix(0)
{
	atlasGlyphs.reserve(1024);

	if (fontSize <= 0)
		fontSize = 14;

	static constexpr int FT_INTERNAL_DPI = 64;
	normScale = 1.0f / (fontSize * FT_INTERNAL_DPI);

	fontFamily = "unknown";
	fontStyle  = "unknown";

#ifndef HEADLESS
	lastTextureUpdate = 0;
	face = nullptr;
	shFace = GetFontFace(fontfile, fontSize);
	face = *shFace;

	if (!face)
		return;

	fontFamily = face->family_name;
	fontStyle  = face->style_name;

	fontDescender = normScale * FT_MulFix(face->descender, face->size->metrics.y_scale);
	//lineHeight = FT_MulFix(face->height, face->size->metrics.y_scale); // bad results
	lineHeight = face->height / face->units_per_EM;

	if (lineHeight <= 0)
		lineHeight = 1.25 * (face->bbox.yMax - face->bbox.yMin);

	// has to be done before first GetGlyph() call!
	CreateTexture(32, 32);

	// precache ASCII glyphs & kernings (save them in an array for better lvl2 cpu cache hitrate)
	memset(kerningPrecached, 0, sizeof(kerningPrecached));

	for (char32_t i = 32; i < 127; ++i) {
		const auto& lgl = GetGlyph(i);
		const float advance = lgl.advance;
		for (char32_t j=32; j<127; ++j) {
			const auto& rgl = GetGlyph(j);
			const auto hash = GetKerningHash(i, j);
			FT_Vector kerning;
			FT_Get_Kerning(face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
			kerningPrecached[hash] = advance + normScale * kerning.x;
		}
	}

	allFonts.insert(this);
#endif
}

CFontTexture::~CFontTexture()
{
#ifndef HEADLESS
	allFonts.erase(this);

	glDeleteTextures(1, (const GLuint*)&texture);
	glDeleteLists(textureSpaceMatrix, 1);

	texture = 0;
	textureSpaceMatrix = 0;
#endif
}


void CFontTexture::Update() {
	std::lock_guard<spring::recursive_mutex> lk(m);
	for (auto& font: allFonts) {
		font->UpdateTexture();
	}
}


bool CFontTexture::GenFontConfig() {
	#ifndef HEADLESS
	#ifdef WIN32
	// called only from SpringApp::ParseCmdLine, regular singleton does not exist
	FtLibraryHandler ftLibHandler;
	char osFontsDir[32 * 1024];

	ExpandEnvironmentStrings("%WINDIR%\\fonts", osFontsDir, sizeof(osFontsDir)); // expands %HOME% etc.

	if (ftLibHandler.CheckFontConfig()) {
		printf("[%s] fontconfig for directory %s up to date\n", __func__, osFontsDir);
		return true;
	}

	printf("[%s] creating fontconfig for directory %s\n", __func__, osFontsDir);
	return (ftLibHandler.BuildFontConfig(osFontsDir));
	#endif
	#endif
	return true;
}


const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	const auto it = glyphs.find(ch);
	if (it != glyphs.end())
		return it->second;

	// Get block start pos
	char32_t start, end;
	start = GetLanguageBlock(ch, end);

	// Load an entire block
	LoadBlock(start, end);
	return GetGlyph(ch);
#else
	static GlyphInfo g = GlyphInfo();
	return g;
#endif
}


float CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
#ifndef HEADLESS
	// first check caches
	const uint32_t hash = GetKerningHash(lgl.utf16, rgl.utf16);

	if (hash < (sizeof(kerningPrecached) / sizeof(kerningPrecached[0])))
		return kerningPrecached[hash];

	const auto it = kerningDynamic.find(hash);

	if (it != kerningDynamic.end())
		return it->second;

	if (lgl.face != rgl.face)
		return (kerningDynamic[hash] = lgl.advance);

	// load & cache
	FT_Vector kerning;
	FT_Get_Kerning(lgl.face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
	return (kerningDynamic[hash] = lgl.advance + normScale * kerning.x);
#else
	return 0;
#endif
}


void CFontTexture::LoadBlock(char32_t start, char32_t end)
{
	std::lock_guard<spring::recursive_mutex> lk(m);

	// load glyphs from different fonts (using fontconfig)
	std::shared_ptr<FontFace> f = shFace;

	spring::unsynced_set<std::shared_ptr<FontFace>> alreadyCheckedFonts;

	// generate list of wanted glyphs
	std::vector<char32_t> map(end - start, 0);

	for (char32_t i = start; i < end; ++i)
		map[i - start] = i;

#ifndef HEADLESS
	do {
		alreadyCheckedFonts.insert(f);

		for (auto it = map.begin(); !map.empty() && it != map.end(); ) {
			FT_UInt index = FT_Get_Char_Index(*f, *it);

			if (index != 0) {
				LoadGlyph(f, *it, index);

				*it = map.back();
				map.pop_back();
			} else {
				++it;
			}
		}

		f = GetFontForCharacters(map, *f, fontSize);
		usedFallbackFonts.insert(f);
	} while (!map.empty() && f && (alreadyCheckedFonts.find(f) == alreadyCheckedFonts.end()));
#endif

	// load fail glyph for all remaining ones (they will all share the same fail glyph)
	for (auto c: map) {
		LoadGlyph(shFace, c, 0);
	}


	// read atlasAlloc glyph data back into atlasUpdate{Shadow}
	{
		atlasAlloc.SetNonPowerOfTwo(globalRendering->supportNonPowerOfTwoTex);

		if (!atlasAlloc.Allocate())
			LOG_L(L_WARNING, "Texture limit reached! (try to reduce the font size and/or outlinewidth)");

		wantedTexWidth  = atlasAlloc.GetAtlasSize().x;
		wantedTexHeight = atlasAlloc.GetAtlasSize().y;

		if ((atlasUpdate.xsize != wantedTexWidth) || (atlasUpdate.ysize != wantedTexHeight))
			atlasUpdate = std::move(atlasUpdate.CanvasResize(wantedTexWidth, wantedTexHeight, false));

		if (atlasUpdateShadow.Empty())
			atlasUpdateShadow.Alloc(wantedTexWidth, wantedTexHeight, 1);

		if ((atlasUpdateShadow.xsize != wantedTexWidth) || (atlasUpdateShadow.ysize != wantedTexHeight))
			atlasUpdateShadow = std::move(atlasUpdateShadow.CanvasResize(wantedTexWidth, wantedTexHeight, false));

		for(char32_t i = start; i < end; ++i) {
			const std::string glyphName  = IntToString(i);
			const std::string glyphName2 = glyphName + "sh";

			if (!atlasAlloc.contains(glyphName))
				continue;

			const auto texpos  = atlasAlloc.GetEntry(glyphName);
			const auto texpos2 = atlasAlloc.GetEntry(glyphName2);

			glyphs[i].texCord       = IGlyphRect(texpos [0], texpos [1], texpos [2] - texpos [0], texpos [3] - texpos [1]);
			glyphs[i].shadowTexCord = IGlyphRect(texpos2[0], texpos2[1], texpos2[2] - texpos2[0], texpos2[3] - texpos2[1]);

			const size_t glyphIdx = reinterpret_cast<size_t>(atlasAlloc.GetEntryData(glyphName));

			assert(glyphIdx < atlasGlyphs.size());

			if (texpos[2] != 0)
				atlasUpdate.CopySubImage(atlasGlyphs[glyphIdx], texpos.x, texpos.y);
			if (texpos2[2] != 0)
				atlasUpdateShadow.CopySubImage(atlasGlyphs[glyphIdx], texpos2.x + outlineSize, texpos2.y + outlineSize);
		}

		atlasAlloc.clear();
		atlasGlyphs.clear();
	}

	++curTextureUpdate;
}



void CFontTexture::LoadGlyph(std::shared_ptr<FontFace>& f, char32_t ch, unsigned index)
{
#ifndef HEADLESS
	if (glyphs.find(ch) != glyphs.end())
		return;

	// check for duplicated glyphs
	const auto pred = [&](const std::pair<char32_t, GlyphInfo>& p) { return (p.second.index == index && p.second.face == f->face); };
	const auto iter = std::find_if(glyphs.begin(), glyphs.end(), pred);

	if (iter != glyphs.end()) {
		auto& glyph = glyphs[ch];
		glyph = iter->second;
		glyph.utf16 = ch;
		return;
	}

	auto& glyph = glyphs[ch];
	glyph.face  = f->face;
	glyph.index = index;
	glyph.utf16 = ch;

	// load glyph
	FT_Error error = FT_Load_Glyph(*f, index, FT_LOAD_RENDER);
	if (error) {
		LOG_L(L_ERROR, "Couldn't load glyph %d", ch);
	}

	FT_GlyphSlot slot = f->face->glyph;

	const float xbearing = slot->metrics.horiBearingX * normScale;
	const float ybearing = slot->metrics.horiBearingY * normScale;

	glyph.size.x = xbearing;
	glyph.size.y = ybearing - fontDescender;
	glyph.size.w =  slot->metrics.width * normScale;
	glyph.size.h = -slot->metrics.height * normScale;

	glyph.advance   = slot->advance.x * normScale;
	glyph.height    = slot->metrics.height * normScale;
	glyph.descender = ybearing - glyph.height;

	// workaround bugs in FreeSansBold (in range 0x02B0 - 0x0300)
	if (glyph.advance == 0 && glyph.size.w > 0) glyph.advance = glyph.size.w;

	const int width  = slot->bitmap.width;
	const int height = slot->bitmap.rows;
	const int olSize = 2 * outlineSize;

	if (width<=0 || height<=0) {
		return;
	}

	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
		LOG_L(L_ERROR, "invalid pixeldata mode");
		return;
	}

	if (slot->bitmap.pitch != width) {
		LOG_L(L_ERROR, "invalid pitch");
		return;
	}

	// store glyph bitmap (index) in allocator until the next LoadBlock call
	atlasGlyphs.emplace_back(slot->bitmap.buffer, width, height, 1);

	atlasAlloc.AddEntry(IntToString(ch)       , int2(width         , height         ), reinterpret_cast<void*>(atlasGlyphs.size() - 1));
	atlasAlloc.AddEntry(IntToString(ch) + "sh", int2(width + olSize, height + olSize)                                                 );
#endif
}


void CFontTexture::CreateTexture(const int width, const int height)
{
#ifndef HEADLESS
	glPushAttrib(GL_TEXTURE_BIT);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if (GLEW_ARB_texture_border_clamp) {
		constexpr GLfloat borderColor[4] = {1.0f, 1.0f, 1.0f, 0.0f};

		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
	glPopAttrib();

	textureSpaceMatrix = glGenLists(1);
	glNewList(textureSpaceMatrix, GL_COMPILE);
	glEndList();

	atlasUpdate = {};
	atlasUpdate.Alloc(texWidth = wantedTexWidth = width, texHeight = wantedTexHeight = height, 1);

	atlasUpdateShadow = {};
	atlasUpdateShadow.Alloc(width, height, 1);
#endif
}

void CFontTexture::ReallocAtlases(bool pre)
{
#ifndef HEADLESS
	assert(!atlasUpdate.Empty());


	static std::vector<uint8_t> atlasMem;
	static std::vector<uint8_t> atlasShadowMem;

	if (pre) {
		atlasMem.clear();
		atlasMem.resize(atlasUpdate.GetMemSize());

		atlasShadowMem.clear();
		atlasShadowMem.resize(atlasUpdateShadow.GetMemSize());

		memcpy(atlasMem.data(), atlasUpdate.GetRawMem(), atlasUpdate.GetMemSize());
		memcpy(atlasShadowMem.data(), atlasUpdateShadow.GetRawMem(), atlasUpdateShadow.GetMemSize());

		atlasUpdate = {};
		atlasUpdateShadow = {};
		return;
	}


	const int xsize = atlasUpdate.xsize;
	const int ysize = atlasUpdate.ysize;

	// NB: pool has already been wiped here, do not return memory to it but just realloc
	atlasUpdate.Alloc(xsize, ysize, 1);
	atlasUpdateShadow.Alloc(xsize, ysize, 1);

	memcpy(atlasUpdate.GetRawMem(), atlasMem.data(), atlasMem.size());
	memcpy(atlasUpdateShadow.GetRawMem(), atlasShadowMem.data(), atlasShadowMem.size());


	if (atlasGlyphs.empty())
		return;

	LOG_L(L_WARNING, "[FontTexture::%s] discarding %u glyph bitmaps", __func__, uint32_t(atlasGlyphs.size()));

	// should be empty, but realloc glyphs just in case so we can safely dispose of them
	for (CBitmap& bmp: atlasGlyphs) {
		bmp.Alloc(1, 1, 1);
	}

	atlasGlyphs.clear();
#endif
}


void CFontTexture::UpdateTexture()
{
#ifndef HEADLESS
	std::lock_guard<spring::recursive_mutex> lk(m);

	if (curTextureUpdate == lastTextureUpdate)
		return;

	lastTextureUpdate = curTextureUpdate;
	texWidth  = wantedTexWidth;
	texHeight = wantedTexHeight;

	// merge shadow and regular atlas bitmaps, dispose shadow
	if (atlasUpdateShadow.xsize == atlasUpdate.xsize && atlasUpdateShadow.ysize == atlasUpdate.ysize) {
		atlasUpdateShadow.Blur(outlineSize, outlineWeight);
		assert((atlasUpdate.xsize * atlasUpdate.ysize) % sizeof(int) == 0);

		const int* src = reinterpret_cast<const int*>(atlasUpdateShadow.GetRawMem());
		      int* dst = reinterpret_cast<      int*>(atlasUpdate.GetRawMem());

		const int size = (atlasUpdate.xsize * atlasUpdate.ysize) / sizeof(int);

		assert(atlasUpdateShadow.GetMemSize() / sizeof(int) == size);
		assert(atlasUpdate.GetMemSize() / sizeof(int) == size);

		for (int i = 0; i < size; ++i) {
			dst[i] |= src[i];
		}

		atlasUpdateShadow = {};
	}


	glPushAttrib(GL_PIXEL_MODE_BIT | GL_TEXTURE_BIT);
		// update texture atlas
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texWidth, texHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());

		// update texture space dlist (this affects already compiled dlists too!)
		glNewList(textureSpaceMatrix, GL_COMPILE);
		glScalef(1.0f / texWidth, 1.0f / texHeight, 1.0f);
		glEndList();
	glPopAttrib();
#endif
}
