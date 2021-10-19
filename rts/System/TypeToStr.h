#pragma once

#include <string>
#include <string_view>
#include <array>   // std::array
#include <utility> // std::index_sequence

// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

namespace spring {
	namespace impl {
		template <std::size_t...Idxs>
		constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
		{
			return std::array{ str[Idxs]... };
		}

		template <typename T>
		constexpr auto type_name_array()
		{
		#if defined(__clang__)
			constexpr auto prefix = std::string_view{ "[T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
		#elif defined(__GNUC__)
			constexpr auto prefix = std::string_view{ "with T = " };
			constexpr auto suffix = std::string_view{ "]" };
			constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
		#elif defined(_MSC_VER)
			constexpr auto space = std::string_view{ " " };
			constexpr auto prefix = std::string_view{ "type_name_array<" };
			constexpr auto suffix = std::string_view{ ">(void)" };
			constexpr auto function = std::string_view{ __FUNCSIG__ };
		#else
			#error Unsupported compiler
		#endif

			constexpr auto start = function.find(prefix) + prefix.size();
			constexpr auto end = function.rfind(suffix);

			static_assert(start < end);

			constexpr auto name = function.substr(start, (end - start));
		#if defined(_MSC_VER)
			constexpr auto begSp = name.find(space) + space.size();

			static_assert(begSp < name.size());

			constexpr auto cleanName = name.substr(begSp); //remove "class "/"struct ", etc
			return substring_as_array(cleanName, std::make_index_sequence<cleanName.size()>{});
		#else
			return substring_as_array(name, std::make_index_sequence<name.size()>{});
		#endif
		}

		template <typename T>
		struct type_name_holder {
			static inline constexpr auto value = type_name_array<T>();
		};
	}

	template <typename T>
	constexpr auto TypeToStr() -> std::string_view
	{
		constexpr auto& value = impl::type_name_holder<T>::value;
		return std::string_view{ value.data(), value.size() };
	}
}
