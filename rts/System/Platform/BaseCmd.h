/**
 * @file BaseCmd.h
 * @brief Abstract commandline base
 * @author Christopher Han <xiphux@gmail.com>
 *
 * Base structure for commandline parser class definition
 * Copyright (C) 2005.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */
#ifndef BASECMD_H
#define BASECMD_H

#include <vector>
#include <string>
#include <boost/program_options.hpp>

/**
 * @brief Abstract Base Command Class
 *
 * Every platform-specific commandline handler should be
 * derived from this abstract interface, so that they can
 * be used polymorphically.
 */
class BaseCmd
{

public:
	/**
	 * @brief Constructor
	 */
	BaseCmd(int argc, char* argv[]);

	/**
	 * @brief virtual Destructor
	 */
	~BaseCmd();

	/// Get the script, or demofile given on cmdline
	std::string GetInputFile();

	/**
	 * @brief usage
	 * @param program name of the program
	 * @param version version of this program
	 */
	void PrintUsage(std::string program, std::string version);

	/**
	 * @brief add option
	 * @param shortopt the short (single character) to use
	 * @param longopt the long (full string) to use
	 * @param parmtype the type of parameter (one of the OPTPARM defines)
	 * @param parmname the formal name for this parameter
	 * @param desc a short description of this parameter
	 */
	void AddSwitch(const char shortopt, std::string longopt, std::string desc);
	void AddString(const char shortopt, std::string longopt, std::string desc);
	void AddInt(const char shortopt, std::string longopt, std::string desc);

	/**
	 * @brief parse
	 *
	 * This will read the parameters and search for recognized strings.
	 * As flags are specified differently on each platform, this method
	 * is abstract and must be implemented.
	 */
	void Parse();

	bool IsSet(const std::string& var) const;
	std::string GetString(const std::string& var) const;
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
	char **argv;

	boost::program_options::variables_map vm;
	boost::program_options::options_description desc;
	boost::program_options::options_description all;
};

#endif
