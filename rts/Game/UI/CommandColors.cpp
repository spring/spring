/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CommandColors.h"


#include "Rendering/GL/myGL.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/StringUtil.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

/******************************************************************************/


CCommandColors cmdColors;


CCommandColors::CCommandColors()
{
	alwaysDrawQueue = false;

	useQueueIcons = true;
	queueIconAlpha = 0.5f;
	queueIconScale = 1.0f;
	useColorRestarts = true;
	useRestartColor = true;
	restartAlpha = 0.25f;

	queuedLineWidth = 1.49f;
	queuedBlendSrc = GL_SRC_ALPHA;
	queuedBlendDst = GL_ONE_MINUS_SRC_ALPHA;
	stipplePattern = 0xffffffff;
	stippleFactor = 1;
	stippleSpeed = 1.0f;

	selectedLineWidth = 1.49f;
	selectedBlendSrc = GL_SRC_ALPHA;
	selectedBlendDst = GL_ONE_MINUS_SRC_ALPHA;
	buildBoxesOnShift = true;

	mouseBoxLineWidth = 1.49f;
	mouseBoxBlendSrc = GL_SRC_ALPHA;
	mouseBoxBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	unitBoxLineWidth = 1.49f;

#define SETUP_COLOR(name, r,g,b,a) \
	colors[name ## _index][0] = r;   \
	colors[name ## _index][1] = g;   \
	colors[name ## _index][2] = b;   \
	colors[name ## _index][3] = a;   \
	name = colors[name ## _index];   \
	colorNames[StringToLower(#name)] = name ## _index

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


CCommandColors::~CCommandColors()
{
}


static bool ParseBlendMode(const string& word, unsigned int& mode)
{
	string lower = StringToLower(word);

	     if (lower == "zero")
	       { mode = GL_ZERO;                return true; }
	else if (lower == "one")
	       { mode = GL_ONE;                 return true; }
	else if (lower == "src_alpha")
	       { mode = GL_SRC_ALPHA;           return true; }
	else if (lower == "src_color")
	       { mode = GL_SRC_COLOR;           return true; }
	else if (lower == "one_minus_src_alpha")
	       { mode = GL_ONE_MINUS_SRC_ALPHA; return true; }
	else if (lower == "one_minus_src_color")
	       { mode = GL_ONE_MINUS_SRC_COLOR; return true; }
	else if (lower == "dst_alpha")
	       { mode = GL_DST_ALPHA;           return true; }
	else if (lower == "dst_color")
	       { mode = GL_DST_COLOR;           return true; }
	else if (lower == "one_minus_dst_alpha")
	       { mode = GL_ONE_MINUS_DST_ALPHA; return true; }
	else if (lower == "one_minus_dst_color")
	       { mode = GL_ONE_MINUS_DST_COLOR; return true; }
	else if (lower == "src_alpha_saturate")
	       { mode = GL_SRC_ALPHA_SATURATE;  return true; }

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


static bool SafeAtoF(float& var, const string& value)
{
	char* endPtr;
	const char* startPtr = value.c_str();
	const float tmp = (float)strtod(startPtr, &endPtr);
	if (endPtr == startPtr) {
		return false;
	}
	var = tmp;
	return true;
}


static bool SafeAtoI(unsigned int& var, const string& value)
{
	char* endPtr;
	const char* startPtr = value.c_str();
	const unsigned int tmp = (unsigned int)strtol(startPtr, &endPtr, 0);
	if (endPtr == startPtr) {
		return false;
	}
	var = tmp;
	return true;
}


bool CCommandColors::LoadConfigFromFile(const string& filename)
{
	CFileHandler ifs(filename);
	std::string cfg;
	ifs.LoadStringData(cfg);
	return LoadConfigFromString(cfg);
}


bool CCommandColors::LoadConfigFromString(const string& cfg)
{
	CSimpleParser parser(cfg);

	while (true) {
		const string line = parser.GetCleanLine();
		if (line.empty()) {
			break;
		}

		const vector<string> &words = parser.Tokenize(line, 1);

		const string command = StringToLower(words[0]);

		if ((command == "alwaysdrawqueue") && (words.size() > 1)) {
			alwaysDrawQueue = !!atoi(words[1].c_str());
		}
		else if ((command == "usequeueicons") && (words.size() > 1)) {
			useQueueIcons = !!atoi(words[1].c_str());
		}
		else if ((command == "queueiconalpha") && (words.size() > 1)) {
			SafeAtoF(queueIconAlpha, words[1]);
		}
		else if ((command == "queueiconscale") && (words.size() > 1)) {
			SafeAtoF(queueIconScale, words[1]);
		}
		else if ((command == "usecolorrestarts") && (words.size() > 1)) {
			useColorRestarts = !!atoi(words[1].c_str());
		}
		else if ((command == "userestartcolor") && (words.size() > 1)) {
			useRestartColor = !!atoi(words[1].c_str());
		}
		else if ((command == "restartalpha") && (words.size() > 1)) {
			SafeAtoF(restartAlpha, words[1]);
		}
		else if ((command == "queuedlinewidth") && (words.size() > 1)) {
			SafeAtoF(queuedLineWidth, words[1]);
		}
		else if ((command == "queuedblendsrc") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidSrcMode(mode)) {
				queuedBlendSrc = mode;
			}
		}
		else if ((command == "queuedblenddst") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidDstMode(mode)) {
				queuedBlendDst = mode;
			}
		}
		else if ((command == "stipplepattern") && (words.size() > 1)) {
			SafeAtoI(stipplePattern, words[1]);
		}
		else if ((command == "stipplefactor") && (words.size() > 1)) {
			SafeAtoI(stippleFactor, words[1]);
		}
		else if ((command == "stipplespeed") && (words.size() > 1)) {
			SafeAtoF(stippleSpeed, words[1]);
		}
		else if ((command == "selectedlinewidth") && (words.size() > 1)) {
			SafeAtoF(selectedLineWidth, words[1]);
		}
		else if ((command == "selectedblendsrc") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidSrcMode(mode)) {
				selectedBlendSrc = mode;
			}
		}
		else if ((command == "selectedblenddst") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidDstMode(mode)) {
				selectedBlendDst = mode;
			}
		}
		else if ((command == "buildboxesonshift") && (words.size() > 1)) {
			buildBoxesOnShift = !!atoi(words[1].c_str());
		}
		else if ((command == "mouseboxlinewidth") && (words.size() > 1)) {
			SafeAtoF(mouseBoxLineWidth, words[1]);
		}
		else if ((command == "mouseboxblendsrc") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidSrcMode(mode)) {
				mouseBoxBlendSrc = mode;
			}
		}
		else if ((command == "mouseboxblenddst") && (words.size() > 1)) {
			unsigned int mode;
			if (ParseBlendMode(words[1], mode) && IsValidDstMode(mode)) {
				mouseBoxBlendDst = mode;
			}
		}
		else if ((command == "unitboxlinewidth") && (words.size() > 1)) {
			SafeAtoF(unitBoxLineWidth, words[1]);
		}
		else {
			// try to parse a color
			if (words.size() > 1) {
				map<string, int>::iterator it = colorNames.find(command);
				if (it != colorNames.end()) {
					//colors[it->second];
					float tmp[4];
					int count = sscanf(words[1].c_str(), "%f %f %f %f",
														 &tmp[0], &tmp[1], &tmp[2], &tmp[3]);
					if (count >= 3) {
						if (count == 3) {
							tmp[3] = 1.0f;
						}
						memcpy(colors[it->second], tmp, sizeof(float[4]));
					}
				}
			}
		}
	}
	return true;
}


void CCommandColors::SetCustomCmdData(int cmdID, int cmdIconID,
                                      const float color[4], bool showArea)
{
	customCmds[cmdID] = DrawData(cmdIconID, color, showArea);
}


void CCommandColors::ClearCustomCmdData(int cmdID)
{
	customCmds.erase(cmdID);
}


const CCommandColors::DrawData*
	CCommandColors::GetCustomCmdData(int cmdID) const
{
	customCmds_type::const_iterator it = customCmds.find(cmdID);
	if (it == customCmds.end()) {
		return NULL;
	} else {
		return &(it->second);
	}
}


/******************************************************************************/
