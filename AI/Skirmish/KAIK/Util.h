#ifndef KAIK_UTIL_HDR
#define KAIK_UTIL_HDR

class IAICallback;
namespace AIUtil {
	std::string GetAbsFileName(IAICallback* cb, const std::string& relFileName);
}

#endif
