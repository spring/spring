/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRINGREGEX_H
#define SPRINGREGEX_H

#include <regex>

#if !defined(_MSC_VER) && !( __cplusplus >= 201103L &&                             \
    (!defined(__GLIBCXX__) || (__cplusplus >= 201402L) || \
        (defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) || \
         defined(_GLIBCXX_REGEX_STATE_LIMIT)           || \
             (defined(_GLIBCXX_RELEASE)                && \
             _GLIBCXX_RELEASE > 4))))
#error The regex implementation of the used stdlibc++ is broken. Please update gcc / clang to compile spring!
#endif


namespace spring {
	using std::regex;
	using std::regex_replace;
	using std::regex_match;
	using std::regex_search;
	using std::regex_error;
}

#endif // SPRINGREGEX_H

