/*
 * filefunctions.h
 * Generic Boost file handling functions
 * Copyright (C) 2005 Christopher Han
 * Glob conversion based on work by Nathaniel Smith
 */
#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include <vector>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>

namespace fs = boost::filesystem;

#define QUOTE(c,str)			\
do {					\
	if (!(isalnum(c)||(c)=='_'))	\
		str+='\\';		\
	str+=c;				\
} while (0)

/**
 * glob_to_regex()
 * Converts a glob expression to a regex
 * @param glob string containing glob
 * @return string containing regex
 */
static inline std::string glob_to_regex(const std::string &glob)
{
	std::string regex;
	regex.reserve(glob.size()<<1);
	int braces = 0;
	for (std::string::const_iterator i = glob.begin(); i != glob.end(); ++i) {
		char c = *i;
#ifdef DEBUG
		if (braces>=5)
			fprintf(stderr,"glob_to_regex warning: braces nested too deeply\n");
#endif
		switch (c) {
			case '*':
				regex+=".*";
				break;
			case '?':
				regex+='.';
				break;
			case '{':
				braces++;
				regex+='(';
				break;
			case '}':
#ifdef DEBUG
				if (braces)
					fprintf(stderr,"glob_to_regex warning: closing brace without an equivalent opening brace\n");
#endif
				regex+=')';
				braces--;
				break;
			case ',':
				if (braces)
					regex+='|';
				else
					QUOTE(c,regex);
				break;
			case '\\':
#ifdef DEBUG
				if (++i==glob.end())
					fprintf(stderr,"glob_to_regex warning: pattern ends with backslash\n");
#else
				++i;
#endif
				QUOTE(*i,regex);
				break;
			default:
				QUOTE(c,regex);
				break;
		}
	}
#ifdef DEBUG
	if (braces)
		fprintf(stderr,"glob_to_regex warning: unterminated brace expression\n");
#endif
	return regex;
}

static inline std::vector<fs::path> find_files(fs::path &dirpath, const std::string &pattern, const bool recurse = false)
{
	std::vector<fs::path> matches;
	if (dirpath.empty())
		dirpath=fs::path("./");
	boost::regex regexpattern(glob_to_regex(pattern));
	if (fs::exists(dirpath) && fs::is_directory(dirpath)) {
		fs::directory_iterator enditr;
		for (fs::directory_iterator diritr(dirpath); diritr != enditr; ++diritr) {
			if (!fs::is_directory(*diritr)) {
				if (boost::regex_match(diritr->leaf(),regexpattern))
					matches.push_back(*diritr);
			} else if (recurse) {
				std::vector<fs::path> submatch = find_files(*diritr,pattern,true);
				if (!submatch.empty()) {
					for (std::vector<fs::path>::iterator it=submatch.begin(); it != submatch.end(); it++)
						matches.push_back(*it);
				}
			}
		}
	} else {
#ifdef DEBUG
		fprintf(stderr,"find_files warning: search path %s is not a directory\n",dirpath.string().c_str());
#endif
	}
	return matches;
}

#endif /* FILEFUNCTIONS_H */
