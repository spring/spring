/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandColors.h"

#include "Rendering/GL/myGL.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"

#include <cstdlib> // strto*
#include <cstring> // memcpy
#include <string>
#include <vector>

/******************************************************************************/


CCommandColors cmdColors;


CCommandColors::CCommandColors()
{
	queuedBlendSrc = GL_SRC_ALPHA;
	queuedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	selectedBlendSrc = GL_SRC_ALPHA;
	selectedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	mouseBoxBlendSrc = GL_SRC_ALPHA;
	mouseBoxBlendDst = GL_ONE_MINUS_SRC_ALPHA;

#define SETUP_COLOR(name, r,g,b,a)  \
	colors[name ## Index][0] = r;   \
	colors[name ## Index][1] = g;   \
	colors[name ## Index][2] = b;   \
	colors[name ## Index][3] = a;   \
	name = colors[name ## Index];   \
	colorNames[StringToLower(#name)] = name ## Index

	colorNames.reserve(32);
	customCmds.reserve(32);

	SETUP_COLOR(start,               1.0f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(restart,             0.4f, 0.4f, 0.4f, 0.7f);
	SETUP_COLOR(stop,                0.0f, 0.0f, 0.0f, 0.7f);
	SETUP_COLOR(wait,                0.5f, 0.5f, 0.5f, 0.7f);
	SETUP_COLOR(build,               0.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(move,                0.5f, 1.0f, 0.5f, 0.7f);
	SETUP_COLOR(attack,              1.0f, 0.2f, 0.2f, 0.7f);
	SETUP_COLOR(fight,               0.5f, 0.5f, 1.0f, 0.7f);
	SETUP_COLOR(guard,               0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(patrol,              0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(capture,             1.0f, 1.0f, 0.3f, 0.7f);
	SETUP_COLOR(repair,              0.3f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(reclaim,             1.0f, 0.2f, 1.0f, 0.7f);
	SETUP_COLOR(restore,             0.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(resurrect,           0.2f, 0.6f, 1.0f, 0.7f);
	SETUP_COLOR(load,                0.3f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(unload,              1.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(deathWait,           0.5f, 0.5f, 0.5f, 0.7f);
	SETUP_COLOR(customArea,          0.5f, 0.5f, 0.5f, 0.5f); // grey

	SETUP_COLOR(rangeAttack,         1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeBuild,          0.3f, 1.0f, 0.3f, 0.7f);
	SETUP_COLOR(rangeRadar,          0.3f, 1.0f, 0.3f, 0.7f);
	SETUP_COLOR(rangeSonar,          0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(rangeSeismic,        0.8f, 0.1f, 0.8f, 0.7f);
	SETUP_COLOR(rangeJammer,         1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeSonarJammer,    1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeShield,         0.8f, 0.8f, 0.3f, 0.7f);
	SETUP_COLOR(rangeDecloak,        0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(rangeExtract,        1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeKamikaze,       0.8f, 0.8f, 0.1f, 0.7f);
	SETUP_COLOR(rangeSelfDestruct,   0.8f, 0.1f, 0.1f, 0.7f);
	SETUP_COLOR(rangeInterceptorOn,  1.0f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(rangeInterceptorOff, 0.0f, 0.0f, 0.0f, 0.7f);
	SETUP_COLOR(unitBox,             0.0f, 1.0f, 0.0f, 0.9f);
	SETUP_COLOR(buildBox,            0.0f, 1.0f, 0.0f, 0.9f);
	SETUP_COLOR(allyBuildBox,        0.8f, 0.8f, 0.2f, 0.9f);
	SETUP_COLOR(mouseBox,            1.0f, 1.0f, 1.0f, 0.9f);
}



static bool ParseBlendMode(const std::string& word, unsigned int& mode)
{
	std::string lower = std::move(StringToLower(word));

	switch (hashString(lower.c_str())) {
		case hashString("zero"               ): { mode = GL_ZERO;                return true; } break;
		case hashString("one"                ): { mode = GL_ONE;                 return true; } break;
		case hashString("src_alpha"          ): { mode = GL_SRC_ALPHA;           return true; } break;
		case hashString("src_color"          ): { mode = GL_SRC_COLOR;           return true; } break;
		case hashString("one_minus_src_alpha"): { mode = GL_ONE_MINUS_SRC_ALPHA; return true; } break;
		case hashString("one_minus_src_color"): { mode = GL_ONE_MINUS_SRC_COLOR; return true; } break;
		case hashString("dst_alpha"          ): { mode = GL_DST_ALPHA;           return true; } break;
		case hashString("dst_color"          ): { mode = GL_DST_COLOR;           return true; } break;
		case hashString("one_minus_dst_alpha"): { mode = GL_ONE_MINUS_DST_ALPHA; return true; } break;
		case hashString("one_minus_dst_color"): { mode = GL_ONE_MINUS_DST_COLOR; return true; } break;
		case hashString("src_alpha_saturate" ): { mode = GL_SRC_ALPHA_SATURATE;  return true; } break;
		default                               : {                                             } break;
	}

	return false;
}


static bool IsValidSrcMode(unsigned int mode)
{
	switch (mode) {
		case GL_ZERO:
		case GL_ONE:
		case GL_DST_COLOR:
		case GL_ONE_MINUS_DST_COLOR:
		case GL_SRC_ALPHA:
		case GL_ONE_MINUS_SRC_ALPHA:
		case GL_DST_ALPHA:
		case GL_ONE_MINUS_DST_ALPHA:
		case GL_SRC_ALPHA_SATURATE: {
			return true;
		}
	}

	return false;
}

static bool IsValidDstMode(unsigned int mode)
{
	switch (mode) {
		case GL_ZERO:
		case GL_ONE:
		case GL_SRC_COLOR:
		case GL_ONE_MINUS_SRC_COLOR:
		case GL_SRC_ALPHA:
		case GL_ONE_MINUS_SRC_ALPHA:
		case GL_DST_ALPHA:
		case GL_ONE_MINUS_DST_ALPHA: {
			return true;
		}
	}

	return false;
}


static bool SafeAtoF(float& var, const std::string& value)
{
	const char* startPtr = value.c_str();
	      char*   endPtr = nullptr;

	const float tmp = static_cast<float>(strtod(startPtr, &endPtr));

	if (endPtr == startPtr)
		return false;

	var = tmp;
	return true;
}

static bool SafeAtoI(unsigned int& var, const std::string& value)
{
	const char* startPtr = value.c_str();
	      char*   endPtr = nullptr;

	const unsigned int tmp = static_cast<unsigned int>(strtol(startPtr, &endPtr, 0));

	if (endPtr == startPtr)
		return false;

	var = tmp;
	return true;
}


bool CCommandColors::LoadConfigFromFile(const std::string& filename)
{
	CFileHandler ifs(filename);
	std::string cfg;
	ifs.LoadStringData(cfg);
	return LoadConfigFromString(cfg);
}


bool CCommandColors::LoadConfigFromString(const std::string& cfg)
{
	CSimpleParser parser(cfg);

	while (true) {
		const std::string line = std::move(parser.GetCleanLine());

		if (line.empty())
			break;

		const std::vector<std::string> words = std::move(parser.Tokenize(line, 1));

		if (words.size() <= 1)
			continue;

		const std::string command = std::move(StringToLower(words[0]));

		switch (hashString(command.c_str())) {
			case hashString("alwaysdrawqueue"): {
				alwaysDrawQueue = !!atoi(words[1].c_str());
			} break;
			case hashString("usequeueicons"): {
				useQueueIcons = !!atoi(words[1].c_str());
			} break;
			case hashString("queueiconalpha"): {
				SafeAtoF(queueIconAlpha, words[1]);
			} break;
			case hashString("queueiconscale"): {
				SafeAtoF(queueIconScale, words[1]);
			} break;
			case hashString("usecolorrestarts"): {
				useColorRestarts = !!atoi(words[1].c_str());
			} break;
			case hashString("userestartcolor"): {
				useRestartColor = !!atoi(words[1].c_str());
			} break;
			case hashString("restartalpha"): {
				SafeAtoF(restartAlpha, words[1]);
			} break;
			case hashString("queuedlinewidth"): {
				SafeAtoF(queuedLineWidth, words[1]);
			} break;

			case hashString("queuedblendsrc"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidSrcMode(mode))
					continue;

				queuedBlendSrc = mode;
			} break;
			case hashString("queuedblenddst"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidDstMode(mode))
					continue;

				queuedBlendDst = mode;
			} break;

			case hashString("stipplepattern"): {
				SafeAtoI(stipplePattern, words[1]);
			} break;
			case hashString("stipplefactor"): {
				SafeAtoI(stippleFactor, words[1]);
			} break;
			case hashString("stipplespeed"): {
				SafeAtoF(stippleSpeed, words[1]);
			} break;

			case hashString("selectedlinewidth"): {
				SafeAtoF(selectedLineWidth, words[1]);
			} break;

			case hashString("selectedblendsrc"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidSrcMode(mode))
					continue;

				selectedBlendSrc = mode;
			} break;
			case hashString("selectedblenddst"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidDstMode(mode))
					continue;

				selectedBlendDst = mode;
			} break;

			case hashString("buildboxesonshift"): {
				buildBoxesOnShift = !!atoi(words[1].c_str());
			} break;

			case hashString("mouseboxlinewidth"): {
				SafeAtoF(mouseBoxLineWidth, words[1]);
			} break;

			case hashString("mouseboxblendsrc"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidSrcMode(mode))
					continue;

				mouseBoxBlendSrc = mode;
			} break;
			case hashString( "mouseboxblenddst"): {
				unsigned int mode;

				if (!ParseBlendMode(words[1], mode) || !IsValidDstMode(mode))
					continue;

				mouseBoxBlendDst = mode;
			} break;

			case hashString("unitboxlinewidth"): {
				SafeAtoF(unitBoxLineWidth, words[1]);
			} break;

			default: {
				// try to parse a color by name
				const auto it = colorNames.find(command);

				if (it == colorNames.end())
					continue;

				float tmp[4] = {0.0f, 0.0f, 0.0f, 1.0f};
				int count;

				// require RGB, optionally A
				if ((count = sscanf(words[1].c_str(), "%f %f %f %f", &tmp[0], &tmp[1], &tmp[2], &tmp[3])) < 3)
					continue;

				memcpy(colors[it->second], tmp, sizeof(float[4]));
			} break;
		}
	}

	return true;
}

