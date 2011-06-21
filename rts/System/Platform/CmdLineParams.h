/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CMD_LINE_PARAMS_H
#define _CMD_LINE_PARAMS_H

#ifdef __APPLE__
#pragma GCC visibility push(default)
#endif

#include <vector>
#include <string>
#include <boost/program_options.hpp>

/**
 * @brief Command-line parser
 * Serves the same purpose as the getopt library.
 */
class CmdLineParams
{

public:
	/**
	 * The base constructor sets up the default help and
	 * version options that should be available on all platforms.
	 */
	CmdLineParams(int argc, char* argv[]);
	~CmdLineParams();

	/// Returns the script or demofile given on the command-line
	/**
	 * @return the script or demofile given on the command-line,
	 *   or "" if none was given.
	 */
	std::string GetInputFile();

	/**
	 * Print out usage information, along with a list of command-line
	 * options that have been set up for this command-line parser.
	 * @param program name of the program
	 * @param version version of this program
	 */
	void PrintUsage(std::string program, std::string version);

	/**
	 * @brief add options
	 * @param shortopt the short (single character) to use (0 for none)
	 * @param longopt the long (full string) to use (required)
	 * @param desc a short, human-readable description of this parameter
	 */
	void AddSwitch(const char shortopt, std::string longopt, std::string desc);
	void AddString(const char shortopt, std::string longopt, std::string desc);
	void AddInt(const char shortopt, std::string longopt, std::string desc);

	/**
	 * @brief Iterates through and processes arguments
	 *
	 * This will read the parameters and search for recognized strings.
	 */
	void Parse();

	/**
	 * @brief check if commandline flag was set
	 * @param var the longopt-name of the config flag
	 */
	bool IsSet(const std::string& var) const;

	/**
	 * @brief Commandline argument as string
	 * @param var the longopt-name of the config flag
	 */
	std::string GetString(const std::string& var) const;

	/**
	 * @brief Commandline argument as int
	 * @param var the longopt-name of the config flag
	 */
	int GetInt(const std::string& var) const;

protected:
	/**
	 * @brief argument count
	 *
	 * Stores the argument count specified at initialization
	 */
	int argc;

	/**
	 * @brief arguments
	 *
	 * Stores the C string array given at initialization
	 */
	char** argv;

	boost::program_options::variables_map vm;
	boost::program_options::options_description desc;
	boost::program_options::options_description all;
};

#endif // _CMD_LINE_PARAMS_H

#ifdef __APPLE__
#pragma GCC visibility pop
#endif
