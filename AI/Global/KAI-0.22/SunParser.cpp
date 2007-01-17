// SunParser.cpp: implementation of the CSunParser class.
//
//////////////////////////////////////////////////////////////////////

#include "SunParser.h"


#pragma warning(disable:4786)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// extern HWND hWnd;

CSunParser::CSunParser(AIClasses* ai)
{
  this->ai=ai;
}
CSunParser::~CSunParser()
{
    //L("CSunParser::~CSunParser()");
	DeleteSection(&sections);
	//L("~");
}
void CSunParser::DeleteSection(map<string,SSection*> *section)
{
    //L("CSunParser::DeleteSection(" << section << ")" << endl);
	map<string, SSection*>::iterator ui;
	for(ui=section->begin();ui!=section->end();ui++)
	{
		DeleteSection(&ui->second->sections);
		delete ui->second;
	}
}
void CSunParser::LoadVirtualFile(string filename)
{
    //L("CSunParser::LoadVirtualFile(" << filename << ")" << endl);
	//DeleteSection(&sections);

	this->filename = filename;

	//FILE *pStream = fopen(filename.c_str(),"rb");
/*
	CFileHandler file(filename);
	if(!file.FileExists()){
		MessageBox(hWnd, ("file " + filename + " not found").c_str(), "file error", MB_OK);
		return;
	}
	int size = file.FileSize();
*/
    int size = ai->cb->GetFileSize(filename.c_str());
    if(size == -1)
    {
    	//L("Sun Parsing Error: File " << filename << " not found" << endl);
        return;
    }
	char *filebuf = new char[size+1];
//	file.Read(filebuf, file.FileSize());
	ai->cb->ReadFile(filename.c_str(), filebuf, size);

	filebuf[size] = '\0'; //append newline at end to avoid parsing error at eof

	try
	{
		Parse(filebuf, size);
	}
	catch(...)
	{
		// MessageBox(hWnd, ("error parsing file " + filename).c_str(), "Sun parsing error", MB_OK);
        //L("Sun Parsing Error: Error parsing virtual file " << filename << endl);
	}

	delete[] filebuf;
}
void CSunParser::LoadRealFile(string filename)
{
    //L("CSunParser::LoadRealFile(" << filename << ")" << endl);
	char filename_buf[1000];
	strcpy(filename_buf, filename.c_str());
	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename_buf);
	this->filename = filename_buf;
    /*
	FILE *RealFile = fopen(filename_buf, "r");
	if(RealFile == NULL)
    {
    	//L("Sun Parsing Error: File " << filename << " not found" << endl);
        return;
    }
    */
    ifstream RealFile(filename_buf);
    if(RealFile.fail())
    {
    	//L("Sun Parsing Error: File " << filename << " not found" << endl);
        return;
    }

    RealFile.seekg(0, ios_base::end);
    int size = RealFile.tellg();
    RealFile.seekg(0, ios_base::beg);
    //L("Size = " << size);
	char *filebuf = new char[size+1];
    RealFile.get(filebuf, size, '\0');
    RealFile.close();
	filebuf[size] = '\0'; //append newline at end to avoid parsing error at eof

	try
	{
		Parse(filebuf, size);
	}
	catch(...)
	{
		// MessageBox(hWnd, ("error parsing file " + filename).c_str(), "Sun parsing error", MB_OK);
        //L("Sun Parsing Error: Error parsing file " << filename << endl);
	}

	delete[] filebuf;
}
void CSunParser::LoadBuffer(char* buf, int size)
{
    //L("CSunParser::LoadBuffer(" << &buf << ", " << size << ")" << endl);
    this->filename = "\'Buffer\'";
	try
	{
		Parse(buf,size);
	}
	catch(...)
	{
		//MessageBox(hWnd, ("error parsing file " + filename).c_str(), "Sun parsing error", MB_OK);
        //L("Sun Parsing Error: Error parsing file " << filename << endl);
	}
}
void CSunParser::Parse(char *buf, int size)
{
    //L("CSunParser::Parse(buf, " << size << ")" << endl);
	string thissection;
	SSection *section = NULL;

	//vector<map<string,SSection*>*> sectionlist;
	//sectionlist.push_back(&sections);

	int se = 0; //for section start/end errorchecking

	char *endptr = buf+size;

	while(buf<=endptr)
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
			transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int))tolower);
			map<string, SSection*>::iterator ui=sections.find(thissection);
			if(ui!=sections.end()){
//				MessageBox(0,"Overwrote earlier section in sunparser",thissection.c_str(),0);
                //L("Sun Parsing Error: Overwrote earlier section in sunparser: " << thissection << endl);
				DeleteSection(&ui->second->sections);
				delete ui->second;
			}
			sections[thissection] = section;
			buf = ParseSection(buf, endptr-buf, section);

		}

		// We can possible hit endptr from somewhere that increases, so don't go past it
		if (buf <= endptr)
			buf++;
	}
	//L("Finished Parsing" << endl);
}
char *CSunParser::ParseSection(char *buf, int size, SSection *section)
{
    //L("CSunParser::ParseSection(buf, " << size << ", " << section << ")" << endl);
	string thissection;

	int se = 0; //for section start/end errorchecking

	char *endptr = buf+size;

	while(buf<=endptr)
	{

		if(buf[0]=='/' && buf[1]=='/') //comment
		{
			while(*buf!='\n' && *buf!='\r')
			{

				buf++;
			}
		}
		else if(buf[0]=='/' && buf[1]=='*') //comment
		{
			while(buf[0]!='*' || buf[1]!='/')
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
			SSection *newsection = new SSection;
			transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int))tolower);
			map<string, SSection*>::iterator ui=section->sections.find(thissection);
			if(ui!=section->sections.end()){
//				MessageBox(0,"Overwrote earlier section in sunparser",thissection.c_str(),0);
                //L("Sun Parsing Error: Overwrote earlier section in sunparser: " << thissection << endl);
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
			string name;
			string value;

			while(*buf != '=')
			{
				name += *buf;
				buf++;
			}
			buf++;
			while(*buf != ';')
			{
				value += *buf;
				buf++;
			}
			transform(name.begin(), name.end(), name.begin(), (int (*)(int))tolower);
			//L("Attempting " << name << " = " << value << endl);
			section->values[name] = value;
			//L(name << " = " << value << endl);
		}

		buf++;
	}

	return buf;
}
//find value, display messagebox if no such value found
string CSunParser::SGetValueMSG(string location)
{
    //L("CSunParser::SGetValueMSG(" << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	string value;

	bool found = SGetValue(value, location);

	if(!found)
	{
//        MessageBox(hWnd, value.c_str(), "Sun parsing error", MB_OK);
        //L("Sun Parsing Error: Value " << value << " not found" << endl);
	}
	return value;
}
//find value, return default value if no such value found
string CSunParser::SGetValueDef(string defaultvalue, string location)
{
    //L("CSunParser::SGetValueDef(" << defaultvalue << ", " << location << "): ");
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	string value;

	bool found = SGetValue(value, location);

	if(!found)
	{
		//L("Using Default: ");
		value = defaultvalue;
	}
	return value;
}
//finds a value in the file, if not found returns false, and errormessages is returned in value
bool CSunParser::GetValue(string &value, ...)
{
    //L("CSunParser::GetValue(" << value << ")" << endl);

	string searchpath; //for errormessages

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
	string svalue = sectionptr->values[arg];

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
    //L("CSunParser::Test()" << endl);
	SSection *unitinfo = sections["UNITINFO"];
	SSection *weapons = unitinfo->sections["WEAPONS"];

	string mo = weapons->values["weapon1"];
}
//finds a value in the file , if not found returns false, and errormessages is returned in value
//location of value is sent as a string "section\section\value"
bool CSunParser::SGetValue(string &value, string location)
{
    //L("CSunParser::SGetValue(" << value << ", " << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	string searchpath; //for errormessages

	//split the location string
	vector<string> loclist = GetLocationVector(location);

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

	string svalue = sectionptr->values[loclist[loclist.size()-1]];
	value = svalue;
	return true;

}
//return a map with all values in section
const map<string, string> CSunParser::GetAllValues(string location)
{
    //L("CSunParser::GetAllValues(" << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	map<string, string> emptymap;
	string searchpath; //for errormessages

	vector<string> loclist = GetLocationVector(location);


	if(sections.find(loclist[0]) == sections.end())
	{
//		MessageBox(hWnd, ("Section " + loclist[0] + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
        //L("Sun Parsing Error: Section " << loclist[0] << " missing in file " << filename << endl);
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
//			MessageBox(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
            //L("Sun Parsing Error: Section " << searchpath << " missing in file " << filename << endl);
			return emptymap;
		}

		sectionptr = sectionptr->sections[loclist[i]];
	}

	return sectionptr->values;
}
//return vector with all section names in it
vector<string> CSunParser::GetSectionList(string location)
{
    //L("CSunParser::GetSectionList(" << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	vector<string> loclist = GetLocationVector(location);

	vector<string> returnvec;

	map<string,SSection*> *sectionsptr = &sections;

	if(loclist[0].compare("")!=0)
	{
 		string searchpath;// = loclist[0];
		for(unsigned int i=0; i<loclist.size(); i++)
		{
			searchpath += loclist[i];

			if(sectionsptr->find(loclist[i]) == sectionsptr->end())
			{
//				MessageBox(hWnd, ("Section " + searchpath + " missing in file " + filename).c_str(), "Sun parsing error", MB_OK);
                //L("Sun Parsing Error: Section " << searchpath << " missing in file " << filename << endl);
				return returnvec;
			}

			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;

			searchpath += '\\';
		}
	}

	map<string,SSection*>::iterator it;
	for(it=sectionsptr->begin(); it!=sectionsptr->end(); it++)
	{
		returnvec.push_back(it->first);
		transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int))tolower);
	}

	return returnvec;
}
bool CSunParser::SectionExist(string location)
{
    //L("CSunParser::SectionExist(" << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	vector<string> loclist = GetLocationVector(location);

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
vector<string> CSunParser::GetLocationVector(string location)
{
    //L("CSunParser::GetLocationVector(" << location << ")" << endl);
	transform(location.begin(), location.end(), location.begin(), (int (*)(int))tolower);
	vector<string> loclist;
	int start = 0;
	int next = 0;
	static const basic_string <char>::size_type npos = -1;

	while((next = location.find_first_of("\\", start)) != npos)
	{
		loclist.push_back(location.substr(start, next-start));
		start = next+1;
	}
	loclist.push_back(location.substr(start, -1));

    return loclist;
}
/*
template<typename T>
void CSunParser::GetMsg(T& value, const string& key)
{
	string str;
	str = SGetValueMSG(key);

	stringstream stream;
	stream << str;
	stream >> value;
}

template<typename T>
void CSunParser::GetDef(T& value, const string& key, const string& defvalue)
{
	string str;
	str = SGetValueDef(key, defvalue);

	stringstream stream;
	stream << str;
	stream >> value;
	}*/
float3 CSunParser::GetFloat3(float3 def, string location)
{
  /*  //L("CSunParser::GetFloat3((" << def.x << ", " << def.y << ", " << def.z << "), " << location << ")" << endl);
	string s=SGetValueDef("",location);
	if(s.empty())
		return def;

	float3 ret;
	ParseArray(s,ret.xyz,3);
	return ret;*/ // Temporarily disabled because of xyz error
	return float3(0,0,0);
}
