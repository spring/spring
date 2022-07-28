/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "DataDirsAccess.h"
#include "FileQueryFlags.h"
#include "System/Config/ConfigHandler.h"
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

CONFIG(std::string, RapidTagResolutionOrder)
	.defaultValue("")
	.description("';' separated list of domains, preference order for resolving package from rapid tags");

static std::vector<std::string> ParseRapidTagResolutionOrder() {
	const auto orderStr = configHandler->GetString("RapidTagResolutionOrder");
	std::vector<std::string> order;
	std::size_t beg, end;
	for (beg = 0; (end = orderStr.find(';', beg)) != orderStr.npos; beg = end + 1) {
		order.emplace_back(orderStr.substr(beg, end - beg));
	}
	order.emplace_back(orderStr.substr(beg,orderStr.size() - beg));

	static const std::string springRtsRepo = "repos.springrts.com";
	if (std::find(order.begin(), order.end(), springRtsRepo) == order.end()) {
		order.emplace_back(springRtsRepo);
	}
	return order;
}

std::string GetRapidPackageFromTag(const std::string& tag)
{
	const auto order = ParseRapidTagResolutionOrder();
	std::string package = tag;
	std::size_t rank = std::numeric_limits<std::size_t>::max();
	for (const std::string& file: dataDirsAccess.FindFiles("rapid", "versions.gz", FileQueryFlags::RECURSE)) {
		RapidEntry re;
		if (GetRapidEntry(dataDirsAccess.LocateFile(file), &re, [&](const RapidEntry& re) { return re.GetTag() == tag; })) {
			// The `file` path looks like `rapid/[domain]/[repo]/versions.gz`
			// e.g: `rapid/repos.springrts.com/byar-chobby/versions.gz` as created
			// by pr-downloader. We extract the domain part from it.
			const auto rapidDomain = std::filesystem::path{file}.parent_path().parent_path().filename().string();
			std::size_t new_rank = std::numeric_limits<std::size_t>::max() - 1;
			if (auto it = std::find(order.begin(), order.end(), rapidDomain); it != order.end()) {
				new_rank = it - order.begin();
			}
			if (new_rank == rank)
				LOG_L(L_WARNING, "Rapid tag %s resolves to multiple versions with the same preference, picking from %s",
				      tag.c_str(), file.c_str());
			if (new_rank <= rank) {
				package = re.GetName();
				rank = new_rank;
			}
		}
	}
	return package;
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

