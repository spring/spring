/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UNSIGNED_STRING_H
#define _UNSIGNED_STRING_H

#include <string>

typedef unsigned char char8_t;

namespace std {
	class u8string : public string {
	public:
		// copy ctors
		//using string::string; // gcc4.8 and newer
		explicit u8string(const std::string& s) : string(s) {}
		u8string(const char* c) : string(c) {}

		//! this is an important difference, we return an unsigned value here!
		//! std::string returns a _signed_ one and breaks this way:
		//!     std::string("\0xFF")[0] == 0xFF // FAILS!!!
		//!   std::u8string("\0xFF")[0] == 0xFF // WORKS!!!
		//! PS: this is very important with our Color...Indicator cause those
		//!     are defined as unsigned to be compatible with char32_t!
		const char8_t& operator[] (const int t) const {
			auto& str = *static_cast<const std::string*>(this);
			return *reinterpret_cast<const char8_t*>(&str[t]);
		}
	};

	// alternative solution
	// We would need to overload all ctors & funcs with "char*" argument then.
	// too complicated -> the above is less code
	//using u8string = basic_string<char8_t>;
}

static inline const std::u8string& toustring(const std::string& str)
{
	return *static_cast<const std::u8string*>(&str);
}

static inline std::u8string& toustring(std::string& str)
{
	return *static_cast<std::u8string*>(&str);
}

#endif
