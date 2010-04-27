#ifndef KAIK_UTIL_HDR
#define KAIK_UTIL_HDR

class IAICallback;
namespace AIUtil {
	std::string GetAbsFileName(IAICallback*, const std::string&);

	bool IsFSGoodChar(const char);
	/// Converts a string to one that can be used in a file name (eg. "Abc.123 $%^*" -> "Abc.123_____")
	std::string MakeFileSystemCompatible(const std::string&);

	void StringToLowerInPlace(std::string&);
	std::string StringToLower(std::string);
}

#endif
