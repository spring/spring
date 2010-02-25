/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "BaseCmd.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>

namespace po = boost::program_options;

/**
 * The base constructor sets up the default help and
 * version options that should be available on all platforms.
 */
BaseCmd::BaseCmd(int _argc, char* _argv[]) : desc("Allowed options")
{
	argc = _argc;
	argv = _argv;

	AddSwitch('h', "help", "This help message");
	AddSwitch('V', "version", "Display program version");
}

/**
 * The destructor currently does nothing
 */
BaseCmd::~BaseCmd()
{
}

std::string BaseCmd::GetInputFile()
{
	if (vm.count("input-file"))
		return vm["input-file"].as<std::string>();
	else
		return "";
}

void BaseCmd::AddSwitch(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), desc.c_str());
}

void BaseCmd::AddString(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), po::value<std::string>(), desc.c_str());
}

void BaseCmd::AddInt(const char shortopt, std::string longopt, std::string desc)
{
	if (shortopt) {
		longopt += std::string(",") + shortopt;
	}
	all.add_options()(longopt.c_str(), po::value<int>(), desc.c_str());
}

/**
 * Iterates through and processes arguments
 */
void BaseCmd::Parse()
{
	desc.add(all);
	all.add_options()("input-file", po::value<std::string>(), "input file");
	po::positional_options_description p;
	p.add("input-file", 1);

	po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
	po::notify(vm);
}

/**
 * Print out usage information, along with a list of commandline
 * options that have been set up for this commandline parser.
 */
void BaseCmd::PrintUsage(std::string program, std::string version)
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

bool BaseCmd::IsSet(const std::string& var) const
{
	if (vm.count(var))
		return true;
	else
		return false;
}

std::string BaseCmd::GetString(const std::string& var) const
{
	if (vm.count(var))
		return vm[var].as<std::string>();
	else
		return "";
}

int BaseCmd::GetInt(const std::string& var) const
{
	if (vm.count(var))
		return vm[var].as<int>();
	else
		return 0;
}
