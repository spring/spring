#include "StdAfx.h"
// CommandColors.cpp: implementation of the CCommandColors class.
//
//////////////////////////////////////////////////////////////////////
#include "CommandColors.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
using namespace std;

#include "Rendering/GL/myGL.h"
#include "System/FileSystem/FileHandler.h"


/******************************************************************************/

static int lineNumber = 0;
static bool inComment = false; //  /*text*/ comments are not implemented

static string GetLine(CFileHandler& fh);
static string GetCleanLine(CFileHandler& fh);
static vector<string> Tokenize(const string& line, int minWords = 0);


/******************************************************************************/

CCommandColors cmdColors;


static void InitColor(float color[4], float r, float g, float b, float a)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;
}


CCommandColors::CCommandColors()
{
	useColorRestarts = true;
	useRestartColor = true;
	restartAlpha = 0.25f;

	queuedLineWidth = 1.49f;
	queuedBlendSrc = GL_SRC_ALPHA;
	queuedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	selectedLineWidth = 1.49f;
	selectedBlendSrc = GL_SRC_ALPHA;
	selectedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

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

	SETUP_COLOR(start,             1.0f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(restart,           0.4f, 0.4f, 0.4f, 0.7f);
	SETUP_COLOR(stop,              0.0f, 0.0f, 0.0f, 0.7f);
	SETUP_COLOR(wait,              0.5f, 0.5f, 0.5f, 0.7f);
	SETUP_COLOR(build,             0.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(move,              0.5f, 1.0f, 0.5f, 0.7f);
	SETUP_COLOR(attack,            1.0f, 0.2f, 0.2f, 0.7f);
	SETUP_COLOR(fight,             0.5f, 0.5f, 1.0f, 0.7f);
	SETUP_COLOR(guard,             0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(patrol,            0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(capture,           1.0f, 1.0f, 0.3f, 0.7f);
	SETUP_COLOR(repair,            0.3f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(reclaim,           1.0f, 0.2f, 1.0f, 0.7f);
	SETUP_COLOR(restore,           0.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(resurrect,         0.2f, 0.6f, 1.0f, 0.7f);
	SETUP_COLOR(load,              0.3f, 1.0f, 1.0f, 0.7f);
	SETUP_COLOR(unload,            1.0f, 1.0f, 0.0f, 0.7f);
	SETUP_COLOR(rangeAttack,       1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeBuild,        0.3f, 1.0f, 0.3f, 0.7f);
	SETUP_COLOR(rangeDecloak,      0.3f, 0.3f, 1.0f, 0.7f);
	SETUP_COLOR(rangeExtract,      1.0f, 0.3f, 0.3f, 0.7f);
	SETUP_COLOR(rangeKamikaze,     0.8f, 0.8f, 0.1f, 0.7f);
	SETUP_COLOR(rangeSelfDestruct, 0.8f, 0.1f, 0.1f, 0.7f);
	SETUP_COLOR(unitBox,           0.0f, 1.0f, 0.0f, 0.9f);
	SETUP_COLOR(mouseBox,          1.0f, 1.0f, 1.0f, 0.9f);
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


bool CCommandColors::LoadConfig(const string& filename)
{
	CFileHandler ifs(filename);
	
	while (true) {
		const string line = GetCleanLine(ifs);
		if (line.empty()) {
			break;   
		}

		vector<string> words = Tokenize(line, 1);
		
		const string command = StringToLower(words[0]);
		
		if ((command == "usecolorrestarts") && (words.size() > 1)) {
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
					colors[it->second];
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


/******************************************************************************/
//
// Parsing Routines
//


// returns next line (without newlines)
static string GetLine(CFileHandler& fh)
{
	lineNumber++;
	char a;
	string s = "";
	while (true) {
		a = fh.Peek();
		if (a == EOF)  { break; }
		fh.Read(&a, 1);
		if (a == '\n') { break; }
		if (a != '\r') { s += a; }
	}
	return s;
}


// returns next non-blank line (without newlines or comments)
static string GetCleanLine(CFileHandler& fh)
{
	string::size_type pos;
	while (true) {
		if (fh.Eof()) {
			return ""; // end of file
		}
		string line = GetLine(fh);

		pos = line.find_first_not_of(" \t");
		if (pos == string::npos) {
			continue; // blank line
		}

		pos = line.find("//");
		if (pos != string::npos) {
			line.erase(pos);
			pos = line.find_first_not_of(" \t");
			if (pos == string::npos) {
				continue; // blank line (after removing comments)
			}
		}

		return line;
	}
}


static vector<string> Tokenize(const string& line, int minWords)
{
	vector<string> words;
	string::size_type start;
	string::size_type end = 0;
	while (true) {
		start = line.find_first_not_of(" \t", end);
		if (start == string::npos) {
			break;
		}
		string word;
		if ((minWords > 0) && (words.size() >= minWords)) {
			word = line.substr(start);
			// strip trailing whitespace
			string::size_type pos = word.find_last_not_of(" \t");
			if (pos != (word.size() - 1)) {
				word.resize(pos + 1);
			}
			end = string::npos;
		}
		else {
			end = line.find_first_of(" \t", start);
			if (end == string::npos) {
				word = line.substr(start);
			} else {
				word = line.substr(start, end - start);
			}
		}
		words.push_back(word);
		if (end == string::npos) {
			break;
		}
	}
	
	return words;
}


/******************************************************************************/
