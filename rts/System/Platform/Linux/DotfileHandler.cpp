/*
 * DotfileHandler.cpp
 * DotfileHandler configuration class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#include "StdAfx.h"
#include "DotfileHandler.h"
#include "Platform/errorhandler.h"
#include <sstream>

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
	} else
		handleerror(0,"Could not read from config file","DotfileHandler",MBF_EXCL);
	file.open(fname.c_str());
	if (!file)
		handleerror(0,"Could not write to config file","DotfileHandler",0);
	filename = fname;
	flushfile();
}

DotfileHandler::~DotfileHandler()
{
	flushfile();
	file.close();
}

unsigned int DotfileHandler::GetInt(const string name, const unsigned int def)
{
	std::map<string,string>::iterator pos = data.find(name);
	return ( pos == data.end() ? def : atoi(data[name].c_str()) );
}

string DotfileHandler::GetString(const string name, const string def)
{
	std::map<string,string>::iterator pos = data.find(name);
	return ( pos == data.end() ? def : data[name] );
}

void DotfileHandler::truncatefile(void)
{
	if (file)
		file.close();
	file.open(filename.c_str(),std::ios::out|std::ios::trunc);
}

/*
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

void DotfileHandler::SetString(const string name, const string value)
{
	data[name] = value;
	flushfile();
}

void DotfileHandler::SetInt(const string name, const unsigned int value)
{
	std::stringstream ss;
	ss << value;
	data[name] = ss.str();
	flushfile();
}
