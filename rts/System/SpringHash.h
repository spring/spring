/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_HASH_H_
#define _SPRING_HASH_H_

#include "Sync/HsiehHash.h"
#include <string>

namespace spring {
	template<typename T>
	struct synced_hash {
		std::uint32_t operator()(const T& s) const;
	};


	#define SpringDefaultHash(T)                          \
	template<>                                            \
	struct synced_hash<T>                                 \
	{                                                     \
		std::uint32_t operator()(const T& t) const {      \
			return static_cast<std::uint32_t>(t);         \
		}                                                 \
	};

	SpringDefaultHash(std::int8_t)
	SpringDefaultHash(std::uint8_t)
	SpringDefaultHash(std::int16_t)
	SpringDefaultHash(std::uint16_t)
	SpringDefaultHash(std::int32_t)
	SpringDefaultHash(std::uint32_t)
	#undef SpringDefaultHash

	template<>
	struct synced_hash<std::int64_t> {
	public:
		std::uint32_t operator()(const std::int64_t& i) const
		{
			return static_cast<std::uint32_t>(i) ^ static_cast<std::uint32_t>(i >> 32);
		}
	};

	template<>
	struct synced_hash<std::uint64_t> {
	public:
		std::uint32_t operator()(const std::int64_t& i) const
		{
			return static_cast<std::uint32_t>(i) ^ static_cast<std::uint32_t>(i >> 32);
		}
	};


	template<>
	struct synced_hash<std::string> {
	public:
		std::uint32_t operator()(const std::string& s) const
		{
			return HsiehHash(&s.data()[0], s.size(), 0);
		}
	};
}

#endif //_SPRING_HASH_H_
