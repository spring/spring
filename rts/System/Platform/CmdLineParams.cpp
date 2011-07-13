/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "CmdLineParams.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

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

std::string CmdLineParams::GetInputFile()
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

	po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
	po::notify(vm);
}

void CmdLineParams::PrintUsage(std::string program, std::string version)
{
	if (!program.empty()) {
		std::cout << program;
		if (!version.empty())
			std::cout << " " << version;
		std::cout << std::endl;
		std::cout << "This program is licensed under the GNU General Public License" << std::endl;
	}
	std::cout << desc << std::endl;
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
