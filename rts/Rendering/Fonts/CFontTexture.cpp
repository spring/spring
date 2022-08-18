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
#endif // HEADLESS

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"
#ifdef _DEBUG
	#include "System/Platform/Threading.h"
#endif
#include "System/SafeUtil.h"
#include "System/StringUtil.h"
#include "System/TimeProfiler.h"
#include "System/UnorderedMap.hpp"
#include "System/float4.h"
#include "System/bitops.h"
#include "System/ContainerUtil.h"
#include "System/ScopedResource.h"
#include "fmt/format.h"
#include "fmt/printf.h"


#ifndef HEADLESS
	#undef __FTERRORS_H__
	#define FT_ERRORDEF( e, v, s )  { e, s },
	#define FT_ERROR_START_LIST     {
	#define FT_ERROR_END_LIST       { 0, 0 } };
	struct FTErrorRecord {
		int          err_code;
		const char*  err_msg;
	} static errorTable[] =
	#include FT_ERRORS_H

	struct IgnoreMe {}; // MSVC IntelliSense is confused by #include FT_ERRORS_H above. This seems to fix it.

	static const char* GetFTError(FT_Error e) {
		auto it = std::find_if(std::begin(errorTable), std::end(errorTable), [e](FTErrorRecord er) { return er.err_code == e; });
		if (it != std::end(errorTable))
			return it->err_msg;

		return "Unknown error";
	}
#endif // HEADLESS





#ifdef HEADLESS
typedef unsigned char FT_Byte;
#endif


static spring::unordered_map<std::string, std::weak_ptr<FontFace>> fontFaceCache;
static spring::unordered_map<std::string, std::weak_ptr<FontFileBytes>> fontMemCache;
static spring::unordered_set<std::pair<std::string, int>, spring::synced_hash<std::pair<std::string, int>>> invalidFonts;
static std::array<std::unique_ptr<spring::mutex_wrapper_concept>, 2> cacheMutexes = {
	std::make_unique<spring::mutex_wrapper<spring::noop_mutex     >>(),
	std::make_unique<spring::mutex_wrapper<spring::recursive_mutex>>()
};

spring::mutex_wrapper_concept* GetCacheMutex() { return cacheMutexes[CFontTexture::threadSafety].get(); }


#ifndef HEADLESS
class FtLibraryHandler {
public:
	FtLibraryHandler() {
		std::string msg;
		std::string err;

		{
			const FT_Error error = FT_Init_FreeType(&lib);

			FT_Int version[3];
			FT_Library_Version(lib, &version[0], &version[1], &version[2]);

			msg = fmt::sprintf("%s::FreeTypeInit (version %d.%d.%d)", __func__, version[0], version[1], version[2]);
			err = fmt::sprintf("[%s] FT_Init_FreeType failure \"%s\"", __func__, GetFTError(error));

			if (error != 0)
				throw std::runtime_error(err);
		}

        #ifdef USE_FONTCONFIG
		if (!UseFontConfig())
			return;

		msg = fmt::sprintf("%s::FontConfigInit (version %d.%d.%d)", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);
		err = fmt::sprintf("[%s] FcInit failure (version %d.%d.%d)", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);

		{
			ScopedOnceTimer timer(msg);

			if (FcInit()) {
				FtLibraryHandler::CheckGenFontConfigFast();
				return;
			}

			throw std::runtime_error(err);
		}
		#endif
	}

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);

		#ifdef USE_FONTCONFIG
		if (!UseFontConfig())
			return;

		FcFini();
		#endif
	}

	//reduced set of fonts
	static bool CheckGenFontConfigFast() {
		FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>("fonts"));

		if (!FtLibraryHandler::CheckFontConfig()) {
			return FcConfigBuildFonts(nullptr);
		}

		return true;
	}

	static bool CheckGenFontConfigFull(bool console) {
	#ifndef HEADLESS
		char osFontsDir[8192];

		#ifdef _WIN32
			ExpandEnvironmentStrings("%WINDIR%\\fonts", osFontsDir, sizeof(osFontsDir)); // expands %HOME% etc.
		#else
			strncpy(osFontsDir, "/etc/fonts/", sizeof(osFontsDir));
		#endif

		FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>("fonts"));
		FcConfigAppFontAddDir(nullptr, reinterpret_cast<const FcChar8*>(osFontsDir));

		{
			auto dirs = FcConfigGetCacheDirs(nullptr);
			FcStrListFirst(dirs);
			for (FcChar8* dir = FcStrListNext(dirs), *prevDir = nullptr; dir != nullptr && dir != prevDir; ) {
				prevDir = dir;
				if (console)
					printf("[%s] Using Fontconfig cache dir \"%s\"\n", __func__, dir);
				else
					LOG("[%s] Using Fontconfig cache dir \"%s\"", __func__, dir);
			}
			FcStrListDone(dirs);
		}


		if (FtLibraryHandler::CheckFontConfig()) {
			if (console)
				printf("[%s] fontconfig for directory \"%s\" up to date\n", __func__, osFontsDir);
			else
				LOG("[%s] fontconfig for directory \"%s\" up to date", __func__, osFontsDir);
			return true;
		}

		if (console)
			printf("[%s] creating fontconfig for directory \"%s\"\n", __func__, osFontsDir);
		else
			LOG("[%s] creating fontconfig for directory \"%s\"", __func__, osFontsDir);

		return FcConfigBuildFonts(nullptr);
	#endif

		return true;
	}

	static bool UseFontConfig() { return (configHandler == nullptr || configHandler->GetBool("UseFontConfigLib")); }

	#ifdef USE_FONTCONFIG
	// command-line CheckGenFontConfigFull invocation checks
	static bool CheckFontConfig() { return (UseFontConfig() && FcConfigUptoDate(nullptr)); }
	#else

	static bool CheckFontConfig() { return false; }
	static bool CheckGenFontConfig(bool fromCons) { return false; }
	#endif

	static FT_Library& GetLibrary() {
		static FtLibraryHandler singleton;

		return singleton.lib;
	};

