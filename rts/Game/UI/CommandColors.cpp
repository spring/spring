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

	queuedBlendSrc = GL_SRC_ALPHA;
	queuedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	selectedBlendSrc = GL_SRC_ALPHA;
	selectedBlendDst = GL_ONE_MINUS_SRC_ALPHA;

	InitColor(colors[start_index],             1.0f, 1.0f, 1.0f, 0.5f);
	InitColor(colors[restart_index],           0.3f, 0.3f, 0.3f, 0.5f);
	InitColor(colors[stop_index],              0.0f, 0.0f, 0.0f, 0.5f);
	InitColor(colors[wait_index],              0.5f, 0.5f, 0.5f, 0.5f);
	InitColor(colors[build_index],             0.0f, 1.0f, 0.0f, 0.5f);
	InitColor(colors[move_index],              0.5f, 1.0f, 0.5f, 0.5f);
	InitColor(colors[attack_index],            1.0f, 0.5f, 0.5f, 0.5f);
	InitColor(colors[fight_index],             0.5f, 0.5f, 1.0f, 0.5f);
	InitColor(colors[guard_index],             0.3f, 0.3f, 1.0f, 0.5f);
	InitColor(colors[patrol_index],            0.0f, 0.0f, 1.0f, 0.5f);
	InitColor(colors[capture_index],           1.0f, 1.0f, 0.3f, 0.5f);
	InitColor(colors[repair_index],            0.3f, 1.0f, 1.0f, 0.5f);
	InitColor(colors[reclaim_index],           1.0f, 0.2f, 1.0f, 0.5f);
	InitColor(colors[restore_index],           0.0f, 1.0f, 0.0f, 0.5f);
	InitColor(colors[resurrect_index],         0.2f, 0.6f, 1.0f, 0.5f);
	InitColor(colors[load_index],              0.3f, 1.0f, 1.0f, 0.5f);
	InitColor(colors[unload_index],            1.0f, 1.0f, 0.0f, 0.5f);
	InitColor(colors[rangeAttack_index],       1.0f, 0.3f, 0.3f, 0.7f);
	InitColor(colors[rangeBuild_index],        0.3f, 1.0f, 0.3f, 0.7f);
	InitColor(colors[rangeDecloak_index],      0.3f, 0.3f, 1.0f, 0.7f);
	InitColor(colors[rangeExtract_index],      1.0f, 0.3f, 0.3f, 0.7f);
	InitColor(colors[rangeKamikaze_index],     0.8f, 0.8f, 0.1f, 0.7f);
	InitColor(colors[rangeSelfDestruct_index], 0.8f, 0.1f, 0.1f, 0.7f);
	
	// setup the easy-access pointers
	start             = colors[start_index];
	restart           = colors[restart_index];
	stop              = colors[stop_index];
	wait              = colors[wait_index];
	build             = colors[build_index];
	move              = colors[move_index];
	attack            = colors[attack_index];
	fight             = colors[fight_index];
	guard             = colors[guard_index];
	patrol            = colors[patrol_index];
	capture           = colors[capture_index];
	repair            = colors[repair_index];
	reclaim           = colors[reclaim_index];
	restore           = colors[restore_index];
	resurrect         = colors[resurrect_index];
	load              = colors[load_index];
	unload            = colors[unload_index];
	rangeAttack       = colors[rangeAttack_index];
	rangeBuild        = colors[rangeBuild_index];
	rangeDecloak      = colors[rangeDecloak_index];
	rangeExtract      = colors[rangeExtract_index];
	rangeKamikaze     = colors[rangeKamikaze_index];
	rangeSelfDestruct = colors[rangeSelfDestruct_index];
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


bool CCommandColors::LoadConfig(const string& filename)
{
	CFileHandler ifs(filename);
	
	map<string, int> colorMap;

	colorMap["start"]             = start_index;
	colorMap["restart"]           = restart_index;
	colorMap["stop"]              = stop_index;
	colorMap["wait"]              = wait_index;
	colorMap["build"]             = build_index;
	colorMap["move"]              = move_index;
	colorMap["attack"]            = attack_index;
	colorMap["fight"]             = fight_index;
	colorMap["guard"]             = guard_index;
	colorMap["patrol"]            = patrol_index;
	colorMap["capture"]           = capture_index;
	colorMap["repair"]            = repair_index;
	colorMap["reclaim"]           = reclaim_index;
	colorMap["restore"]           = restore_index;
	colorMap["resurrect"]         = resurrect_index;
	colorMap["load"]              = load_index;
	colorMap["unload"]            = unload_index;
	colorMap["rangeattack"]       = rangeAttack_index;
	colorMap["rangebuild"]        = rangeBuild_index;
	colorMap["rangedecloak"]      = rangeDecloak_index;
	colorMap["rangeextract"]      = rangeExtract_index;
	colorMap["rangekamikaze"]     = rangeKamikaze_index;
	colorMap["rangeselfdestruct"] = rangeSelfDestruct_index;
	

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
			char* endPtr;
			const char* startPtr = words[1].c_str();
			restartAlpha = (float)strtod(startPtr, &endPtr);
			if (endPtr == startPtr) {
				restartAlpha = 0.5f;
			}
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
		else {
			// try to parse a color
			if (words.size() > 1) {
				map<string, int>::iterator it = colorMap.find(command);
				if (it != colorMap.end()) {
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
