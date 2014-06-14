/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <vector>
#include <stdio.h>

#include "DataDirsAccess.h"
#include "DataDirLocater.h"
#include "FileQueryFlags.h"
#include "System/Log/ILog.h"

#include <zlib.h>

class RapidEntry{
public:
	RapidEntry(const std::string& line) {
		value.resize(entries);
		size_t pos = 0, start = 0;
		for(size_t i=0; i<entries; i++) {
			pos = line.find(delim, start);
			value[i] = (line.substr(start, pos - start));
			start = pos + 1;
		}
	}
	const std::string& GetTag()
	{
		return value[0];
	}

	const std::string& GetName()
	{
		return value[3];
	}
private:
	static const char delim = ',';
	static const size_t entries = 4;
	std::vector<std::string> value;

};


static int bufsize = 4096;

static std::string GetNameFromFile(const std::string& tag, const std::string& name)
{
	gzFile in = gzopen(name.c_str(), "rb");
	if (in == NULL) {
		LOG_L(L_ERROR, "couldn't open %s", name.c_str());
		return "";
	}

	char buf[bufsize];
	while (gzgets(in, buf, bufsize)!=NULL){
		size_t len = strnlen(buf, bufsize);
		if (len <= 2) continue; //line to short/invalid, skip
		if (buf[len-1] == '\n') len--;
		if (buf[len-1] == '\r') len--;

		RapidEntry ent(std::string(buf, len));
		if (ent.GetTag() == tag) {
			return ent.GetName();
		}
	}
	gzclose(in);
	return "";
}

std::string GetRapidName(const std::string& tag)
{
	std::string res;
	const std::vector<DataDir>& dirs = dataDirLocater.GetDataDirs();
	for(DataDir dir: dirs) {
		std::string path = dir.path + "rapid"; //TODO: does GetDataDirs() ensure paths to end with a delimn?
		std::vector<std::string> files = dataDirsAccess.FindFiles(path, "versions.gz", FileQueryFlags::RECURSE);
		for(const std::string file: files) {
			printf("%s\n", file.c_str());
			res = GetNameFromFile(tag, file);
			if (!res.empty())
				return res;
		}
	}
	return res;
}

