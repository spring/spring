/*
 * dotfileHandler.cpp
 * dotfileHandler configuration class implementation
 * Copyright (C) 2005 Christopher Han <xiphux@gmail.com>
 */

#include "dotfileHandler.h"
#include <sstream>

dotfileHandler regHandler(DOTCONFIGPATH);

dotfileHandler::dotfileHandler(string filename)
{
	std::ifstream reader(filename.c_str());
	if (reader.good()) {
		string read;
		int idx;
		while (getline(reader,read))
			if ((idx = read.find_first_of("=")) > 0)
				data[read.substr(0,idx)] = read.substr(idx+1);
		reader.close();
	} else
		MessageBox(0,"Could not read from config file","dotfileHandler",MB_ICONEXCLAMATION);
	file.open(filename.c_str());
	if (!file)
		MessageBox(0,"Could not write to config file","dotfileHandler",MB_ICONEXCLAMATION);
}

dotfileHandler::~dotfileHandler()
{
	flushfile();
	file.close();
}

unsigned int dotfileHandler::GetInt(string name, unsigned int def)
{
	std::map<string,string>::iterator pos = data.find(name);
	return ( pos == data.end() ? def : atoi(data[name].c_str()) );
}

string dotfileHandler::GetString(string name, string def)
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
	file.seekp(ios::beg);
	for(std::map<string,string>::iterator iter = data.begin(); iter != data.end(); iter++)
		file << iter->first << "=" << iter->second << std::endl;
	file.flush();
}

void dotfileHandler::SetString(string name, string value)
{
	data[name] = value;
	flushfile();
}

void dotfileHandler::SetInt(string name, unsigned int value)
{
	std::stringstream ss;
	ss << value;
	data[name] = ss.str();
	flushfile();
}
