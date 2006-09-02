/**
 * @file DotfileHandler.cpp
 * @brief Linux dotfile config implementation
 * @author Christopher Han <xiphux@gmail.com>
 * 
 * DotfileHandler configuration class implementation
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#include "StdAfx.h"
#include "DotfileHandler.h"
#include "Platform/errorhandler.h"
#include <sstream>

/**
 * Attempts to open an existing config file.
 * If none exists, create one
 */
DotfileHandler::DotfileHandler(const string fname)
{
	std::ifstream reader(fname.c_str());
	if (reader.good()) {
		int idx;
		string read;
		while (getline(reader,read))
			if ((idx = read.find_first_of("=")) > 0)
				data[read.substr(0,idx)] = read.substr(idx+1);
		reader.close();
	}
	file.open(fname.c_str());
	if (!file)
		handleerror(0,"Could not write to config file","DotfileHandler",0);
	filename = fname;
	flushfile();
}

/**
 * Flushes all changes and closes file
 */
DotfileHandler::~DotfileHandler()
{
	flushfile();
	file.close();
}

/**
 * Gets integer value from config
 */
int DotfileHandler::GetInt(const string name, const int def)
{
	std::map<string,string>::iterator pos = data.find(name);
	if (pos == data.end()) {
		SetInt(name, def);
		return def;
	}
	return atoi(pos->second.c_str());
}

/**
 * Gets string value from config
 */
string DotfileHandler::GetString(const string name, const string def)
{
	std::map<string,string>::iterator pos = data.find(name);
	if (pos == data.end()) {
		SetString(name, def);
		return def;
	}
	return pos->second;
}

/**
 * Closes and reopens the file, truncating it to 0
 * (to prevent extra trailing characters)
 */
void DotfileHandler::truncatefile(void)
{
	if (file)
		file.close();
	file.open(filename.c_str(),std::ios::out|std::ios::trunc);
}

/**
 * Flush changes to disk
 *
 * Pretty hackish, but Windows registry changes
 * save immediately, while with DotfileHandler the
 * data is stored in an internal data structure for
 * fast access.  So we want to keep the settings in
 * the event of a crash
 */
void DotfileHandler::flushfile(void)
{
	truncatefile();
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); iter++)
		file << iter->first << "=" << iter->second << std::endl;
	file.flush();
}

/**
 * Sets a config string
 */
void DotfileHandler::SetString(const string name, const string value)
{
	data[name] = value;
	flushfile();
}

/**
 * Sets a config integer
 */
void DotfileHandler::SetInt(const string name, const int value)
{
	std::stringstream ss;
	ss << value;
	data[name] = ss.str();
	flushfile();
}
