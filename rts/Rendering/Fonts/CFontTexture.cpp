/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CFontTexture.h"
#include "FontLogSection.h"

#include <cstring> // for memset, memcpy
#include <string>
#include <vector>
#include <sstream>

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

#define SUPPORT_AMD_HACKS_HERE

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

//Python printable = (c).to_bytes(4, byteorder="little").decode("utf32").isprintable()
std::vector<char32_t> CFontTexture::nonPrintableRanges = {
	0,
	31,
	127,
	160,
	173,
	173,
	888,
	889,
	896,
	899,
	907,
	907,
	909,
	909,
	930,
	930,
	1328,
	1328,
	1367,
	1368,
	1419,
	1420,
	1424,
	1424,
	1480,
	1487,
	1515,
	1518,
	1525,
	1541,
	1564,
	1565,
	1757,
	1757,
	1806,
	1807,
	1867,
	1868,
	1970,
	1983,
	2043,
	2044,
	2094,
	2095,
	2111,
	2111,
	2140,
	2141,
	2143,
	2143,
	2155,
	2207,
	2229,
	2229,
	2248,
	2258,
	2274,
	2274,
	2436,
	2436,
	2445,
	2446,
	2449,
	2450,
	2473,
	2473,
	2481,
	2481,
	2483,
	2485,
	2490,
	2491,
	2501,
	2502,
	2505,
	2506,
	2511,
	2518,
	2520,
	2523,
	2526,
	2526,
	2532,
	2533,
	2559,
	2560,
	2564,
	2564,
	2571,
	2574,
	2577,
	2578,
	2601,
	2601,
	2609,
	2609,
	2612,
	2612,
	2615,
	2615,
	2618,
	2619,
	2621,
	2621,
	2627,
	2630,
	2633,
	2634,
	2638,
	2640,
	2642,
	2648,
	2653,
	2653,
	2655,
	2661,
	2679,
	2688,
	2692,
	2692,
	2702,
	2702,
	2706,
	2706,
	2729,
	2729,
	2737,
	2737,
	2740,
	2740,
	2746,
	2747,
	2758,
	2758,
	2762,
	2762,
	2766,
	2767,
	2769,
	2783,
	2788,
	2789,
	2802,
	2808,
	2816,
	2816,
	2820,
	2820,
	2829,
	2830,
	2833,
	2834,
	2857,
	2857,
	2865,
	2865,
	2868,
	2868,
	2874,
	2875,
	2885,
	2886,
	2889,
	2890,
	2894,
	2900,
	2904,
	2907,
	2910,
	2910,
	2916,
	2917,
	2936,
	2945,
	2948,
	2948,
	2955,
	2957,
	2961,
	2961,
	2966,
	2968,
	2971,
	2971,
	2973,
	2973,
	2976,
	2978,
	2981,
	2983,
	2987,
	2989,
	3002,
	3005,
	3011,
	3013,
	3017,
	3017,
	3022,
	3023,
	3025,
	3030,
	3032,
	3045,
	3067,
	3071,
	3085,
	3085,
	3089,
	3089,
	3113,
	3113,
	3130,
	3132,
	3141,
	3141,
	3145,
	3145,
	3150,
	3156,
	3159,
	3159,
	3163,
	3167,
	3172,
	3173,
	3184,
	3190,
	3213,
	3213,
	3217,
	3217,
	3241,
	3241,
	3252,
	3252,
	3258,
	3259,
	3269,
	3269,
	3273,
	3273,
	3278,
	3284,
	3287,
	3293,
	3295,
	3295,
	3300,
	3301,
	3312,
	3312,
	3315,
	3327,
	3341,
	3341,
	3345,
	3345,
	3397,
	3397,
	3401,
	3401,
	3408,
	3411,
	3428,
	3429,
	3456,
	3456,
	3460,
	3460,
	3479,
	3481,
	3506,
	3506,
	3516,
	3516,
	3518,
	3519,
	3527,
	3529,
	3531,
	3534,
	3541,
	3541,
	3543,
	3543,
	3552,
	3557,
	3568,
	3569,
	3573,
	3584,
	3643,
	3646,
	3676,
	3712,
	3715,
	3715,
	3717,
	3717,
	3723,
	3723,
	3748,
	3748,
	3750,
	3750,
	3774,
	3775,
	3781,
	3781,
	3783,
	3783,
	3790,
	3791,
	3802,
	3803,
	3808,
	3839,
	3912,
	3912,
	3949,
	3952,
	3992,
	3992,
	4029,
	4029,
	4045,
	4045,
	4059,
	4095,
	4294,
	4294,
	4296,
	4300,
	4302,
	4303,
	4681,
	4681,
	4686,
	4687,
	4695,
	4695,
	4697,
	4697,
	4702,
	4703,
	4745,
	4745,
	4750,
	4751,
	4785,
	4785,
	4790,
	4791,
	4799,
	4799,
	4801,
	4801,
	4806,
	4807,
	4823,
	4823,
	4881,
	4881,
	4886,
	4887,
	4955,
	4956,
	4989,
	4991,
	5018,
	5023,
	5110,
	5111,
	5118,
	5119,
	5760,
	5760,
	5789,
	5791,
	5881,
	5887,
	5901,
	5901,
	5909,
	5919,
	5943,
	5951,
	5972,
	5983,
	5997,
	5997,
	6001,
	6001,
	6004,
	6015,
	6110,
	6111,
	6122,
	6127,
	6138,
	6143,
	6158,
	6159,
	6170,
	6175,
	6265,
	6271,
	6315,
	6319,
	6390,
	6399,
	6431,
	6431,
	6444,
	6447,
	6460,
	6463,
	6465,
	6467,
	6510,
	6511,
	6517,
	6527,
	6572,
	6575,
	6602,
	6607,
	6619,
	6621,
	6684,
	6685,
	6751,
	6751,
	6781,
	6782,
	6794,
	6799,
	6810,
	6815,
	6830,
	6831,
	6849,
	6911,
	6988,
	6991,
	7037,
	7039,
	7156,
	7163,
	7224,
	7226,
	7242,
	7244,
	7305,
	7311,
	7355,
	7356,
	7368,
	7375,
	7419,
	7423,
	7674,
	7674,
	7958,
	7959,
	7966,
	7967,
	8006,
	8007,
	8014,
	8015,
	8024,
	8024,
	8026,
	8026,
	8028,
	8028,
	8030,
	8030,
	8062,
	8063,
	8117,
	8117,
	8133,
	8133,
	8148,
	8149,
	8156,
	8156,
	8176,
	8177,
	8181,
	8181,
	8191,
	8207,
	8232,
	8239,
	8287,
	8303,
	8306,
	8307,
	8335,
	8335,
	8349,
	8351,
	8384,
	8399,
	8433,
	8447,
	8588,
	8591,
	9255,
	9279,
	9291,
	9311,
	11124,
	11125,
	11158,
	11158,
	11311,
	11311,
	11359,
	11359,
	11508,
	11512,
	11558,
	11558,
	11560,
	11564,
	11566,
	11567,
	11624,
	11630,
	11633,
	11646,
	11671,
	11679,
	11687,
	11687,
	11695,
	11695,
	11703,
	11703,
	11711,
	11711,
	11719,
	11719,
	11727,
	11727,
	11735,
	11735,
	11743,
	11743,
	11859,
	11903,
	11930,
	11930,
	12020,
	12031,
	12246,
	12271,
	12284,
	12288,
	12352,
	12352,
	12439,
	12440,
	12544,
	12548,
	12592,
	12592,
	12687,
	12687,
	12772,
	12783,
	12831,
	12831,
	40957,
	40959,
	42125,
	42127,
	42183,
	42191,
	42540,
	42559,
	42744,
	42751,
	42944,
	42945,
	42955,
	42996,
	43053,
	43055,
	43066,
	43071,
	43128,
	43135,
	43206,
	43213,
	43226,
	43231,
	43348,
	43358,
	43389,
	43391,
	43470,
	43470,
	43482,
	43485,
	43519,
	43519,
	43575,
	43583,
	43598,
	43599,
	43610,
	43611,
	43715,
	43738,
	43767,
	43776,
	43783,
	43784,
	43791,
	43792,
	43799,
	43807,
	43815,
	43815,
	43823,
	43823,
	43884,
	43887,
	44014,
	44015,
	44026,
	44031,
	55204,
	55215,
	55239,
	55242,
	55292,
	std::numeric_limits<uint32_t>::max()
};



