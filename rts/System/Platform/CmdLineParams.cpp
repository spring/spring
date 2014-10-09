/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CmdLineParams.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

#include "Game/GameVersion.h"
#include "System/Util.h"

namespace po = boost::program_options;

CmdLineParams::CmdLineParams(int _argc, char* _argv[]) : desc("Allowed options")
{
	argc = _argc;
	argv = _argv;

	AddSwitch('h', "help", "This help message");
	AddSwitch('V', "version", "Display program version");
}

CmdLineParams::~CmdLineParams()
{
}

std::string CmdLineParams::GetInputFile() const
{
	if (vm.count("input-file"))
		return vm["input-file"].as<std::string>();
	else
		return "";
}

void CmdLineParams::AddSwitch(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), desc.c_str());
}

void CmdLineParams::AddString(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), po::value<std::string>(), desc.c_str());
}

void CmdLineParams::AddInt(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), po::value<int>(), desc.c_str());
}

void CmdLineParams::Parse()
{
	desc.add(all);
	all.add_options()("input-file", po::value<std::string>(), "input file");
	po::positional_options_description p;
	p.add("input-file", 1);

	try {
		po::parsed_options parsed = po::command_line_parser(argc, argv).options(all).positional(p).allow_unregistered().run();
		po::store(parsed, vm);
		po::notify(vm);

		std::vector<std::string> unrecognized = po::collect_unrecognized(parsed.options, po::exclude_positional);
		if (!unrecognized.empty())
			throw unrecognized_option("unrecognized option '" + unrecognized[0] + "'");
	} catch(const boost::program_options::too_many_positional_options_error& err) {
		throw unrecognized_option("too many unnamed cmdline input options (forgot to quote filepath?)");
	}
}

void CmdLineParams::PrintUsage() const
{
	if (!usage_desc.empty()) {
		std::cout << usage_desc << std::endl;
	}

	std::cout << desc << std::endl;

	std::cout << "Spring" << " " << SpringVersion::GetFull() << std::endl;
	std::cout << "This program is licensed under the GNU General Public License" << std::endl;
}

std::string CmdLineParams::GetCmdLine() const
{
	std::string cmdLine = (argc > 0) ? UnQuote(argv[0]) : "unknown";
	for (int i = 1; i < argc; ++i) {
		cmdLine += " ";
		if (argv[i][0] != '-') {
			cmdLine += "\"";
			cmdLine += argv[i];
			cmdLine += "\"";
		} else {
			cmdLine += argv[i];
		}
	}
	return cmdLine;
}

bool CmdLineParams::IsSet(const std::string& var) const
{
	if (vm.count(var))
		return true;
	else
		return false;
}

std::string CmdLineParams::GetString(const std::string& var) const
{
	if (vm.count(var))
		return vm[var].as<std::string>();
	else
		return "";
}

int CmdLineParams::GetInt(const std::string& var) const
{
	if (vm.count(var))
		return vm[var].as<int>();
	else
		return 0;
}
