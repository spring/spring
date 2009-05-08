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
 * @brief Option parameter none
 *
 * Specifies that this option takes no extra
 * arguments.
 */
#define OPTPARM_NONE 0

/**
 * @brief Option parameter int
 *
 * Specifies that this option takes an integer
 * argument.
 */
#define OPTPARM_INT 1

/**
 * @brief Option parameter string
 *
 * Specifies that this option takes a string argument.
 */
#define OPTPARM_STRING 2

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
	void usage(std::string program, std::string version);

	/**
	 * @brief add option
	 * @param shortopt the short (single character) to use
	 * @param longopt the long (full string) to use
	 * @param parmtype the type of parameter (one of the OPTPARM defines)
	 * @param parmname the formal name for this parameter
	 * @param desc a short description of this parameter
	 */
	void addoption(const char shortopt, std::string longopt, unsigned int parmtype, std::string parmname, std::string desc);

	/**
	 * @brief parse
	 *
	 * This will read the parameters and search for recognized strings.
	 * As flags are specified differently on each platform, this method
	 * is abstract and must be implemented.
	 */
	void parse();

	/**
	 * @brief result
	 * @param o single character option to recognize
	 * @return whether this option was specified
	 */
	bool result(const char o);

	/**
	 * @brief result
	 * @param o long string option to recognize
	 * @return whether this option was specified
	 */
	bool result(std::string o);

	/**
	 * @brief result
	 * @param o single character option to recognize
	 * @param ret reference to integer in which to store the argument
	 * @return whether this option was specified
	 */
	bool result(const char o, int &ret);

	/**
	 * @brief result
	 * @param o long string option to recognize
	 * @param ret reference to integer in which to store the argument
	 * @return whether this option was specified
	 */
	bool result(std::string o, int &ret);

	/**
	 * @brief result
	 * @param o short option to recognize
	 * @param ret reference to string in which to store the argument
	 * @return whether this option was specified
	 */
	bool result(const char o, std::string &ret);

	/**
	 * @brief result
	 * @param o long option to recognize
	 * @param ret reference to string in which to store the argument
	 * @return whether this option was specified
	 */
	bool result(std::string o, std::string &ret);

protected:

	/**
	 * @brief option struct
	 * 
	 * Internal struct used to represent an option
	 */
	struct option {

		/**
		 * @brief short option
		 *
		 * The single character flag for this option
		 */
		char shortopt;

		/**
		 * @brief long option
		 *
		 * The full string flag for this option
		 */
		std::string longopt;

		/**
		 * @brief parameter type
		 *
		 * Stores what type of argument this parameter is
		 */
		unsigned int parmtype;

		/**
		 * @brief parameter name
		 *
		 * Stores the formal name of this argument
		 */
		std::string parmname;

		/**
		 * @brief description
		 *
		 * Stores the description of this argument
		 */
		std::string desc;

		/**
		 * @brief given
		 *
		 * Boolean flag representing whether this option was
		 * given or not.
		 */
		bool given;

		/**
		 * @brief return union
		 *
		 * Stores the parameter given with this argument.
		 * It's a union since a parameter is either integer
		 * or string, but not both.
		 */
		union {
			int intret; 		//!< Integer return
			char *stringret; 	//!< C string return
		} ret;
	};

	/**
	 * @brief option vector
	 *
	 * Vector used to store the list of recognized options
	 */
	std::vector<struct option> options;

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

	/**
	 * @brief invalid option
	 * @param opt unrecognized option string
	 * @return exit code to use
	 */
	int invalidoption(std::string opt);

	/**
	 * @brief missing parameter
	 * @param opt unrecognized option string
	 * @return exit code to use
	 */
	int missingparm(std::string opt);

	/**
	 * @brief is s an integer?
	 * @param s string to examine
	 * @return true if s is an integer, false if not.
	 */
	bool is_int(const std::string& s) const;

	boost::program_options::variables_map vm;
	boost::program_options::options_description desc;
};

#endif
