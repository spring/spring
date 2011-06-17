// GML - OpenGL Multithreading Library
// for Spring http://springrts.com
// Author: Mattias "zerver" Radeskog
// (C) Ware Zerver Tech. http://zerver.net
// Ware Zerver Tech. licenses this library
// to be used, distributed and modified 
// freely for any purpose, as long as 
// this notice remains unchanged

#ifndef GMLCONFIG_H
#define GMLCONFIG_H

#ifdef USE_GML
#	define GML_ENABLE 1 // multithreaded drawing of units and ground
#else
#	define GML_ENABLE 0 // manually enable opengl multithreading here
#endif

#ifdef USE_GML_SIM
#	define GML_ENABLE_SIM (GML_ENABLE && 1) // runs a completely independent thread loop for the Sim
#else
#	define GML_ENABLE_SIM 0  // manually enable sim thread here
#endif

#ifdef USE_GML_DEBUG
#	define GML_CALL_DEBUG 0  // manually force enable call debugging here
#else
#	define GML_CALL_DEBUG (GML_ENABLE && GML_ENABLE_SIM && 1) // checks for calls made from the wrong thread (enabled by default)
#endif

#define GML_ENABLE_DRAW (GML_ENABLE && 0) // draws everything in a separate thread (for testing only, will degrade performance)
#define GML_SERVER_GLCALL 1 // allows the server thread (0) to make direct GL calls
#define GML_INIT_QUEUE_SIZE 10 // initial queue size, will be reallocated, but must be >= 4
#define GML_USE_NO_ERROR 1 // glGetError always returns success (to improve performance)
#define GML_USE_DEFAULT 1// compile/link/buffer status always returns TRUE/COMPLETE (to improve performance)
#define GML_USE_CACHE 1 // certain glGet calls may use data cached during gmlInit (to improve performance)
//#define GML_USE_QUADRIC_SERVER 1 // use server thread to create/delete quadrics
#define GML_AUX_PREALLOC 128*1024 // preallocation size for aux queue to reduce risk for hang if gl calls happen to be made from Sim thread
#define GML_ENABLE_ITEMSERVER_CHECK (GML_ENABLE_SIM && 1) // if calls to itemserver are made from Sim, output errors to log
#define GML_UPDSRV_INTERVAL 10
#define GML_ALTERNATE_SYNCMODE 1 // mutex-protected synced execution, slower but more portable
#define GML_ENABLE_TLS_CHECK 1 // check if Thread Local Storage appears to be working
#define GML_GCC_TLS_FIX 1 // fix buggy TLS in GCC by using the Win32 TIB (faster also!)
#define GML_MSC_TLS_OPT 1 // use the Win32 TIB for TLS in MSVC (possibly faster)
#define GML_64BIT_USE_GS 1 // 64-bit OS will use the GS register for TLS (untested feature)
#define GML_LOCKED_GMLCOUNT_ASSIGNMENT 0 // experimental feature, probably not needed
#define GML_SHARE_LISTS 1 // use glShareLists to allow opengl calls in sim thread
#define GML_DRAW_THREAD_NUM 0 // thread number of draw thread
#define GML_LOAD_THREAD_NUM 1 // thread number of game loading thread
#define GML_SIM_THREAD_NUM 2 // thread number of sim thread
#define GML_DEBUG_MUTEX 0 // debugs the mutex locking order
//#define BOOST_AC_USE_PTHREADS

#endif