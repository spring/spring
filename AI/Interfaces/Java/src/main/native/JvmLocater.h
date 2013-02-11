/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _JVM_LOCATER_H
#define _JVM_LOCATER_H

#if !defined bool
#include <stdbool.h>
#endif
#if !defined size_t
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the arch dir name, used eg here:
 * .../jre/lib/${arch}/client/libjvm.so
 *
 * @return sparc, sparcv9, i386, amd64 or ia64 (windows only)
 */
const char* GetArchPath();

/*
 * Given a JRE location and a JVM type, construct what the name the
 * JVM shared library will be.
 *
 * @param jvmType "/", "\\", "client", "server"
 * @param arch see GetArchPath(), use NULL for the default value
 * @return true, if the JVM library was found, false otherwise.
 */
bool GetJVMPath(const char* jrePath, const char* jvmType,
		char* jvmPath, size_t jvmPathSize, const char* arch);

/**
 * Find the path to a JRE install dir, using platform dependent means.
 *
 * @param path       path of the JRE installation
 * @param pathSize   size of the path parameter
 * @param configFile path to a simple text file containing only a path to the
 *                   JRE installation to use, or NULL
 * @param arch see GetArchPath(), use NULL for the default value
 * @return true, if a JRE was found, false otherwise.
 */
bool GetJREPath(char* path, size_t pathSize, const char* configFile,
		const char* arch);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JVM_LOCATER_H
