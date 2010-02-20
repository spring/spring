#ifndef KAIK_UTIL_HDR
#define KAIK_UTIL_HDR

class IAICallback;
namespace AIUtil {
	std::string GetAbsFileName(IAICallback* cb, const std::string& relFileName);

	bool IsFSGoodChar(const char c);
	/// Converts a string to one that can be used in a file name (eg. "Abc.123 $%^*" -> "Abc.123_____")
	std::string MakeFileSystemCompatible(const std::string& str);
}

#endif