#ifndef HEADLESS
class FtLibraryHandler {
public:
	FtLibraryHandler()
		: config(nullptr)
		, lib(nullptr)
	{
		{
			const FT_Error error = FT_Init_FreeType(&lib);

			FT_Int version[3];
			FT_Library_Version(lib, &version[0], &version[1], &version[2]);

			std::string msg = fmt::sprintf("%s::FreeTypeInit (version %d.%d.%d)", __func__, version[0], version[1], version[2]);
			std::string err = fmt::sprintf("[%s] FT_Init_FreeType failure \"%s\"", __func__, GetFTError(error));

			if (error != 0)
				throw std::runtime_error(err);
		}

        #ifdef USE_FONTCONFIG
		if (!UseFontConfig())
			return;

		{
			std::string msg = fmt::sprintf("%s::FontConfigInit (version %d.%d.%d)", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);
			ScopedOnceTimer timer(msg);

			config = FcInitLoadConfig();
		}
		#endif
	}

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);

		#ifdef USE_FONTCONFIG
		if (!UseFontConfig())
			return;

		FcConfigDestroy(config);
		FcFini();
		config = nullptr;
		#endif
	}

	// reduced set of fonts
	// not called if FcInit() fails
	static bool CheckGenFontConfigFast() {
		FcConfigAppFontClear(GetFCConfig());
		if (!FcConfigAppFontAddDir(GetFCConfig(), reinterpret_cast<const FcChar8*>("fonts")))
			return false;

		if (!FtLibraryHandler::CheckFontConfig()) {
			return FcConfigBuildFonts(GetFCConfig());
		}

		return true;
	}

	static bool CheckGenFontConfigFull(bool console) {
	#ifndef HEADLESS
		auto LOG_MSG = [console](const std::string& fmt, bool isError, auto&&... args) {
			if (console) {
				std::string fmtNL = fmt + "\n";
				printf(fmtNL.c_str(), args...);
			}
			else {
				if (isError) {
					LOG_L(L_ERROR, fmt.c_str(), args...);
				}
				else {
					LOG(fmt.c_str(), args...);
				}
			}
		};

		if (!FtLibraryHandler::CanUseFontConfig()) {
			LOG_MSG("[%s] Fontconfig(version %d.%d.%d) failed to initialize", true, __func__, FC_MAJOR, FC_MINOR, FC_REVISION);
			return false;
		}

		char osFontsDir[8192];

		#ifdef _WIN32
			ExpandEnvironmentStrings("%WINDIR%\\fonts", osFontsDir, sizeof(osFontsDir)); // expands %HOME% etc.
		#else
			strncpy(osFontsDir, "/etc/fonts/", sizeof(osFontsDir));
		#endif

		FcConfigAppFontClear(GetFCConfig());
		FcConfigAppFontAddDir(GetFCConfig(), reinterpret_cast<const FcChar8*>("fonts"));
		FcConfigAppFontAddDir(GetFCConfig(), reinterpret_cast<const FcChar8*>(osFontsDir));

		{
			auto dirs = FcConfigGetCacheDirs(GetFCConfig());
			FcStrListFirst(dirs);
			for (FcChar8* dir = FcStrListNext(dirs), *prevDir = nullptr; dir != nullptr && dir != prevDir; ) {
				prevDir = dir;
				LOG_MSG("[%s] Using Fontconfig cache dir \"%s\"", false, __func__, dir);
			}
			FcStrListDone(dirs);
		}

		if (FtLibraryHandler::CheckFontConfig()) {
			LOG_MSG("[%s] fontconfig for directory \"%s\" up to date", false, __func__, osFontsDir);
			return true;
		}

		LOG_MSG("[%s] creating fontconfig for directory \"%s\"", false, __func__, osFontsDir);

		return FcConfigBuildFonts(GetFCConfig());
	#endif

		return true;
	}

	static bool UseFontConfig() { return (configHandler == nullptr || configHandler->GetBool("UseFontConfigLib")); }

	#ifdef USE_FONTCONFIG
	// command-line CheckGenFontConfigFull invocation checks
	static bool CheckFontConfig() { return (UseFontConfig() && FcConfigUptoDate(GetFCConfig())); }
	#else

	static bool CheckFontConfig() { return false; }
	static bool CheckGenFontConfig(bool fromCons) { return false; }
	#endif

	static FT_Library& GetLibrary() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->lib;
	};
	static FcConfig* GetFCConfig() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->config;
	}
	static inline bool CanUseFontConfig() {
		return GetFCConfig() != nullptr;
	}
