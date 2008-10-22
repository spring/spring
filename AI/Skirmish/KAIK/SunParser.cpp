#include "SunParser.h"

CSunParser::CSunParser(AIClasses* ai) {
	this->ai = ai;
}
CSunParser::~CSunParser() {
	DeleteSection(&sections);
}

void CSunParser::DeleteSection(map<string, SSection*>* section) {
	map<string, SSection*>::iterator ui;

	for (ui = section->begin(); ui != section->end(); ui++) {
		DeleteSection(&ui->second->sections);
		delete ui->second;
	}
}

void CSunParser::LoadVirtualFile(string filename) {
	this->filename = filename;
	int size = ai->cb->GetFileSize(filename.c_str());

	if (size == -1) {
		return;
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
}

void CSunParser::LoadRealFile(string filename) {
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

void CSunParser::LoadBuffer(char* buf, int size) {
    this->filename = "\'Buffer\'";

	try {
		Parse(buf,size);
	} catch(...) {
	}
}

void CSunParser::Parse(char* buf, int size) {
	string thissection;
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
			while ((buf != endptr) && buf[0] != '*' || buf[1] != '/') {
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
			map<string, SSection*>::iterator ui = sections.find(thissection);

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
	string thissection;
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
			map<string, SSection*>::iterator ui = section->sections.find(thissection);

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
			string name;
			string value;

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
string CSunParser::SGetValueMSG(string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	string value;

	SGetValue(value, location);
	return value;
}

// find value, return default value if no such value found
string CSunParser::SGetValueDef(string defaultvalue, string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	string value;

	bool found = SGetValue(value, location);

	if (!found) {
		value = defaultvalue;
	}

	return value;
}


//finds a value in the file, if not found returns false
// errormessages is returned in value
bool CSunParser::GetValue(string &value, void* amJustHereForIntelCompilerCompatibility, ...) {
	string searchpath;

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
	string svalue = sectionptr->values[arg];

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

	string mo = weapons->values["weapon1"];
}


// finds a value in the file , if not found returns false
// errormessage is returned in value, location of value is
// sent as a string "section\section\value"
bool CSunParser::SGetValue(string &value, string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	string searchpath;

	//split the location string
	vector<string> loclist = GetLocationVector(location);

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

	string svalue = sectionptr->values[loclist[loclist.size() - 1]];
	value = svalue;
	return true;
}

//return a map with all values in section
const map<string, string> CSunParser::GetAllValues(string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	map<string, string> emptymap;
	string searchpath;
	vector<string> loclist = GetLocationVector(location);

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
vector<string> CSunParser::GetSectionList(string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	vector<string> loclist = GetLocationVector(location);
	vector<string> returnvec;
	map<string, SSection*>* sectionsptr = &sections;

	if (loclist[0].compare("") != 0) {
 		string searchpath;
		for (unsigned int i = 0; i < loclist.size(); i++) {
			searchpath += loclist[i];

			if (sectionsptr->find(loclist[i]) == sectionsptr->end()) {
				return returnvec;
			}

			sectionsptr = &sectionsptr->find(loclist[i])->second->sections;
			searchpath += '\\';
		}
	}

	map<string,SSection*>::iterator it;
	for (it = sectionsptr->begin(); it != sectionsptr->end(); it++) {
		returnvec.push_back(it->first);
		transform(returnvec.back().begin(), returnvec.back().end(), returnvec.back().begin(), (int (*)(int)) tolower);
	}

	return returnvec;
}

bool CSunParser::SectionExist(string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	vector<string> loclist = GetLocationVector(location);

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

vector<string> CSunParser::GetLocationVector(string location) {
	transform(location.begin(), location.end(), location.begin(), (int (*)(int)) tolower);
	vector<string> loclist;

	unsigned long start = 0;
	unsigned long next = 0;

	while ((next = location.find_first_of("\\", start)) != std::string::npos) {
		loclist.push_back(location.substr(start, next - start));
		start = next + 1;
	}

	loclist.push_back(location.substr(start));

	return loclist;
}

float3 CSunParser::GetFloat3(float3 def, string location) {
	def = def;
	location = location;
	return float3(0, 0, 0);
}
