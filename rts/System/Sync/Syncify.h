#ifndef SYNCIFY_H
#define SYNCIFY_H

#include <new>

// #define SYNCIFY
#if defined BUILDING_AI || defined BUILDING_AI_INTERFACE
	#ifdef SYNCIFY
		#undef SYNCIFY
	#endif
#endif // defined BUILDING_AI || defined BUILDING_AI_INTERFACE

#ifdef SYNCIFY

struct Syncify_t {};
static Syncify_t syncify;

// Not sure this is needed.
#ifdef _WIN32
	void	*operator new(size_t reportedSize, Syncify_t);
	void	*operator new[](size_t reportedSize, Syncify_t);
	void	operator delete(void *reportedAddress);
	void	operator delete[](void *reportedAddress);
#else
	void	*operator new(size_t reportedSize, Syncify_t) throw(std::bad_alloc);
	void	*operator new[](size_t reportedSize, Syncify_t) throw(std::bad_alloc);
	void	operator delete(void *reportedAddress) throw();
	void	operator delete[](void *reportedAddress) throw();
#endif

	namespace Syncify {
		void InitSyncify();
		void EndSyncify();
		void EnterSyncedMode();
		void EnterUnsyncedMode();
		void EnterMixedMode();
		void PushCodeMode();
		void PopCodeMode();
		void SetCodeModeToMem(void* mem);
		void AssertSyncedMode(char* file,int line);
		void AssertUnsyncedMode(char* file,int line);
		void AssertMixedMode(char* file,int line);
	}

	#define INIT_SYNCIFY Syncify::InitSyncify();
	#define END_SYNCIFY Syncify::EndSyncify();
	#define ENTER_SYNCED	Syncify::EnterSyncedMode();
	#define ENTER_UNSYNCED	Syncify::EnterUnsyncedMode();
	#define ENTER_MIXED	Syncify::EnterMixedMode();
	#define PUSH_CODE_MODE	Syncify::PushCodeMode();
	#define POP_CODE_MODE Syncify::PopCodeMode();
	#define SET_CODE_MODE_TO_MEM(pointer)	Syncify::SetCodeModeToMem(pointer);
	#define ASSERT_SYNCED_MODE Syncify::AssertSyncedMode(__FILE__,__LINE__);
	#define ASSERT_UNSYNCED_MODE Syncify::AssertUnsyncedMode(__FILE__,__LINE__);
	#define ASSERT_MIXED_MODE Syncify::AssertMixedMode(__FILE__,__LINE__);
	#define SAFE_NEW new(syncify)

#else // SYNCIFY

	#define INIT_SYNCIFY
	#define END_SYNCIFY
	#define ENTER_SYNCED
	#define ENTER_UNSYNCED
	#define ENTER_MIXED
	#define POP_CODE_MODE
	#define PUSH_CODE_MODE
	#define SET_CODE_MODE_TO_MEM(pointer)
	#define ASSERT_SYNCED_MODE
	#define ASSERT_UNSYNCED_MODE
	#define ASSERT_MIXED_MODE
	#define SAFE_NEW new
#endif // SYNCIFY

#endif // SYNCIFY_H