private:
	FcConfig* config;
	FT_Library lib;

	static inline std::unique_ptr<FtLibraryHandler> singleton = nullptr;
};
#endif



void FtLibraryHandlerProxy::InitFtLibrary()
{
#ifndef HEADLESS
	FtLibraryHandler::GetLibrary();
#endif
}

bool FtLibraryHandlerProxy::CheckGenFontConfigFast()
{
#ifndef HEADLESS
	return FtLibraryHandler::CheckGenFontConfigFast();
#else
	return false;
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
	if (!FtLibraryHandler::CanUseFontConfig())
		return nullptr;

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

		FcPatternAddCharSet(pattern, FC_CHARSET   , cset);
		FcPatternAddBool(   pattern, FC_SCALABLE  , FcTrue);
		FcPatternAddDouble( pattern, FC_SIZE      , static_cast<double>(origSize));

		double pixelSize = 0.0;
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
			FcPatternGetInteger(origPattern, FC_WEIGHT    , 0, &weight );
			FcPatternGetInteger(origPattern, FC_SLANT     , 0, &slant  );
			FcPatternGetBool(   origPattern, FC_OUTLINE   , 0, &outline);
			FcPatternGetDouble( origPattern, FC_PIXEL_SIZE, 0, &pixelSize);

			FcPatternGetString( origPattern, FC_FAMILY , 0, &family );
			FcPatternGetString( origPattern, FC_FOUNDRY, 0, &foundry);

		}

		FcPatternAddInteger(pattern, FC_WEIGHT, weight);
		FcPatternAddInteger(pattern, FC_SLANT, slant);
		FcPatternAddBool(pattern, FC_OUTLINE, outline);

		if (pixelSize > 0.0)
			FcPatternAddDouble(pattern, FC_PIXEL_SIZE, pixelSize);

		if (family)
			FcPatternAddString(pattern, FC_FAMILY, family);
		if (foundry)
			FcPatternAddString(pattern, FC_FOUNDRY, foundry);
	}

	FcDefaultSubstitute(pattern);
	if (!FcConfigSubstitute(FtLibraryHandler::GetFCConfig(), pattern, FcMatchPattern))
	{
		return nullptr;
	}

	// search fonts that fit our request
	FcResult res;
	auto fs = spring::ScopedResource(
		FcFontSort(FtLibraryHandler::GetFCConfig(), pattern, FcFalse, nullptr, &res),
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

			#ifdef _DEBUG
			{
				std::ostringstream ss;
				for (auto c : characters) {
					ss << "<" << static_cast<uint32_t>(c) << ">";
				}
				LOG_L(L_INFO, "[%s] Using \"%s\" to render chars (size=%d) %s", __func__, filename.c_str(), origSize, ss.str().c_str());
			}
			#endif

			return face;
		}
		catch (content_error& ex) {
			invalidFonts.emplace(filename, origSize);
			LOG_L(L_WARNING, "[%s] \"%s\" (s = %d): %s", __func__, filename.c_str(), origSize, ex.what());
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

	static constexpr int FT_INTERNAL_DPI = 64;
	normScale = 1.0f / (fontSize * FT_INTERNAL_DPI);

	if (!FT_IS_SCALABLE(shFace->face)) {
		LOG_L(L_WARNING, "[%s] %s is not scalable", __func__, fontfile.c_str());
		normScale = 1.0f;
	}

	if (!FT_HAS_KERNING(face)) {
		LOG_L(L_INFO, "[%s] %s has no kerning data", __func__, fontfile.c_str());
	}

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
	LoadWantedGlyphs(32, 127);
	for (char32_t i = 32; i < 127; ++i) {
		const auto& lgl = GetGlyph(i);
		const float advance = lgl.advance;
		for (char32_t j = 32; j < 127; ++j) {
			const auto& rgl = GetGlyph(j);
			const auto hash = GetKerningHash(i, j);
			FT_Vector kerning = {};
			if (FT_HAS_KERNING(face))
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

	static std::vector<std::shared_ptr<CFontTexture>> fontsToUpdate;
	fontsToUpdate.clear();

	for (const auto& font : allFonts) {
		auto lf = font.lock();
		if (lf->GlyphAtlasTextureNeedsUpdate())
			fontsToUpdate.emplace_back(std::move(lf));
	}

	for_mt_chunk(0, fontsToUpdate.size(), [](int i) {
		fontsToUpdate[i]->UpdateGlyphAtlasTexture();
	});

	for (const auto& font : fontsToUpdate)
		font->UploadGlyphAtlasTexture();

	fontsToUpdate.clear();
}

const GlyphInfo& CFontTexture::GetGlyph(char32_t ch)
{
#ifndef HEADLESS
	if (const auto it = glyphs.find(ch); it != glyphs.end())
		return it->second;
#endif

	return dummyGlyph;
}


float CFontTexture::GetKerning(const GlyphInfo& lgl, const GlyphInfo& rgl)
{
#ifndef HEADLESS
	if (!FT_HAS_KERNING(shFace->face))
		return lgl.advance;

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
		if (failedToFind.find(c) != failedToFind.end())
			continue;

		auto it = std::lower_bound(nonPrintableRanges.begin(), nonPrintableRanges.end(), c);
		if (std::distance(nonPrintableRanges.begin(), it) % 2 != 0) {
			LoadGlyph(shFace, c, 0);
			failedToFind.emplace(c);
		}
		else {
			map.emplace_back(c);
		}
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
		LOG_L(L_WARNING, "[CFontTexture::%s] Failed to load glyph %u", __func__, uint32_t(c));
		failedToFind.insert(c);
	}


	// read atlasAlloc glyph data back into atlasUpdate{Shadow}
	{
		if (!atlasAlloc.Allocate())
			LOG_L(L_WARNING, "[CFontTexture::%s] Texture limit reached! (try to reduce the font size and/or outlinewidth)", __func__);

		wantedTexWidth  = atlasAlloc.GetAtlasSize().x;
		wantedTexHeight = atlasAlloc.GetAtlasSize().y;

		if ((atlasUpdate.xsize != wantedTexWidth) || (atlasUpdate.ysize != wantedTexHeight))
			atlasUpdate = atlasUpdate.CanvasResize(wantedTexWidth, wantedTexHeight, false);

		if (atlasUpdateShadow.Empty())
			atlasUpdateShadow.Alloc(wantedTexWidth, wantedTexHeight, 1);

		if ((atlasUpdateShadow.xsize != wantedTexWidth) || (atlasUpdateShadow.ysize != wantedTexHeight))
			atlasUpdateShadow = atlasUpdateShadow.CanvasResize(wantedTexWidth, wantedTexHeight, false);

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
	constexpr GLfloat borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	// NB:
	// The modern and core formats like GL_R8 and GL_RED are intentionally replaced with
	// deprecated GL_ALPHA, such that AMD-HACK users could enjoy no-shader fallback
	// But why fallback? See: https://github.com/beyond-all-reason/spring/issues/383
	// Remove the code under `#ifdef SUPPORT_AMD_HACKS_HERE` blocks throughout this file
	// when all potatoes die.

#ifdef SUPPORT_AMD_HACKS_HERE
	constexpr GLint swizzleMaskF[] = { GL_ALPHA, GL_ALPHA, GL_ALPHA, GL_ALPHA };
	constexpr GLint swizzleMaskD[] = { GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMaskF);
#endif
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

#ifdef SUPPORT_AMD_HACKS_HERE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
#endif

	glBindTexture(GL_TEXTURE_2D, 0);
#ifdef SUPPORT_AMD_HACKS_HERE
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMaskD);
#endif

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

bool CFontTexture::GlyphAtlasTextureNeedsUpdate() const
{
#ifndef HEADLESS
	return curTextureUpdate != lastTextureUpdate;
#else
	return false;
#endif
}

void CFontTexture::UpdateGlyphAtlasTexture()
{
#ifndef HEADLESS
	// no need to lock, MT safe
	if (!GlyphAtlasTextureNeedsUpdate())
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

	needsTextureUpload = true;
#endif
}

void CFontTexture::UploadGlyphAtlasTexture()
{
#ifndef HEADLESS
	if (!needsTextureUpload)
		return;

	// update texture atlas
	glBindTexture(GL_TEXTURE_2D, glyphAtlasTextureID);
	#ifdef SUPPORT_AMD_HACKS_HERE
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texWidth, texHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	#else
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasUpdate.GetRawMem());
	#endif
	glBindTexture(GL_TEXTURE_2D, 0);

	needsTextureUpload = false;
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