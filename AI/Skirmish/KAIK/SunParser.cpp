#include <fstream>
#include <cstdarg>

#include "SunParser.h"
#include "IncExternAI.h"
#include "Containers.h"

CSunParser::CSunParser(AIClasses* ai) {
	this->ai = ai;
}
CSunParser::~CSunParser() {
	DeleteSection(&sections);
}

void CSunParser::DeleteSection(std::map<std::string, SSection*>* section) {
	std::map<std::string, SSection*>::iterator ui;

	for (ui = section->begin(); ui != section->end(); ui++) {
		DeleteSection(&ui->second->sections);
		delete ui->second;
	}
}

bool CSunParser::LoadVirtualFile(std::string filename) {
	this->filename = filename;
	int size = ai->cb->GetFileSize(filename.c_str());

	if (size == -1) {
		return false;
	}

	char* filebuf = new char[size + 1];
	ai->cb->ReadFile(filename.c_str(), filebuf, size);
	 // append newline at end to avoid parsing error at EOF
	filebuf[size] = '\0';

	try {
		Parse(filebuf, size);
	} catch(...) {
	}

	delete[] filebuf;
	return true;
}

/*
void CSunParser::LoadRealFile(std::string filename) {
	char filename_buf[1024];
	strcpy(filename_buf, filename.c_str());
	ai->cb->GetValue(AIVAL_LOCATE_FILE_R, filename_buf);

	this->filename = filename_buf;
	std::ifstream RealFile(filename_buf);

	if (RealFile.fail()) {
		return;
	}

	RealFile.seekg(0, std::ios_base::end);
	int size = RealFile.tellg();
	RealFile.seekg(0, std::ios_base::beg);

	char* filebuf = new char[size + 1];
	RealFile.get(filebuf, size, '\0');
	RealFile.close();
	// append newline at end to avoid parsing error at EOF
	filebuf[size] = '\0';

	try {
		Parse(filebuf, size);
	} catch(...) {
	}

	delete[] filebuf;
}
*/

void CSunParser::LoadBuffer(char* buf, int size) {
    this->filename = "\'Buffer\'";

	try {
		Parse(buf,size);
	} catch(...) {
	}
}

void CSunParser::Parse(char* buf, int size) {
	std::string thissection;
	SSection* section = NULL;

	char* endptr = buf + size;

	while (buf<=endptr) {
		if (buf[0] == '/' && buf[1] == '/') {
			// comment
			while ((buf != endptr) && *buf!='\n' && *buf!='\r') {
				buf++;
			}
		}
		else if (buf[0] == '/' && buf[1] == '*') {
			// comment
			while (((buf != endptr) && (buf[0] != '*')) || (buf[1] != '/')) {
				buf++;
			}
		}
		else if (*buf == '[') {
			// sectionname
			thissection = "";

			while (*(++buf) != ']') {
				thissection += *buf;
			}
		}
		else if (*buf == '{') {
			// section
			buf++;
			section = new SSection;
			transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int)) tolower);
			std::map<std::string, SSection*>::iterator ui = sections.find(thissection);

			if (ui != sections.end()) {
				DeleteSection(&ui->second->sections);
				delete ui->second;
			}
			sections[thissection] = section;
			buf = ParseSection(buf, endptr - buf, section);
		}

		// We can possible hit endptr from somewhere that increases, so don't go past it
		if (buf <= endptr)
			buf++;
	}
}

char* CSunParser::ParseSection(char* buf, int size, SSection* section) {
	std::string thissection;
	char* endptr = buf + size;

	while (buf <= endptr) {

		if (buf[0] == '/' && buf[1] == '/') {
			// comment
			while (*buf != '\n' && *buf != '\r') {
				buf++;
			}
		}

		else if (buf[0] == '/' && buf[1] == '*') {
			// comment
			while (buf[0] != '*' || buf[1] != '/') {
				buf++;
			}
		}

		else if (*buf == '[') {
			// sectionname
			thissection = "";

			while (*(++buf) != ']') {
				thissection += *buf;
			}
		}

		else if (*buf == '{') {
			// section
			buf++;
			SSection* newsection = new SSection;
			transform(thissection.begin(), thissection.end(), thissection.begin(), (int (*)(int)) tolower);
			std::map<std::string, SSection*>::iterator ui = section->sections.find(thissection);

			if (ui != section->sections.end()) {
				DeleteSection(&ui->second->sections);
				delete ui->second;
			}

			section->sections[thissection] = newsection;
			buf = ParseSection(buf, endptr - buf, newsection);
		}

		else if (*buf == '}') {
			// endsection
			// buf++;
			return buf;
		}

		else if (*buf >= '0' && *buf <= 'z') {
			std::string name;
			std::string value;

			while (*buf != '=') {
				name += *buf;
				buf++;
			}

			buf++;

			while (*buf != ';') {
				value += *buf;
				buf++;
			}

			transform(name.begin(), name.end(), name.begin(), (int (*)(int)) tolower);
			section->values[name] = value;
		}

		buf++;
	}

	return buf;
}

// find value, display messagebox if no such value found
std::string CSunParser::SGetValueMSG(std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::string value;

	SGetValue(value, location);
	return value;
}

