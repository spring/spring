/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGREGEX_H
#define SPRINGREGEX_H

#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
#else
#include <regex>
#endif


namespace spring {
#ifdef USE_BOOST_REGEX
	using boost::regex;
	using boost::regex_replace;
	using boost::regex_match;
	using boost::regex_search;
#else
	using std::regex;
	using std::regex_replace;
	using std::regex_match;
	using std::regex_search;
#endif
}

#endif // SPRINGREGEX_H
