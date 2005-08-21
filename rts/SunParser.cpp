// SunParser.cpp: implementation of the CSunParser class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SunParser.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "FileHandler.h"
#include <algorithm>
#include <cctype>
#include "SunParser.h"
#include "InfoConsole.h"
//#include "mmgr.h"

//#pragma warning(disable:4786)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifndef USE_GLUT
extern HWND hWnd;
#endif

CSunParser::CSunParser()
{

}

CSunParser::~CSunParser()
{
	DeleteSection(&sections);
}

void CSunParser::DeleteSection(std::map<std::string,SSection*> *section)
{
	std::map<std::string, SSection*>::iterator ui;
	for(ui=section->begin();ui!=section->end();ui++)
	{
		DeleteSection(&ui->second->sections);
		delete ui->second;
	}
}

void CSunParser::LoadFile(std::string filename)
{
	transform(filename.begin(), filename.end(), filename.begin(), (int (*)(int))tolower);
	//DeleteSection(&sections);
	this->filename = filename;
	//FILE *pStream = fopen(filename.c_str(),"rb");
	CFileHandler file(filename);
	if(!file.FileExists()){
		MessageBox(hWnd, ("file " + filename + " not found").c_str(), "file error", MB_OK);
		return;
	}
	int size = file.FileSize();
	char *filebuf = new char[size+1];
	file.Read(filebuf, file.FileSize());
	filebuf[size] = '\n'; //append newline at end to avoid parsing error at eof
	try
	{
		Parse(filebuf, size);
	}
	catch(...)
	{
		MessageBox(hWnd, ("error parsing file " + filename).c_str(), "Sun parsing error", MB_OK);
	}
	delete[] filebuf;
}

void CSunParser::LoadBuffer(char* buf, int size)
{
	try
	{
		Parse(buf,size);
	}
	catch(...)
	{
		MessageBox(hWnd, ("Error parsing file " + filename).c_str(), "Sun parsing error", MB_OK);
	}
}

void CSunParser::Parse(char *buf, int size)
{
	std::string thissection;
	SSection *section = NULL;
	//std::vector<std::map<std::string,SSection*>*> sectionlist;
	//sectionlist.push_back(&sections);
	int se = 0; //for section start/end errorchecking
	char *endptr = buf+size;
	while(buf<endptr)
	{
		if(buf[0]=='/' && buf[1]=='/') //comment
		{
			while((buf != endptr) && *buf!='\n' && *buf!='\r')
			{
				buf++;
			}
		}
		else if(buf[0]=='/' && buf[1]=='*') //comment
		{
			while((buf != endptr) && buf[0]!='*' || buf[1]!='/')
			{
				buf++;
			}
		}
		else if(*buf == '[') //sectionname
		{
			thissection = "";
			while(*(++buf)!=']')
			{
				thissection += *buf;
			}
		}
		else if(*buf == '{') //section
		{
			buf++;
			section = new SSection;
			std::transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int))std::tolower);
			std::map<std::string, SSection*>::iterator ui=sections.find(thissection);
			if(ui!=sections.end()){
//				MessageBox(0,"Overwrote earlier section in sunparser",thissection.c_str(),0);
				DeleteSection(&ui->second->sections);
				delete ui->second;
			}
			sections[thissection] = section;
			buf = ParseSection(buf, endptr-buf, section);
		}
		
		// We can possible hit endptr from somewhere that increases, so don't go past it
		if (buf != endptr)
		buf++;
	}
}

char *CSunParser::ParseSection(char *buf, int size, SSection *section)
{
	std::string thissection;
	int se = 0; //for section start/end errorchecking
	char *endptr = buf+size;
	while(buf<endptr)
	{
		if(buf[0]=='/' && buf[1]=='/') //comment
		{
			while(*buf!='\n' && *buf!='\r' && buf<endptr)
			{
				buf++;
			}
		}
		else if(buf[0]=='/' && buf[1]=='*') //comment
		{
			while((buf[0]!='*' || buf[1]!='/')&&buf<endptr)
			{
				buf++;
			}
		}
		else if(*buf == '[') //sectionname
		{
			thissection = "";
			while(*(++buf)!=']' && buf<endptr)
			{
				thissection += *buf;
			}
		}
		else if(*buf == '{') //section
		{
			buf++;
			SSection *newsection = new SSection;
			std::transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int))std::tolower);
			std::map<std::string, SSection*>::iterator ui=section->sections.find(thissection);
			if(ui!=section->sections.end()){
				MessageBox(0,"Overwrote earlier section in sunparser",thissection.c_str(),0);
				DeleteSection(&ui->second->sections);
				delete ui->second;
			}
			section->sections[thissection] = newsection;
			buf = ParseSection(buf, endptr-buf, newsection);
		}
		else if(*buf == '}') //endsection
		{
			//buf++;
			return buf;
		}
		else if(*buf >= '0' && *buf <= 'z')
		{
			std::string name;
			std::string value;
			while(*buf != '=' && buf<endptr)
			{
				name += *buf;
				buf++;
			}
			buf++;
			while(*buf != ';' && buf<endptr)
			{
				value += *buf;
				buf++;
			}
			std::transform(name.begin(), name.end(), name.begin(), (int (*)(int))std::tolower);
			section->values[name] = value;

		}
		buf++;
	}
	return buf;
}

//find value, display messagebox if no such value found
std::string CSunParser::SGetValueMSG(std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::string value;
	bool found = SGetValue(value, location);
	if(!found)
		MessageBox(hWnd, value.c_str(), "Sun parsing error", MB_OK);
	return value;
}