// find value, return default value if no such value found
std::string CSunParser::SGetValueDef(std::string defaultvalue, std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::string value;

	bool found = SGetValue(value, location);

	if (!found) {
		value = defaultvalue;
	}

	return value;
}


//finds a value in the file, if not found returns false
// errormessages is returned in value
bool CSunParser::GetValue(std::string& value, void* amJustHereForIntelCompilerCompatibility, ...) {
	std::string searchpath;

	va_list loc;
	va_start(loc, amJustHereForIntelCompilerCompatibility);
	int numargs = 0;

	while (va_arg(loc, char*)) {
		// determine number of arguments
		numargs++;
	}

	va_start(loc, amJustHereForIntelCompilerCompatibility);
	SSection* sectionptr = 0x0;

	for (int i = 0; i < numargs - 1; i++) {
		char* arg = va_arg(loc, char*);

		searchpath += '\\';
		searchpath += arg;
		sectionptr = sections[arg];

		if (sectionptr == NULL) {
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}
	}

	char* arg = va_arg(loc, char*);
	std::string svalue = sectionptr->values[arg];

	searchpath += '\\';
	searchpath += arg;

	if (svalue == "") {
		value = "Value " + searchpath + " missing in file " + filename;
		return false;
	}

	value = svalue;
	return true;
}

void CSunParser::Test() {
	SSection* unitinfo = sections["UNITINFO"];
	SSection* weapons = unitinfo->sections["WEAPONS"];

	std::string mo = weapons->values["weapon1"];
}


// finds a value in the file , if not found returns false
// errormessage is returned in value, location of value is
// sent as a string "section\section\value"
bool CSunParser::SGetValue(std::string& value, std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::string searchpath;

	//split the location string
	std::vector<std::string> loclist = GetLocationVector(location);

	if (sections.find(loclist[0]) == sections.end()) {
		value = "Section " + loclist[0] + " missing in file " + filename;
		return false;
	}

	SSection* sectionptr = sections[loclist[0]];
	searchpath = loclist[0];

	for (unsigned int i = 1; i < loclist.size() - 1; i++) {
		searchpath += '\\';
		searchpath += loclist[i];

		if (sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()) {
			value = "Section " + searchpath + " missing in file " + filename;
			return false;
		}
		sectionptr = sectionptr->sections[loclist[i]];
	}

	searchpath += '\\';
	searchpath += loclist[loclist.size()-1];

	if (sectionptr->values.find(loclist[loclist.size()-1]) == sectionptr->values.end()) {
		value = "Value " + searchpath + " missing in file " + filename;

		return false;
	}

	std::string svalue = sectionptr->values[loclist[loclist.size() - 1]];
	value = svalue;
	return true;
}

//return a map with all values in section
const std::map<std::string, std::string> CSunParser::GetAllValues(std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::map<std::string, std::string> emptymap;
	std::string searchpath;
	std::vector<std::string> loclist = GetLocationVector(location);

	if (sections.find(loclist[0]) == sections.end()) {
		return emptymap;
	}

	SSection* sectionptr = sections[loclist[0]];
	searchpath = loclist[0];

	for (unsigned int i = 1; i < loclist.size(); i++) {
		searchpath += '\\';
		searchpath += loclist[i];

		if (sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()) {
			return emptymap;
		}

		sectionptr = sectionptr->sections[loclist[i]];
	}

	return sectionptr->values;
}

// return vector with all section names in it
std::vector<std::string> CSunParser::GetSectionList(std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::vector<std::string> loclist = GetLocationVector(location);
	std::vector<std::string> returnvec;
	std::map<std::string, SSection*>* sectionsptr = &sections;

	if (loclist[0].compare("") != 0) {
 		std::string searchpath;
		for (unsigned int i = 0; i < loclist.size(); i++) {
			searchpath += loclist[i];

			if (sectionsptr->find(loclist[i]) == sectionsptr->end()) {
				return returnvec;
			}

			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
			searchpath += '\\';
		}
	}

	std::map<std::string, SSection*>::iterator it;
	for (it = sectionsptr->begin(); it != sectionsptr->end(); it++) {
		returnvec.push_back(it->first);
		transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int)) tolower);
	}

	return returnvec;
}

bool CSunParser::SectionExist(std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::vector<std::string> loclist = GetLocationVector(location);

	if (sections.find(loclist[0]) == sections.end()) {
		return false;
	}

	SSection* sectionptr = sections[loclist[0]];

	for (unsigned int i = 1; i < loclist.size(); i++) {
		if (sectionptr->sections.find(loclist[i]) == sectionptr->sections.end()) {
			return false;
		}

		sectionptr = sectionptr->sections[loclist[i]];
	}

	return true;
}

std::vector<std::string> CSunParser::GetLocationVector(std::string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	std::vector<std::string> loclist;

	unsigned long start = 0;
	unsigned long next = 0;

	while ((next = location.find_first_of("\\", start)) != std::string::npos) {
		loclist.push_back(location.substr(start, next - start));
		start = next + 1;
	}

	loclist.push_back(location.substr(start));

	return loclist;
}

float3 CSunParser::GetFloat3(float3 def, std::string location) {
	def = def;
	location = location;
	return float3(0, 0, 0);
}
