// used to decide what gets imported and what gets exported

#include "ExternalAI/aibase.h" // we need this to define DLL_EXPORT

// define DLL_IMPORT, which is platform dependent
// since we only have a C interface, we dont care about destructors, virtual functions and so on
// so that simplifies
#ifdef _WIN32
	#define DLL_IMPORT extern "C" __declspec(dllimport)
#elif __GNUC__ >= 4
	// Support for '-fvisibility=hidden'.
	#define DLL_EXPORT extern "C" __attribute__ ((visibility("default")))
#else
	// Older versions of gcc have everything visible; no need for fancy stuff.
	#define DLL_EXPORT extern "C"
#endif

// define BUILDING_ABIC when building abic dll:
#ifdef BUILDING_ABIC
   #define SPRING_TO_ABIC_API DLL_EXPORT  // export, for spring to call into us
   #define AICALLBACK_API DLL_EXPORT    // export, for ai.dll to callback into us
                                                   // no outgoing imports: we link dynamically with ai.dll
#endif

// define BUILDING_AI when building ai dll:
#ifdef BUILDING_AI
   #define AICALLBACK_API DLL_IMPORT     // import, to call into abic.dll
#endif

