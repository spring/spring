/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>
#include <vector>

#include "DataDirsAccess.h"
#include "FileQueryFlags.h"
#include "System/Log/ILog.h"

#include <zlib.h>
#include <cstring> //strnlen

static const int bufsize = 4096;

class RapidEntry{
public:
	RapidEntry() {
		value.resize(entries);
	}

	RapidEntry(const std::string& line) {
		value.resize(entries);
		size_t pos = 0, start = 0;
		for(size_t i=0; i<entries; i++) {
			pos = line.find(delim, start);
			value[i] = (line.substr(start, pos - start));
			start = pos + 1;
		}
	}

	const std::string& GetTag() const
	{
		return value[0];
	}

	const std::string& GetPackageHash() const
	{
		return value[1];
	}

	const std::string& GetParentGameName() const
	{
		return value[2];
	}

	const std::string& GetName() const
	{
		return value[3];
	}
private:
	static const char delim = ',';
	static const size_t entries = 4;
	std::vector<std::string> value;

};



template <typename Lambda>
static bool GetRapidEntry(const std::string& file, RapidEntry* re, Lambda p)
{
	gzFile in = gzopen(file.c_str(), "rb");
	if (in == nullptr) {
		LOG_L(L_ERROR, "couldn't open %s", file.c_str());
		return false;
	}

	char buf[bufsize];
	while (gzgets(in, buf, bufsize) != nullptr) {
		size_t len = strnlen(buf, bufsize);
		if (len <= 2) continue; //line to short/invalid, skip
		if (buf[len-1] == '\n') len--;
		if (buf[len-1] == '\r') len--;

		*re = RapidEntry(std::string(buf, len));
		if (p(*re)) {
			gzclose(in);
			return true;
		}
	}
	gzclose(in);
	return false;
}




std::string GetRapidPackageFromTag(const std::string& tag)
{
	for (const std::string& file: dataDirsAccess.FindFiles("rapid", "versions.gz", FileQueryFlags::RECURSE)) {
		RapidEntry re;
		if (GetRapidEntry(dataDirsAccess.LocateFile(file), &re, [&](const RapidEntry& re) { return re.GetTag() == tag; }))
			return re.GetName();
	}
	return tag;
}

std::string GetRapidTagFromPackage(const std::string& pkg)
{
	for (const std::string& file: dataDirsAccess.FindFiles("rapid", "versions.gz", FileQueryFlags::RECURSE)) {
		RapidEntry re;
		if (GetRapidEntry(dataDirsAccess.LocateFile(file), &re, [&](const RapidEntry& re) { return re.GetPackageHash() == pkg; }))
			return re.GetTag();
	}
	return "rapid_tag_unknown";
}