private:
	FT_Library lib;
};
#endif



void FtLibraryHandlerProxy::InitFtLibrary()
{
#ifndef HEADLESS
	FtLibraryHandler::GetLibrary();
#endif
}

bool FtLibraryHandlerProxy::CheckGenFontConfigFull(bool console)
{
#ifndef HEADLESS
	return FtLibraryHandler::CheckGenFontConfigFull(console);
#else
	return false;
#endif

}



/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/
/*******************************************************************************/

#ifndef HEADLESS
static inline uint64_t GetKerningHash(char32_t lchar, char32_t rchar)
{
	if (lchar < 128 && rchar < 128)
		return (lchar << 7) | rchar; // 14bit used

	return (static_cast<uint64_t>(lchar) << 32) | static_cast<uint64_t>(rchar); // 64bit used
}

static std::shared_ptr<FontFace> GetFontFace(const std::string& fontfile, const int size)
{
	assert(CFontTexture::threadSafety || Threading::IsMainThread());
	std::lock_guard lk(*GetCacheMutex());

	//TODO add support to load fonts by name (needs fontconfig)

	const auto fontKey = fontfile + IntToString(size);
	const auto fontIt = fontFaceCache.find(fontKey);

	if (fontIt != fontFaceCache.end() && !fontIt->second.expired())
		return fontIt->second.lock();

	// get the file (no need to cache, takes too little time)
	std::string fontPath(fontfile);
	CFileHandler f(fontPath);

	if (!f.FileExists()) {
		// check in 'fonts/', too
		if (fontPath.substr(0, 6) != "fonts/") {
			f.Close();
			f.Open(fontPath = "fonts/" + fontPath);
		}

		if (!f.FileExists())
			throw content_error("Couldn't find font '" + fontfile + "'.");
	}

	// we need to keep a copy of the memory
	const int filesize = f.FileSize();

	std::weak_ptr<FontFileBytes>& fontMemWeak = fontMemCache[fontPath];
	std::shared_ptr<FontFileBytes> fontMem = fontMemWeak.lock();

	if (fontMemWeak.expired()) {
		fontMem = std::make_shared<FontFileBytes>(FontFileBytes(filesize));
		f.Read(fontMem.get()->data(), filesize);
		fontMemWeak = fontMem;
	}

	// load the font
	FT_Face face_ = nullptr;
	FT_Error error = FT_New_Memory_Face(FtLibraryHandler::GetLibrary(), fontMem.get()->data(), filesize, 0, &face_);
	auto face = spring::ScopedResource(
		face_,
		[](FT_Face f) { if (f) FT_Done_Face(f); }
	);

	if (error != 0) {
		throw content_error(fmt::format("FT_New_Face failed: {}", GetFTError(error)));
	}

	// set render size
	if ((error = FT_Set_Pixel_Sizes(face, 0, size)) != 0) {
		throw content_error(fmt::format("FT_Set_Pixel_Sizes failed: {}", GetFTError(error)));
	}

	// select unicode charmap
	if ((error = FT_Select_Charmap(face, FT_ENCODING_UNICODE)) != 0) {
		throw content_error(fmt::format("FT_Select_Charmap failed: {}", GetFTError(error)));
	}

	return (fontFaceCache[fontKey] = std::make_shared<FontFace>(face.Release(), fontMem)).lock();
}
#endif



