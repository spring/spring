/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "JniUtil.h"

#include "CUtils/SimpleLog.h"


// JNI global vars
static jclass g_cls_url = NULL;
static jmethodID g_m_url_ctor = NULL;

static jclass g_cls_urlClassLoader = NULL;
static jmethodID g_m_urlClassLoader_ctor = NULL;
static jmethodID g_m_urlClassLoader_findClass = NULL;


const char* jniUtil_getJniRetValDescription(const jint retVal) {

	switch (retVal) {
		case JNI_OK:        { return "JNI_OK - success"; break; }
		case JNI_ERR:       { return "JNI_ERR - unknown error"; break; }
		case JNI_EDETACHED: { return "JNI_EDETACHED - thread detached from the VM"; break; }
		case JNI_EVERSION:  { return "JNI_EVERSION - JNI version error"; break; }
#ifdef    JNI_ENOMEM
		case JNI_ENOMEM:    { return "JNI_ENOMEM - not enough (contiguous) memory"; break; }
#endif // JNI_ENOMEM
#ifdef    JNI_EEXIST
		case JNI_EEXIST:    { return "JNI_EEXIST - VM already created"; break; }
#endif // JNI_EEXIST
#ifdef    JNI_EINVAL
		case JNI_EINVAL:    { return "JNI_EINVAL - invalid arguments"; break; }
#endif // JNI_EINVAL
		default:            { return "UNKNOWN - unknown/invalid JNI return value"; break; }
	}
}

bool jniUtil_checkException(JNIEnv* env, const char* const errorMsg) {

	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(LOG_LEVEL_ERROR, errorMsg);
		(*env)->ExceptionDescribe(env);
		return true;
	}

	return false;
}

jclass jniUtil_findClass(JNIEnv* env, const char* const className) {

	jclass res = NULL;

	res = (*env)->FindClass(env, className);
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Class not found: \"%s\"", className);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}

jobject jniUtil_makeGlobalRef(JNIEnv* env, jobject localObject, const char* objDesc) {

	jobject res = NULL;

	// Make the local class a global reference,
	// so it will not be garbage collected,
	// even after this method returned,
	// but only if explicitly deleted with DeleteGlobalRef
	res = (*env)->NewGlobalRef(env, localObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed to make %s a global reference.",
				((objDesc == NULL) ? "" : objDesc));
		(*env)->ExceptionDescribe(env);
		res = NULL;
	}

	return res;
}

bool jniUtil_deleteGlobalRef(JNIEnv* env, jobject globalObject,
		const char* objDesc) {

	// delete the AI class-loader global reference,
	// so it will be garbage collected
	(*env)->DeleteGlobalRef(env, globalObject);
	if ((*env)->ExceptionCheck(env)) {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed to delete global reference %s.",
				((objDesc == NULL) ? "" : objDesc));
		(*env)->ExceptionDescribe(env);
		return false;
	}

	return true;
}

jmethodID jniUtil_getMethodID(JNIEnv* env, jclass cls,
		const char* const name, const char* const signature) {

	jmethodID res = NULL;

	res = (*env)->GetMethodID(env, cls, name, signature);
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}

jmethodID jniUtil_getStaticMethodID(JNIEnv* env, jclass cls,
		const char* const name, const char* const signature) {

	jmethodID res = NULL;

	res = (*env)->GetStaticMethodID(env, cls, name, signature);
	const bool hasException = (*env)->ExceptionCheck(env);
	if (res == NULL || hasException) {
		simpleLog_logL(LOG_LEVEL_ERROR, "Method not found: %s(%s)",
				name, signature);
		if (hasException) {
			(*env)->ExceptionDescribe(env);
		}
		res = NULL;
	}

	return res;
}


