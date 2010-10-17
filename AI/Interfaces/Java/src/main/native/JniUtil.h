/*
	Copyright (c) 2009 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _JNI_UTIL_H
#define _JNI_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool, true, false

#include <jni.h>

/**
 * Takes a JNI function return value, and returns a short description for it.
 * This can be used for getting a human readable string describing the
 * return value of functions like AttachCurrentThread() and CreateJavaVM().
 *
 * @return  a short description for a JNI function return value
 */
const char* jniUtil_getJniRetValDescription(const jint retVal);

/**
 * Handles a possible JNI/Java exception.
 * In case an exception is present, the supplied error message is written to
 * the log file, and the whole exception info is written to stderr.
 *
 * @return  true, if there was an exception
 */
bool jniUtil_checkException(JNIEnv* env, const char* const errorMsg);

/**
 * Tries to find a Java class.
 *
 * @return  a local reference to the class if found, NULL otherwise
 */
jclass jniUtil_findClass(JNIEnv* env, const char* const className);

/**
 * Converts a local to a global reference.
 * As jclass inherits from jobject, this can be used for jclass too.
 *
 * @return  the global reference to localObject on success, NULL otherwise
 */
jobject jniUtil_makeGlobalRef(JNIEnv* env, jobject localObject, const char* objDesc);

/**
 * Deletes a global reference to an object.
 * As jclass inherits from jobject, this can be used for jclass too.
 *
 * @return  true on success, false if an error ocurred
 */
bool jniUtil_deleteGlobalRef(JNIEnv* env, jobject globalObject, const char* objDesc);

/**
 * Retrieves a reference ID for calling a Java object method.
 *
 * @return  the method ID on success, NULL otherwise
 */
jmethodID jniUtil_getMethodID(JNIEnv* env, jclass cls, const char* const name, const char* const signature);

/**
 * Retrieves a reference ID for calling a Java static method.
 *
 * @return  the method ID on success, NULL otherwise
 */
jmethodID jniUtil_getStaticMethodID(JNIEnv* env, jclass cls, const char* const name, const char* const signature);

jobject jniUtil_createURLObject(JNIEnv* env, const char* const url);
jobjectArray jniUtil_createURLArray(JNIEnv* env, size_t size);
bool jniUtil_insertURLIntoArray(JNIEnv* env, jobjectArray arr, size_t index, jobject url);

jobject jniUtil_createURLClassLoader(JNIEnv* env, jobject urlArray);
jclass jniUtil_findClassThroughLoader(JNIEnv* env, jobject classLoader, const char* const className);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _JNI_UTIL_H
