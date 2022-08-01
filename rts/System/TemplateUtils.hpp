// https://stackoverflow.com/questions/38067106/c-verify-callable-signature-of-template-type
namespace spring {

	template<typename, typename, typename = void>
	struct is_signature : std::false_type {};

	// use static_assert(is_signature<SomeTemplateF, int(int)>::value);
	template<typename TFunc, typename Ret, typename... Args>
	struct is_signature<TFunc, Ret(Args...),
			typename std::enable_if_t<
				std::is_convertible<TFunc, std::function<Ret(Args...)>>::value
			>
		> : public std::true_type
	{};

	// use static_assert(is_signature<std::function<int(int)>, decltype(someFunction)>::value);
	template<typename TFunc1, typename TFunc2>
	struct is_signature<TFunc1, TFunc2,
			typename std::enable_if_t<
				std::is_convertible<TFunc1, TFunc2>::value
			>
		> : public std::true_type
	{};
	
	template<typename... Args>
	inline constexpr bool is_signature_v = is_signature<Args...>::value;

	// cpp20 compat
	template< class T >
	struct remove_cvref {
		typedef std::remove_cv_t<std::remove_reference_t<T>> type;
	};
};