#ifndef HEADLESS

inline
static std::string GetFaceKey(FT_Face f)
{
	FT_FaceRec_* fr = static_cast<FT_FaceRec_*>(f);
	return fmt::format("{}-{}-{}", fr->family_name, fr->style_name, fr->num_glyphs);
}

// NOLINTNEXTLINE{misc-misplaced-const}
template<typename USET>
static std::shared_ptr<FontFace> GetFontForCharacters(const std::vector<char32_t>& characters, const FT_Face origFace, const int origSize, const USET& blackList)
{
#if defined(USE_FONTCONFIG)
	if (characters.empty())
		return nullptr;

	// create list of wanted characters
	auto cset = spring::ScopedResource(
		FcCharSetCreate(),
		[](FcCharSet* cs) { if (cs) FcCharSetDestroy(cs); }
	);

	for (auto c: characters) {
		FcCharSetAddChar(cset, c);
	}

	// create properties of the wanted font
	auto pattern = spring::ScopedResource(
		FcPatternCreate(),
		[](FcPattern* p) { if (p) FcPatternDestroy(p); }
	);

	{
		{
			FcValue v;
			v.type = FcTypeBool;
			v.u.b = FcTrue;
			FcPatternAddWeak(pattern, FC_ANTIALIAS, v, FcFalse);
		}

		FcPatternAddCharSet(pattern, FC_CHARSET , cset);
		FcPatternAddBool(   pattern, FC_SCALABLE, FcTrue);
		FcPatternAddDouble( pattern, FC_SIZE    , static_cast<double>(origSize));

		int weight = FC_WEIGHT_NORMAL;
		int slant  = FC_SLANT_ROMAN;
		FcBool outline = FcFalse;

		FcChar8* family = nullptr;
		FcChar8* foundry = nullptr;

		const FcChar8* ftname = reinterpret_cast<const FcChar8*>("not used");

		auto blanks = spring::ScopedResource(
			FcBlanksCreate(),
			[](FcBlanks* b) { if (b) FcBlanksDestroy(b); }
		);

		auto origPattern = spring::ScopedResource(
			FcFreeTypeQueryFace(origFace, ftname, 0, blanks),
			[](FcPattern* p) { if (p) FcPatternDestroy(p); }
		);

		if (origPattern != nullptr) {
			FcPatternGetInteger(origPattern, FC_WEIGHT , 0, &weight );
			FcPatternGetInteger(origPattern, FC_SLANT  , 0, &slant  );
			FcPatternGetBool(   origPattern, FC_OUTLINE, 0, &outline);

			FcPatternGetString( origPattern, FC_FAMILY , 0, &family );
			FcPatternGetString( origPattern, FC_FOUNDRY, 0, &foundry);
		}

		FcPatternAddInteger(pattern, FC_WEIGHT, weight);
		FcPatternAddInteger(pattern, FC_SLANT, slant);
		FcPatternAddBool(pattern, FC_OUTLINE, outline);

		if (family)
			FcPatternAddString(pattern, FC_FAMILY, family);
		if (foundry)
			FcPatternAddString(pattern, FC_FOUNDRY, foundry);
	}

	FcDefaultSubstitute(pattern);
	if (!FcConfigSubstitute(nullptr, pattern, FcMatchPattern))
	{
		return nullptr;
	}

	// search fonts that fit our request
	FcResult res;
	auto fs = spring::ScopedResource(
		FcFontSort(nullptr, pattern, FcFalse, nullptr, &res),
		[](FcFontSet* f) { if (f) FcFontSetDestroy(f); }
	);

	if (fs == nullptr)
		return nullptr;
	if (res != FcResultMatch)
		return nullptr;

	// iterate returned font list
	for (int i = 0; i < fs->nfont; ++i) {
		const FcPattern* font = fs->fonts[i];

		FcChar8* cFilename = nullptr;
		FcResult r = FcPatternGetString(font, FC_FILE, 0, &cFilename);
		if (r != FcResultMatch || cFilename == nullptr)
			continue;

		const std::string filename = std::string{ reinterpret_cast<char*>(cFilename) };

		if (invalidFonts.find(std::make_pair(filename, origSize)) != invalidFonts.end()) //this font is known to error out
			continue;

		try {
			auto face = GetFontFace(filename, origSize);

			if (blackList.find(GetFaceKey(*face)) != blackList.cend())
				continue;

			return face;
		}
		catch (content_error& ex) {
			invalidFonts.emplace(std::make_pair(filename, origSize));
			//LOG_L(L_ERROR, "[%s] \"%s\" (s = %d): %s", __func__, filename.c_str(), origSize, ex.what());
			continue;
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
{
	atlasAlloc.SetNonPowerOfTwo(globalRendering->supportNonPowerOfTwoTex);
	atlasAlloc.SetMaxSize(globalRendering->maxTextureSize, globalRendering->maxTextureSize);

	atlasGlyphs.reserve(1024);

	if (fontSize <= 0)
		fontSize = 14;

	static constexpr int FT_INTERNAL_DPI = 64;
	normScale = 1.0f / (fontSize * FT_INTERNAL_DPI);

	fontFamily = "unknown";
	fontStyle  = "unknown";

#ifndef HEADLESS

	try {
		shFace = GetFontFace(fontfile, fontSize);
	}
	catch (content_error& ex) {
		LOG_L(L_ERROR, "[%s] %s (s=%d): %s", __func__, fontfile.c_str(), fontSize, ex.what());
		return;
	}

	if (shFace == nullptr)
		return;

	FT_Face face = *shFace;

	fontFamily = face->family_name;
	fontStyle  = face->style_name;

	fontDescender = normScale * FT_MulFix(face->descender, face->size->metrics.y_scale);
	//lineHeight = FT_MulFix(face->height, face->size->metrics.y_scale); // bad results
	lineHeight = face->height / face->units_per_EM;

	if (lineHeight <= 0)
		lineHeight = 1.25 * (face->bbox.yMax - face->bbox.yMin);

	// has to be done before first GetGlyph() call!
	CreateTexture(32, 32);

	// precache ASCII glyphs & kernings (save them in kerningPrecached array for better lvl2 cpu cache hitrate)

	//preload Glyphs
	LoadWantedGlyphs(32, 128);

	for (char32_t i = 32; i < 128; ++i) {
		const auto& lgl = GetGlyph(i);
		const float advance = lgl.advance;
		for (char32_t j = 32; j < 128; ++j) {
			const auto& rgl = GetGlyph(j);
			const auto hash = GetKerningHash(i, j);
			FT_Vector kerning;
			FT_Get_Kerning(face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
			kerningPrecached[hash] = advance + normScale * kerning.x;
		}
	}
#endif
}

CFontTexture::~CFontTexture()
{
#ifndef HEADLESS
	glDeleteTextures(1, &glyphAtlasTextureID);
	glyphAtlasTextureID = 0;
#endif
}


void CFontTexture::KillFonts()
{
	// check unused fonts
	spring::VectorEraseAllIf(allFonts, [](std::weak_ptr<CFontTexture> item) { return item.expired(); });

	assert(allFonts.empty());
	allFonts = {}; //just in case
}

void CFontTexture::Update() {
	// called from Game::UpdateUnsynced
	assert(CFontTexture::threadSafety || Threading::IsMainThread());
	std::lock_guard lk(*GetCacheMutex());

	// check unused fonts
	spring::VectorEraseAllIf(allFonts, [](std::weak_ptr<CFontTexture> item) { return item.expired(); });

	for_mt_chunk(0, allFonts.size(), [](int i) {
		allFonts[i].lock()->UpdateGlyphAtlasTexture();
	});

	for (const auto& font : allFonts)
		font.lock()->UploadGlyphAtlasTexture();
}

const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	const auto it = glyphs.find(ch);

	if (it != glyphs.end())
		return it->second;
#endif

	return dummyGlyph;
}


float CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
#ifndef HEADLESS
	// first check caches
	const uint64_t hash = GetKerningHash(lgl.letter, rgl.letter);

	if (hash < kerningPrecached.size())
		return kerningPrecached[hash];

	const auto it = kerningDynamic.find(hash);

	if (it != kerningDynamic.end())
		return it->second;

	if (lgl.face != rgl.face)
		return (kerningDynamic[hash] = lgl.advance);

	// load & cache
	FT_Vector kerning;
	FT_Get_Kerning(*lgl.face, lgl.index, rgl.index, FT_KERNING_DEFAULT, &kerning);
	return (kerningDynamic[hash] = lgl.advance + normScale * kerning.x);
#else
	return 0;
#endif
}

void CFontTexture::LoadWantedGlyphs(char32_t begin, char32_t end)
{
	static std::vector<char32_t> wanted;
	wanted.clear();
	for (char32_t i = begin; i < end; ++i)
		wanted.emplace_back(i);

	LoadWantedGlyphs(wanted);
}

void CFontTexture::LoadWantedGlyphs(const std::vector<char32_t>& wanted)
{
	if (wanted.empty())
		return;

	assert(CFontTexture::threadSafety || Threading::IsMainThread());
	std::lock_guard lk(*GetCacheMutex());

	static std::vector<char32_t> map;
	map.clear();

	for (auto c : wanted) {
		if (failedToFind.find(c) == failedToFind.end())
			map.emplace_back(c);
	}
	spring::VectorSortUnique(map);

	if (map.empty())
		return;

	// load glyphs from different fonts (using fontconfig)
	std::shared_ptr<FontFace> f = shFace;

#ifndef HEADLESS
	static spring::unordered_set<std::string> alreadyCheckedFonts;
	alreadyCheckedFonts.clear();
	do {
		alreadyCheckedFonts.insert(GetFaceKey(*f));

		for (std::size_t idx = 0; idx < map.size(); /*nop*/) {
			FT_UInt index = FT_Get_Char_Index(*f, map[idx]);

			if (index != 0) {
				LoadGlyph(f, map[idx], index);

				map[idx] = map.back();
				map.pop_back();
			}
			else {
				++idx;
			}
		}
		f = GetFontForCharacters(map, *f, fontSize, alreadyCheckedFonts);
	} while (!map.empty() && f);
#endif

	// load fail glyph for all remaining ones (they will all share the same fail glyph)
	for (auto c: map) {
		LoadGlyph(shFace, c, 0);
		failedToFind.insert(c);
	}


	// read atlasAlloc glyph data back into atlasUpdate{Shadow}
	{
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

		for (const auto i : wanted) {
			const std::string glyphName  = IntToString(i);
			const std::string glyphName2 = glyphName + "sh";

			if (!atlasAlloc.contains(glyphName))
				continue;

			const auto texpos  = atlasAlloc.GetEntry(glyphName);
			const auto texpos2 = atlasAlloc.GetEntry(glyphName2);

			//glyphs is a map
			auto& thisGlyph = glyphs[i];

			thisGlyph.texCord       = IGlyphRect(texpos [0], texpos [1], texpos [2] - texpos [0], texpos [3] - texpos [1]);
			thisGlyph.shadowTexCord = IGlyphRect(texpos2[0], texpos2[1], texpos2[2] - texpos2[0], texpos2[3] - texpos2[1]);

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

	// schedule a texture update
	++curTextureUpdate;
}



void CFontTexture::LoadGlyph(std::shared_ptr<FontFace>& f, char32_t ch, unsigned index)
{
#ifndef HEADLESS
	if (glyphs.find(ch) != glyphs.end())
		return;

	// check for duplicated glyphs
	const auto pred = [&](const std::pair<char32_t, GlyphInfo>& p) { return (p.second.index == index && *p.second.face == f->face); };
	const auto iter = std::find_if(glyphs.begin(), glyphs.end(), pred);

	if (iter != glyphs.end()) {
		auto& glyph = glyphs[ch];
		glyph = iter->second;
		glyph.letter = ch;
		return;
	}

	auto& glyph = glyphs[ch];
	glyph.face  = f;
	glyph.index = index;
	glyph.letter = ch;

	// load glyph
	if (FT_Load_Glyph(*f, index, FT_LOAD_RENDER) != 0)
		LOG_L(L_ERROR, "Couldn't load glyph %d", ch);

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
	if (glyph.advance == 0 && glyph.size.w > 0)
		glyph.advance = glyph.size.w;

	const int width  = slot->bitmap.width;
	const int height = slot->bitmap.rows;
	const int olSize = 2 * outlineSize;

	if (width <= 0 || height <= 0)
		return;

	if (slot->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) {
		LOG_L(L_ERROR, "invalid pixeldata mode");
		return;
	}

	if (slot->bitmap.pitch != width) {
		LOG_L(L_ERROR, "invalid pitch");
		return;
	}

	// store glyph bitmap (index) in allocator until the next LoadWantedGlyphs call
	atlasGlyphs.emplace_back(slot->bitmap.buffer, width, height, 1);

	atlasAlloc.AddEntry(IntToString(ch)       , int2(width         , height         ), reinterpret_cast<void*>(atlasGlyphs.size() - 1));
	atlasAlloc.AddEntry(IntToString(ch) + "sh", int2(width + olSize, height + olSize)                                                 );
#endif
}

void CFontTexture::CreateTexture(const int width, const int height)
{
#ifndef HEADLESS
	glGenTextures(1, &glyphAtlasTextureID);
	glBindTexture(GL_TEXTURE_2D, glyphAtlasTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// no border to prevent artefacts in outlined text
	constexpr GLfloat borderColor[4] = {0.0f, 1.0f, 1.0f, 1.0f};
	// constexpr GLint swizzleMask[4] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};

	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

	atlasUpdate = {};
	atlasUpdate.Alloc(texWidth = wantedTexWidth = width, texHeight = wantedTexHeight = height, 1);

	atlasUpdateShadow = {};
	atlasUpdateShadow.Alloc(width, height, 1);
#endif
}

void CFontTexture::ReallocAtlases(bool pre)
{
#ifndef HEADLESS
	static std::vector<uint8_t> atlasMem;
	static std::vector<uint8_t> atlasShadowMem;
	static int2 atlasDim;
	static int2 atlasUDim;

	if (pre) {
		assert(!atlasUpdate.Empty());

		atlasMem.clear();
		atlasMem.resize(atlasUpdate.GetMemSize());

		atlasShadowMem.clear();
		atlasShadowMem.resize(atlasUpdateShadow.GetMemSize());

		memcpy(atlasMem.data(), atlasUpdate.GetRawMem(), atlasUpdate.GetMemSize());
		memcpy(atlasShadowMem.data(), atlasUpdateShadow.GetRawMem(), atlasUpdateShadow.GetMemSize());

		atlasDim = { atlasUpdate.xsize, atlasUpdate.ysize };
		atlasUDim = { atlasUpdateShadow.xsize, atlasUpdateShadow.ysize };

		atlasUpdate = {};
		atlasUpdateShadow = {};
		return;
	}

	// NB: pool has already been wiped here, do not return memory to it but just realloc
	atlasUpdate.Alloc(atlasDim.x, atlasDim.y, 1);
	atlasUpdateShadow.Alloc(atlasUDim.x, atlasUDim.y, 1);

	memcpy(atlasUpdate.GetRawMem(), atlasMem.data(), atlasMem.size());
	memcpy(atlasUpdateShadow.GetRawMem(), atlasShadowMem.data(), atlasShadowMem.size());


	if (atlasGlyphs.empty()) {
		atlasMem = {};
		atlasShadowMem = {};
		atlasDim = {};
		atlasUDim = {};
		return;
	}

	LOG_L(L_WARNING, "[FontTexture::%s] discarding %u glyph bitmaps", __func__, uint32_t(atlasGlyphs.size()));

	// should be empty, but realloc glyphs just in case so we can safely dispose of them
	for (CBitmap& bmp: atlasGlyphs) {
		bmp.Alloc(1, 1, 1);
	}

	atlasGlyphs.clear();

	atlasMem = {};
	atlasShadowMem = {};
	atlasDim = {};
	atlasUDim = {};
#endif
}


void CFontTexture::UpdateGlyphAtlasTexture()
{
#ifndef HEADLESS
	// no need to lock, MT safe

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

		atlasUpdateShadow = {}; // MT-safe
	}
#endif
}

void CFontTexture::UploadGlyphAtlasTexture() const
{
#ifndef HEADLESS
	// update texture atlas
	glBindTexture(GL_TEXTURE_2D, glyphAtlasTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

FT_Byte* FontFileBytes::data()
{
	return vec.data();
}

FontFace::FontFace(FT_Face f, std::shared_ptr<FontFileBytes>& mem)
	: face(f)
	, memory(mem)
{ }

FontFace::~FontFace()
{
#ifndef HEADLESS
	FT_Done_Face(face);
#endif
}

FontFace::operator FT_Face()
{
	return this->face;
}