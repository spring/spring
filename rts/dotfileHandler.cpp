/*
 * dotfileHandler.cpp
 * dotfileHandler configuration class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#include "dotfileHandler.h"
#include "errorhandler.h"
#include <sstream>

dotfileHandler::dotfileHandler(const string filename)
{
	std::ifstream reader(filename.c_str());
	if (reader.good()) {
		int idx;
		string read;
		while (getline(reader,read))
			if ((idx = read.find_first_of("=")) > 0)
				data[read.substr(0,idx)] = read.substr(idx+1);
		reader.close();
	} else
		handleerror(0,"Could not read from config file","dotfileHandler",MB_ICONEXCLAMATION);
	file.open(filename.c_str());
	if (!file)
		handleerror(0,"Could not write to config file","dotfileHandler",0);
	flushfile();
}

dotfileHandler::~dotfileHandler()
{
	flushfile();
	file.close();
}

unsigned int dotfileHandler::GetInt(const string name, const unsigned int def)
{
	std::map<string,string>::iterator pos = data.find(name);
	return ( pos == data.end() ? def : atoi(data[name].c_str()) );
}

string dotfileHandler::GetString(const string name, const string def)
{
	std::map<string,string>::iterator pos = data.find(name);
	return ( pos == data.end() ? def : data[name] );
}

/*
 * Pretty hackish, but Windows registry changes
 * save immediately, while with dotfileHandler the
 * data is stored in an internal data structure for
 * fast access.  So we want to keep the settings in
 * the event of a crash
 */
void dotfileHandler::flushfile(void)
{
	file.seekp(std::ios::beg);
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); iter++)
		file << iter->first << "=" << iter->second << std::endl;
	file.flush();
}

void dotfileHandler::SetString(const string name, const string value)
{
	data[name] = value;
	flushfile();
}

void dotfileHandler::SetInt(const string name, const unsigned int value)
{
	std::stringstream ss;
	ss << value;
	data[name] = ss.str();
	flushfile();
}