//find value, return default value if no such value found
std::string CSunParser::SGetValueDef(std::string defaultvalue, std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::string value;
	bool found = SGetValue(value, location);
	if(!found)
		value = defaultvalue;
	return value;
}

//finds a value in the file, if not found returns false, and errormessages is returned in value
bool CSunParser::GetValue(std::string &value, ...)
{
	std::string searchpath; //for errormessages
	va_list loc;
	va_start(loc, value);
	int numargs=0;
	while(va_arg(loc, char*)) //determine number of arguments
		numargs++;
	va_start(loc, value);
	SSection *sectionptr;
	for(int i=0; i<numargs-1; i++)
	{
		char *arg = va_arg(loc, char*);
		searchpath += '\\';
		searchpath += arg;
		sectionptr = sections[arg];
		if(sectionptr==NULL)
		{
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}
	}
	char *arg = va_arg(loc, char*);
	std::string svalue = sectionptr->values[arg];
	searchpath += '\\';
	searchpath += arg;
	if(svalue == "")
	{
		value = "Value " + searchpath + " missing in file " + filename;
		return false;
	}
	value = svalue;
	return true;
}

void CSunParser::Test()
{
	SSection *unitinfo = sections["UNITINFO"];
	SSection *weapons = unitinfo->sections["WEAPONS"];
	std::string mo = weapons->values["weapon1"];
}

//finds a value in the file , if not found returns false, and errormessages is returned in value
//location of value is sent as a string "section\section\value"
bool CSunParser::SGetValue(std::string &value, std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::string searchpath; //for errormessages
	//split the location string
	std::vector<std::string> loclist = GetLocationVector(location);
	if(sections.find(loclist[0]) == sections.end())
	{
		value = "Section " + loclist[0] + " missing in file " + filename;
		return false;
	}
	SSection *sectionptr = sections[loclist[0]];
	searchpath = loclist[0];
	for(unsigned int i=1; i<loclist.size()-1; i++)
	{
		//const char *arg = loclist[i].c_str();
		searchpath += '\\';
		searchpath += loclist[i];
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	searchpath += '\\';
	searchpath += loclist[loclist.size()-1];
	if(sectionptr->values.find(loclist[loclist.size()-1]) == sectionptr->values.end())
	{
		value = "Value " + searchpath + " missing in file " + filename;
		return false;
	}
        std::string svalue = sectionptr->values[loclist[loclist.size()-1]];
	value = svalue;
	return true;
}

//return a map with all values in section
const std::map<std::string, std::string> CSunParser::GetAllValues(std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::map<std::string, std::string> emptymap;
	std::string searchpath; //for errormessages
	std::vector<std::string> loclist = GetLocationVector(location);	
	if(sections.find(loclist[0]) == sections.end())
	{
		MessageBox(hWnd, ("Section " + loclist[0] + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
		return emptymap;
	}
	SSection *sectionptr = sections[loclist[0]];
	searchpath = loclist[0];
	for(unsigned int i=1; i<loclist.size(); i++)
	{
		searchpath += '\\';
		searchpath += loclist[i];
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			MessageBox(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
			return emptymap;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	return sectionptr->values;
}

//return vector with all section names in it
std::vector<std::string> CSunParser::GetSectionList(std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::vector<std::string> loclist = GetLocationVector(location);
	std::vector<std::string> returnvec;
	std::map<std::string,SSection*> *sectionsptr = &sections;
	if(loclist[0].compare("")!=0)
	{
 		std::string searchpath;// = loclist[0];
		for(unsigned int i=0; i<loclist.size(); i++)
		{
			searchpath += loclist[i];
			if(sectionsptr->find(loclist[i]) == sectionsptr->end())
			{
				MessageBox(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
        			return returnvec;
			}
			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
        		searchpath += '\\';
		}       
	}
	std::map<std::string,SSection*>::iterator it;
	for(it=sectionsptr->begin(); it!=sectionsptr->end(); it++)
	{
		returnvec.push_back(it->first);
		std::transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int))std::tolower);
	}
	return returnvec;
}

bool CSunParser::SectionExist(std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::vector<std::string> loclist = GetLocationVector(location);		
	if(sections.find(loclist[0]) == sections.end())
	{
		return false;
	}
	SSection *sectionptr = sections[loclist[0]];
	for(unsigned int i=1; i<loclist.size(); i++)
	{
		if(sectionptr->sections.find(loclist[i]) == sectionptr->sections.end())
		{
			return false;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}
	return true;
}

std::vector<std::string> CSunParser::GetLocationVector(std::string location)
{
	std::transform(location.begin(), location.end(), location.begin(), (int (*)(int))std::tolower);
	std::vector<std::string> loclist;
	int start = 0;
	int next = 0;

	while((next = location.find_first_of("\\", start)) != std::string::npos)
	{
		loclist.push_back(location.substr(start, next-start));
		start = next+1;
	}
	loclist.push_back(location.substr(start));

    return loclist;
}

/*
template<typename T>
void CSunParser::GetMsg(T& value, const std::string& key)
{
	std::string str;
	str = SGetValueMSG(key);
	std::stringstream stream;
	stream << str;
	stream >> value;
}

template<typename T>
void CSunParser::GetDef(T& value, const std::string& key, const std::string& defvalue)
{
	std::string str;
	str = SGetValueDef(key, defvalue);
	std::stringstream stream;
	stream << str;
	stream >> value;
}*/

float3 CSunParser::GetFloat3(float3 def, std::string location)
{
	std::string s=SGetValueDef("",location);
	if(s.empty())
		return def;
	float3 ret;
	ParseArray(s,ret.xyz,3);
	return ret;
}