static bool jniUtil_initURLClass(JNIEnv* env) {

	if (g_m_url_ctor == NULL) {
		// get the URL class
		static const char* const fcCls = "java/net/URL";

		g_cls_url = jniUtil_findClass(env, fcCls);
		if (g_cls_url == NULL) return false;

		g_cls_url = jniUtil_makeGlobalRef(env, g_cls_url, fcCls);
		if (g_cls_url == NULL) return false;

		// get (String)	constructor
		g_m_url_ctor = jniUtil_getMethodID(env, g_cls_url,
				"<init>", "(Ljava/lang/String;)V");
		if (g_m_url_ctor == NULL) return false;
	}

	return true;
}
jobject jniUtil_createURLObject(JNIEnv* env, const char* const url) {

	jobject jurl = NULL;

	bool ok = true;
	if (g_cls_url == NULL) {
		ok = jniUtil_initURLClass(env);
	}

	if (ok) {
		jstring jstrUrl = (*env)->NewStringUTF(env, url);
		if (jniUtil_checkException(env, "Failed creating Java String.")) { jstrUrl = NULL; }
		if (jstrUrl != NULL) {
			jurl = (*env)->NewObject(env, g_cls_url, g_m_url_ctor, jstrUrl);
			if (jniUtil_checkException(env, "Failed creating Java URL.")) { jurl = NULL; }
		}
	} else {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed creating Java URL; URL class not initialized.");
	}

	return jurl;
}
jobjectArray jniUtil_createURLArray(JNIEnv* env, size_t size) {

	jobjectArray jurlArr = NULL;

	bool ok = true;
	if (g_cls_url == NULL) {
		ok = jniUtil_initURLClass(env);
	}

	if (ok) {
		jurlArr = (*env)->NewObjectArray(env, size, g_cls_url, NULL);
		if (jniUtil_checkException(env, "Failed creating URL[].")) { jurlArr = NULL; }
	} else {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed creating Java URL[]; URL class not initialized.");
	}

	return jurlArr;
}
bool jniUtil_insertURLIntoArray(JNIEnv* env, jobjectArray arr, size_t index, jobject url) {

	bool ok = true;

	(*env)->SetObjectArrayElement(env, arr, index, url);
	if (jniUtil_checkException(env, "Failed inserting Java URL into array.")) { ok = false; }

	return ok;
}

static bool jniUtil_initURLClassLoaderClass(JNIEnv* env) {

	if (g_m_urlClassLoader_findClass == NULL) {
		// get the URLClassLoader class
		static const char* const fcCls = "java/net/URLClassLoader";

		g_cls_urlClassLoader = jniUtil_findClass(env, fcCls);
		if (g_cls_urlClassLoader == NULL) return false;

		g_cls_urlClassLoader =
				jniUtil_makeGlobalRef(env, g_cls_urlClassLoader, fcCls);
		if (g_cls_urlClassLoader == NULL) return false;

		// get (URL[])	constructor
		g_m_urlClassLoader_ctor = jniUtil_getMethodID(env,
				g_cls_urlClassLoader, "<init>", "([Ljava/net/URL;)V");
		if (g_m_urlClassLoader_ctor == NULL) return false;

		// get the findClass(String) method
		g_m_urlClassLoader_findClass = jniUtil_getMethodID(env,
				g_cls_urlClassLoader, "findClass",
				"(Ljava/lang/String;)Ljava/lang/Class;");
		if (g_m_urlClassLoader_findClass == NULL) return false;
	}

	return true;
}
jobject jniUtil_createURLClassLoader(JNIEnv* env, jobject urlArray) {

	jobject classLoader = NULL;

	bool ok = true;
	if (g_m_urlClassLoader_ctor == NULL) {
		ok = jniUtil_initURLClassLoaderClass(env);
	}

	if (ok) {
		classLoader = (*env)->NewObject(env, g_cls_urlClassLoader, g_m_urlClassLoader_ctor, urlArray);
		if (jniUtil_checkException(env, "Failed creating class-loader.")) { return NULL; }
	} else {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed creating class-loader; class-loader class not initialized.");
	}

	return classLoader;
}
jclass jniUtil_findClassThroughLoader(JNIEnv* env, jobject classLoader, const char* const className) {

	jclass cls = NULL;

	bool ok = true;
	if (g_m_urlClassLoader_findClass == NULL) {
		ok = jniUtil_initURLClassLoaderClass(env);
	}

	if (ok) {
		//cls = (*env)->FindClass(env, classNameP);
		jstring jstr_className = (*env)->NewStringUTF(env, className);
		cls = (*env)->CallObjectMethod(env, classLoader, g_m_urlClassLoader_findClass, jstr_className);
		const bool hasException = (*env)->ExceptionCheck(env);
		if (cls == NULL || hasException) {
			simpleLog_logL(LOG_LEVEL_ERROR,
					"Class not found \"%s\"", className);
			if (hasException) {
				(*env)->ExceptionDescribe(env);
			}
			cls = NULL;
		}
	} else {
		simpleLog_logL(LOG_LEVEL_ERROR,
				"Failed finding class; class-loader class not initialized.");
	}

	return cls;
}

