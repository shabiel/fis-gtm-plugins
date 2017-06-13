/****************************************************************
*								*
*	Copyright 2013 Fidelity Information Services, Inc	*
*								*
*	This source code contains the intellectual property	*
*	of its copyright holder(s), and is made available	*
*	under a license.  If you do not know the terms of	*
*	the license, please stop and do not read further.	*
*								*
****************************************************************/
#include "gtm_sizeof.h"
#include "gtm_common_defs.h"
#include "gtm_limits.h"
#include "ci_gateway.h"
#include "gtmxc_types.h"
#include "gtmji.h"

#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#ifdef DEBUG
#  include <assert.h>
#endif

/* Obtain a reference to the specified class and cache it for further call-in invocations. */
#define FIND_CLASS_REFERENCE(ENV, CLASS, TEMP_VAR, VAR)							\
{													\
	TEMP_VAR = (*ENV)->FindClass(ENV, CLASS);							\
	if (NULL == TEMP_VAR)										\
	{												\
		THROW_EXCEPTION_AND_RETURN(ENV, JNI_ERR, "Cannot find %s class definition.", CLASS);	\
	}												\
	VAR = (*ENV)->NewGlobalRef(ENV, TEMP_VAR);							\
}

/* Find the ID of the specified field. */
#define FIND_FIELD_ID(ENV, CLS, NAME, TYPE, VAR)							\
{													\
	VAR = (*ENV)->GetFieldID(ENV, CLS, NAME, TYPE);							\
	if (NULL == VAR)										\
	{												\
		THROW_EXCEPTION_AND_RETURN(ENV, JNI_ERR, "Cannot find field id for %s.", NAME);		\
	}												\
}

/* Find the ID of the specified method. */
#define FIND_METHOD_ID(ENV, CLS, NAME, TYPE, VAR)							\
{													\
	VAR = (*ENV)->GetMethodID(ENV, CLS, NAME, TYPE);						\
	if (NULL == VAR)										\
	{												\
		THROW_EXCEPTION_AND_RETURN(ENV, JNI_ERR, "Cannot find method id for %s.", NAME);	\
	}												\
}

/* Pull each argument individually from the argument list, cache the reference in a native array,
 * obtain the argument's type for further type-based processing, and also cache it.
 */
#define GET_ARG(ENV, FUNC_NAME, ARGS_ARRAY, ARG, TYPE, NUM, OFFSET, JSTR_CNT, JARR_CNT)			\
{													\
	ARG = (*ENV)->GetObjectArrayElement(ENV, ARGS_ARRAY, NUM);					\
	if (NULL == ARG)										\
	{												\
		CLEAN_UP_AND_RETURN(ENV, JSTR_CNT, JARR_CNT, JNI_ERR, 3,				\
			"Arg #%d to entryref '%s' is null.", (NUM + 1), FUNC_NAME);			\
	}												\
	arg_conts[NUM + OFFSET] = ARG;									\
	if ((*ENV)->IsInstanceOf(ENV, ARG, class_refs[JAVA_STRING]))					\
		TYPE = JAVA_STRING;									\
	else if ((*ENV)->IsInstanceOf(ENV, ARG, class_refs[JAVA_BYTE_ARRAY]))				\
	      TYPE = JAVA_BYTE_ARRAY;									\
	else if ((*ENV)->IsInstanceOf(ENV, ARG, class_refs[JAVA_BIG_DECIMAL]))				\
	      TYPE = JAVA_BIG_DECIMAL;									\
	else if ((*ENV)->IsInstanceOf(ENV, ARG, gtm_container_class))					\
	      TYPE = (*ENV)->GetIntField(ENV, ARG, gtm_container_type_fid);				\
	else												\
	{												\
		CLEAN_UP_AND_RETURN(ENV, JSTR_CNT, JARR_CNT, JNI_ERR, 3,				\
			"Arg #%d to entryref '%s' is of unsupported type.", (NUM + 1), FUNC_NAME);	\
	}												\
	arg_types[NUM + OFFSET] = TYPE;									\
}

/* Release JVM-allocated resources and return with the specified code. */
#define CLEAN_UP_AND_RETURN(ENV, JSTR_CNT, JARR_CNT, RET_CODE, VAR_ARG_CNT, ...)			\
{													\
	int		i, add_prefix = 1, var_arg_cnt = VAR_ARG_CNT, prefix_len = 0;			\
	char		err_buffer[BUF_LEN], *err_buffer_ptr;						\
													\
	/* Release all UTF strings allocated by JVM. */							\
	for (i = 0; i < JSTR_CNT; i++)									\
		(*ENV)->ReleaseStringUTFChars(ENV, jstr_ptrs[i], str_ptrs[i]);				\
	/* Release all byte arrays allocated by JVM. Note the JNI_ABORT flag; it ensures that the	\
	 * content of the primitive array referenced by arr_ptrs[i] will NOT be copied back into the	\
	 * Java array referenced by jarr_ptrs[i].							\
	 */												\
	for (i = 0; i < JARR_CNT; i++)									\
		(*ENV)->ReleaseByteArrayElements(ENV, jarr_ptrs[i], arr_ptrs[i], JNI_ABORT);		\
	/* If there is any unhandled exception at this point, return right away (even if we have an	\
	 * error condition of our own that we want to report), letting Java handle it.			\
	 */												\
	if ((*ENV)->ExceptionCheck(ENV))								\
		return RET_CODE;									\
	/* Negative argument count instructs that the message not be prefixed with MSG_LBL. */		\
	if (0 > var_arg_cnt)										\
	{												\
		var_arg_cnt = -var_arg_cnt;								\
		add_prefix = 0;										\
	}												\
	/* In case we are returning with an error, throw a Java exception. */				\
	if (RET_CODE != JNI_OK)										\
	{												\
		err_buffer_ptr = err_buffer;								\
		if (add_prefix)										\
		{											\
			prefix_len = SIZEOF(MSG_LBL);							\
			snprintf(err_buffer_ptr, prefix_len, MSG_LBL);					\
			err_buffer_ptr += (prefix_len - 1);						\
		}											\
		if (0 < var_arg_cnt)									\
			snprintf(err_buffer_ptr, BUF_LEN - prefix_len, __VA_ARGS__);			\
		else											\
			snprintf(err_buffer_ptr, BUF_LEN - prefix_len, "Unknown error");		\
		(*ENV)->ThrowNew(ENV, java_exception_class, err_buffer);				\
	}												\
	return RET_CODE;										\
}

/* Throws a generic Java exception with the specified message. */
#define THROW_EXCEPTION_AND_RETURN(ENV, RET_CODE, ...)							\
{													\
	char err_buffer[BUF_LEN];									\
													\
	if ((*ENV)->ExceptionCheck(ENV))								\
		return RET_CODE;									\
	snprintf(err_buffer, BUF_LEN, __VA_ARGS__);							\
	(*ENV)->ThrowNew(ENV, java_exception_class, err_buffer);					\
	return RET_CODE;										\
}

/* This macro takes care of concurrency control, return object initialization, and error control. */
#define DO_WORK(ENV, TYPE, NAME, ARGS, NEEDS_RETURN, DEF_RET_VALUE)					\
{													\
	jint status, size;										\
													\
	/* In case a call-in is invoked from a call-out, we can only allow the main thread to execute	\
	 * GT.M code to avoid concurrency. The following check enforces that by making use of		\
	 * gtm_is_main_thread() routine, exposed by libgtmshr.so. Note that there is no need to pass	\
	 * any arguments as the function can easily find out our thread ID.				\
	 */												\
	if (0 == gtm_is_main_thread_fptr())								\
	{												\
		THROW_EXCEPTION_AND_RETURN(ENV, DEF_RET_VALUE,						\
			"%sCall-in from within a call-out is only allowed from the main thread.",	\
			MSG_LBL);									\
	}												\
	assert(1 == ++num_of_threads);									\
	arg_types[0] = TYPE;										\
	size = (*ENV)->GetArrayLength(ENV, ARGS);							\
	status = do_work(ENV, NAME, ARGS, size, NEEDS_RETURN);						\
	assert(0 == --num_of_threads);									\
	if (JNI_OK != status)										\
		return DEF_RET_VALUE;									\
}

#define DLSYM_OR_EXIT(ENV, HANDLE, FPTR_TYPE, FPTR, FUNC_NAME)						\
{													\
	char *dlerr_ptr;										\
													\
	FPTR = (FPTR_TYPE)dlsym(HANDLE, FUNC_NAME);							\
	if (NULL == FPTR)										\
	{												\
		if (NULL == (dlerr_ptr = dlerror()))							\
		{											\
        		THROW_EXCEPTION_AND_RETURN(ENV, JNI_ERR, 					\
				"Unable to resolve symbol %s. Unknown system error.", FUNC_NAME);	\
		} else											\
		{											\
        		THROW_EXCEPTION_AND_RETURN(ENV, JNI_ERR, 					\
				"Unable to resolve symbol %s. %s", FUNC_NAME, dlerr_ptr);		\
		}											\
	}												\
}

/* Define the name of the main GT.M library; on all supported platforms it should have the .so extension. */
#define GTMSHR_IMAGENAME		"libgtmshr.so"

/* Define names for all functions that libgtmj2m.so imports from libgtmshr.so at runtime. */
#define GTM_JINIT_FUNC			"gtm_jinit"
#define GTM_EXIT_FUNC			"gtm_exit"
#define GTM_CIJ_FUNC			"gtm_cij"
#define GTM_ZSTATUS_FUNC		"gtm_zstatus"
#define GTM_IS_MAIN_THREAD_FUNC		"gtm_is_main_thread"

/* Define special function pointer types for all imported functions. */
typedef gtm_status_t			(*gtm_jinit_fptr_t)(void);
typedef gtm_status_t			(*gtm_exit_fptr_t)(void);
typedef gtm_status_t			(*gtm_cij_fptr_t)(const char *, char **, int, int *, unsigned int *, unsigned int *);
typedef void				(*gtm_zstatus_fptr_t)(char *, int);
typedef int				(*gtm_is_main_thread_fptr_t)(void);

/* Define the actual function pointers for all imported functions. */
STATICDEF gtm_jinit_fptr_t		gtm_jinit_fptr;
STATICDEF gtm_exit_fptr_t		gtm_exit_fptr;
STATICDEF gtm_cij_fptr_t		gtm_cij_fptr;
STATICDEF gtm_zstatus_fptr_t		gtm_zstatus_fptr;
STATICDEF gtm_is_main_thread_fptr_t	gtm_is_main_thread_fptr;

/* References to GTMContainer and Exception classes to avoid frequent look-ups. */
STATICDEF jclass		gtm_container_class, java_exception_class;

/* Reference to the type field of GTMContainer class, used for identifying each argument's type. */
STATICDEF jfieldID		gtm_container_type_fid;

/* Reference to toString() method of BigDecimal class, used for conversion to a string object. */
STATICDEF jmethodID		java_big_decimal_tostring;

/* Class, constructor, and attribute references for faster look-ups. */
STATICDEF jclass		class_refs[JTYPES_SIZE];
STATICDEF jfieldID		value_field_ids[GTM_JTYPES_SIZE];
STATICDEF jmethodID		const_method_ids[GTM_JTYPES_SIZE];

/* NOTE: The following arrays have a capacity of MAX_PARAMS + 1, extra slot serving being for the
 * return value in case one is expected.
 */

/* Arrays of per-argument references to Java strings/JVM-allocated native strings and byte arrays/
 * JVM-allocated native-type arrays that we later need to release to prevent memory leaks.
 */
STATICDEF jstring		jstr_ptrs[MAX_PARAMS + 1];
STATICDEF const gtm_char_t	*str_ptrs[MAX_PARAMS + 1];
STATICDEF jbyteArray		jarr_ptrs[MAX_PARAMS + 1];
STATICDEF jbyte			*arr_ptrs[MAX_PARAMS + 1];

/* Arrays that store arguments themselves, their type, and---when dealing with strings---character-based
 * representation, all to facilitate the argument passing and conversion process.
 */
STATICDEF jobject		arg_conts[MAX_PARAMS + 1];	/* Argument containers. */
STATICDEF int			arg_types[MAX_PARAMS + 1];
STATICDEF gtm_string_t		gtm_strs[MAX_PARAMS + 1];

/* Arrays to maintain per-argument reference to the byte-array-related structures and their lengths. */
STATICDEF int			byte_array_lens[MAX_PARAMS + 1];
STATICDEF int			byte_array_indices[MAX_PARAMS + 1];

/* Buffer that we fill with argument values for GT.M to read and update. */
STATICDEF char			*arg_list[GTM64_ONLY(1) NON_GTM64_ONLY(2) * (MAX_PARAMS + 1)];

#ifdef	DEBUG
/* Variable used to ensure we are not dealing with concurrent threads inside GT.M. */
STATICDEF int			num_of_threads = 0;
#endif

/* Names of all error types to be used in error messages. */
STATICDEF const char *ret_type_names[] =
{
	"boolean",
	"int",
	"long",
	"float",
	"double",
	"GTMString",
	"GTMByteArray",
	"String",
	"byte[]"
};

jint do_work(JNIEnv *env, jstring name, jobjectArray args, int size, jboolean needs_return);

/* Do the call-in initialization when Java loads our library. */
JNIEXPORT jint JNICALL JNI_OnLoad
(JavaVM *jvm, void *reserved)
{
	JNIEnv		*env;
	jclass		temp_class;
	void		*handle = NULL;
	const char	*gtm_dist, *dlerr_ptr;
	char		gtmshr_file[GTM_PATH_MAX];
	int		gtmshr_imagename_len, dir_len, reserved_path_len;

	/* Check if the JNI version is supported. */
	if ((*jvm)->GetEnv(jvm, (void **)&env, CUR_JNI_VERSION))
		return JNI_ERR;

	/* We cannot use FIND_CLASS_REFERENCE() macro for the Exception class, because in case of an error
	 * the macro would try to raise an exception whose class is not defined yet.
	 */
	temp_class = (*env)->FindClass(env, "java/lang/Exception");
	if (NULL == temp_class)
	{
		assert(JNI_FALSE);
		MSG("Cannot find %s class definition.", "java/lang/Exception");
		return JNI_ERR;
	}
	java_exception_class = (*env)->NewGlobalRef(env, temp_class);

	/* First see if the call-in environment is even set up. */
	if (!getenv("GTMCI"))
	{
        	THROW_EXCEPTION_AND_RETURN(env, JNI_ERR, "The GTMCI environment variable not set.");
	}

	/* Verify early if we will be able to resolve symbols from libgtmshr.so. */
	if (!(gtm_dist = getenv("gtm_dist")))
	{
        	THROW_EXCEPTION_AND_RETURN(env, JNI_ERR, "The gtm_dist environment variable not set.");
	}

	/* The following references describe the primitive and reference types in JNI as well as their signatures
	 * and mnemonics usage in field- and method-specific searches:
	 *   http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html
	 *   http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/functions.html
	 */

	/* Cache class references for possible types. */
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMContainer", temp_class, gtm_container_class);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMBoolean", temp_class, class_refs[GTM_BOOLEAN]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMInteger", temp_class, class_refs[GTM_INTEGER]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMLong", temp_class, class_refs[GTM_LONG]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMFloat", temp_class, class_refs[GTM_FLOAT]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMDouble", temp_class, class_refs[GTM_DOUBLE]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMString", temp_class, class_refs[GTM_STRING]);
	FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMByteArray", temp_class, class_refs[GTM_BYTE_ARRAY]);
	FIND_CLASS_REFERENCE(env, "java/lang/String", temp_class, class_refs[JAVA_STRING]);
	FIND_CLASS_REFERENCE(env, "[B", temp_class, class_refs[JAVA_BYTE_ARRAY]);
	FIND_CLASS_REFERENCE(env, "java/math/BigDecimal", temp_class, class_refs[JAVA_BIG_DECIMAL]);

	/* Cache field ids for value lookups. */
	FIND_FIELD_ID(env, gtm_container_class, "type", "I", gtm_container_type_fid);
	FIND_FIELD_ID(env, class_refs[GTM_BOOLEAN], "value", "Z", value_field_ids[GTM_BOOLEAN]);
	FIND_FIELD_ID(env, class_refs[GTM_INTEGER], "value", "I", value_field_ids[GTM_INTEGER]);
	FIND_FIELD_ID(env, class_refs[GTM_LONG], "value", "J", value_field_ids[GTM_LONG]);
	FIND_FIELD_ID(env, class_refs[GTM_FLOAT], "value", "F", value_field_ids[GTM_FLOAT]);
	FIND_FIELD_ID(env, class_refs[GTM_DOUBLE], "value", "D", value_field_ids[GTM_DOUBLE]);
	FIND_FIELD_ID(env, class_refs[GTM_STRING], "value", "Ljava/lang/String;", value_field_ids[GTM_STRING]);
	FIND_FIELD_ID(env, class_refs[GTM_BYTE_ARRAY], "value", "[B", value_field_ids[GTM_BYTE_ARRAY]);

	/* Cache method ids for constructor usage. */
	FIND_METHOD_ID(env, class_refs[GTM_BOOLEAN], "<init>", "(Z)V", const_method_ids[GTM_BOOLEAN]);
	FIND_METHOD_ID(env, class_refs[GTM_INTEGER], "<init>", "(I)V", const_method_ids[GTM_INTEGER]);
	FIND_METHOD_ID(env, class_refs[GTM_LONG], "<init>", "(J)V", const_method_ids[GTM_LONG]);
	FIND_METHOD_ID(env, class_refs[GTM_FLOAT], "<init>", "(F)V", const_method_ids[GTM_FLOAT]);
	FIND_METHOD_ID(env, class_refs[GTM_DOUBLE], "<init>", "(D)V", const_method_ids[GTM_DOUBLE]);
	FIND_METHOD_ID(env, class_refs[GTM_STRING], "<init>", "(Ljava/lang/String;)V", const_method_ids[GTM_STRING]);
	FIND_METHOD_ID(env, class_refs[GTM_BYTE_ARRAY], "<init>", "([B)V", const_method_ids[GTM_BYTE_ARRAY]);
	FIND_METHOD_ID(env, class_refs[JAVA_BIG_DECIMAL], "toString", "()Ljava/lang/String;", java_big_decimal_tostring);

	/* Calculate space requirements for DIR_SEPARATOR, GTMSHR_IMAGENAME, and '\0'. */
	gtmshr_imagename_len = STR_LIT_LEN(GTMSHR_IMAGENAME);
	reserved_path_len = 1 + gtmshr_imagename_len + 1;
	dir_len = STRLEN(gtm_dist);

	/* Make sure the buffer is big enough for the path. */
	if (reserved_path_len + dir_len > GTM_PATH_MAX)
	{
        	THROW_EXCEPTION_AND_RETURN(env, JNI_ERR, "Path specified in the gtm_dist environment variable is too long.");
	} else
	{	/* Obtain the path to libgtmshr.so and dlopen it. */
		memcpy(&gtmshr_file[0], gtm_dist, dir_len);
		gtmshr_file[dir_len] = DIR_SEPARATOR;
		MEMCPY_LIT(&gtmshr_file[dir_len + 1], GTMSHR_IMAGENAME);
		gtmshr_file[dir_len + 1 + gtmshr_imagename_len] = '\0';
		handle = dlopen(gtmshr_file, RTLD_GLOBAL | RTLD_NOW);
	}
	/* Verify that we did link with the library. */
	if (NULL == handle)
	{
		if (NULL == (dlerr_ptr = dlerror()))
		{
        		THROW_EXCEPTION_AND_RETURN(env, JNI_ERR, "Unable to resolve GT.M interface functions. Unknown system error.");
		} else
		{
        		THROW_EXCEPTION_AND_RETURN(env, JNI_ERR, "Unable to resolve GT.M interface functions. %s", dlerr_ptr);
		}
	}

	/* Finally, resolve all required symbols. */
	DLSYM_OR_EXIT(env, handle, gtm_jinit_fptr_t, gtm_jinit_fptr, GTM_JINIT_FUNC);
	DLSYM_OR_EXIT(env, handle, gtm_exit_fptr_t, gtm_exit_fptr, GTM_EXIT_FUNC);
	DLSYM_OR_EXIT(env, handle, gtm_cij_fptr_t, gtm_cij_fptr, GTM_CIJ_FUNC);
	DLSYM_OR_EXIT(env, handle, gtm_zstatus_fptr_t, gtm_zstatus_fptr, GTM_ZSTATUS_FUNC);
	DLSYM_OR_EXIT(env, handle, gtm_is_main_thread_fptr_t, gtm_is_main_thread_fptr, GTM_IS_MAIN_THREAD_FUNC);

	return CUR_JNI_VERSION;
}

/* Invoke a void method. */
JNIEXPORT void JNICALL Java_com_fis_gtm_ji_GTMCI_doVoidCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	/* Assign a type number that we know is positive---to prevent it accidentally being a negative number
	 * indicating an error---but not signifying any specific return types since this is a void case.
	 * JTYPES_SIZE works well for that purpose.
	 */
	DO_WORK(env, JTYPES_SIZE, name, args, JNI_FALSE, /* VOID */);
	return;
}

/* Invoke a boolean method. Construct a boolean value and return it. */
JNIEXPORT jboolean JNICALL Java_com_fis_gtm_ji_GTMCI_doBooleanCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, GTM_BOOLEAN, name, args, JNI_TRUE, JNI_FALSE);
	return *(int *)(&arg_list[0]) ? JNI_TRUE : JNI_FALSE;
}

/* Invoke an int method. Construct an int value and return it. */
JNIEXPORT jint JNICALL Java_com_fis_gtm_ji_GTMCI_doIntCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, GTM_INTEGER, name, args, JNI_TRUE, 0);
	return *(int *)(&arg_list[0]);
}

/* Invoke a long method. Construct a long value and return it. */
JNIEXPORT jlong JNICALL Java_com_fis_gtm_ji_GTMCI_doLongCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, GTM_LONG, name, args, JNI_TRUE, 0L);
	return *(jlong *)(&arg_list[0]);
}

/* Invoke a float method. Construct a float value and return it. */
JNIEXPORT jfloat JNICALL Java_com_fis_gtm_ji_GTMCI_doFloatCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, GTM_FLOAT, name, args, JNI_TRUE, 0);
	return *(float *)(&arg_list[0]);
}

/* Invoke a double method. Initialize a double value and return it. */
JNIEXPORT jdouble JNICALL Java_com_fis_gtm_ji_GTMCI_doDoubleCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, GTM_DOUBLE, name, args, JNI_TRUE, 0);
	return *(double *)(&arg_list[0]);
}

/* Invoke a string method. Construct a String object and return it. */
JNIEXPORT jstring JNICALL Java_com_fis_gtm_ji_GTMCI_doStringCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, JAVA_STRING, name, args, JNI_TRUE, NULL);
	return arg_conts[0];
}

/* Invoke a byte[] method. Construct a byte[] object and return it. */
JNIEXPORT jbyteArray JNICALL Java_com_fis_gtm_ji_GTMCI_doByteArrayCallin
(JNIEnv *env, jclass obj, jstring name, jobjectArray args)
{
	DO_WORK(env, JAVA_BYTE_ARRAY, name, args, JNI_TRUE, NULL);
	return arg_conts[0];
}

JNIEXPORT jboolean JNICALL Java_com_fis_gtm_ji_GTMCI_doInit
(JNIEnv *env, jclass obj)
{
	char msgbuf[BUF_LEN];

	/* If the environment is set, call gtm_jinit(). Do it on library load, so that all subsequent threads
	 * would have things set up.
	 */
	if (0 != gtm_jinit_fptr())
	{
		gtm_zstatus_fptr(msgbuf, BUF_LEN);
		THROW_EXCEPTION_AND_RETURN(env, JNI_FALSE, "gtm_jinit() failed with the return code %s.", msgbuf);
	}
	return JNI_TRUE;
}

/* Rundown the call-in interface when the Java application is shut down. We rely on this method
 * rather than on JNI_OnUnload because there is no guarantee that the latter will get called. This
 * is achieved by using a Runtime shutdown hook, registered in the static context of the GTMCI class.
 */
JNIEXPORT void JNICALL Java_com_fis_gtm_ji_GTMCI_doShutdown
(JNIEnv *env, jclass obj)
{
	char msgbuf[BUF_LEN];

	if (0 != gtm_exit_fptr())
	{
		gtm_zstatus_fptr(msgbuf, BUF_LEN);
		THROW_EXCEPTION_AND_RETURN(env, /* VOID */, "gtm_exit() failed: %s", msgbuf);
	}
	return;
}

/* This is the function that communicates with GT.M via the call-ins interface and takes care of argument
 * packaging, type conversion, resource allocation and deallocation, and so on.
 */
jint do_work(JNIEnv *env, jstring name, jobjectArray args, int size, jboolean needs_return)
{
	int			i, j, k, len, incr;
	int			offset, non_ret_arg;
	int			jstr_idx = 0, jarr_idx = 0;
	gtm_int_t		int_value;
	jlong			jlong_value;
	gtm_float_t		float_value;
	gtm_double_t		double_value;
	const gtm_char_t	*string_value;
	gtm_string_t		*gtm_string_value;
	gtm_status_t    	status;
	char			msgbuf[BUF_LEN], namebuf[BUF_LEN];
	char			temp_char;
	jint			type;
	jstring			jstr;
	jbyteArray		byte_array;
	jbyte			*byte_array_ptr;
	jobject			arg;
	unsigned int		io_vars_mask, has_return, mask;

	/* Make sure the call-in function name fits our buffer. */
	len = (*env)->GetStringLength(env, name);
	if (BUF_LEN < len)
	{
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 2, "Entryref exceeds %d characters.", BUF_LEN);
	}
	(*env)->GetStringUTFRegion(env, name, 0, len, namebuf);

	/* Increment by two slots on 32-bit machines, where doubles and 8-byte longs do not fit in one. */
	incr = GTM64_ONLY(1) NON_GTM64_ONLY(2);

	/* Because of different argument types, reset the argument list storage. This also ensures that the missing
	 * arguments will be NULL instead of some random values.
	 */
	memset(arg_list, 0, (SIZEOF(char *) * GTM64_ONLY(1) NON_GTM64_ONLY(2) * MAX_PARAMS));

	/* If we are expecting a return value, leave enough room in the arg_list array for it. Also, be sure to use
	 * a gtm_string_t type when dealing with either of the three types below.
	 */
	if (needs_return)
	{
		j = incr;
		type = arg_types[0];
		if ((JAVA_STRING == type) || (JAVA_BYTE_ARRAY == type))
		{
			gtm_strs[0].length = 0;
			gtm_strs[0].address = (gtm_char_t *)NULL;
			*(gtm_string_t **)arg_list = gtm_strs;
		}
		offset = 1;
	} else
		j = offset = 0;

	/* Number of arguments passed plus return value should be no larger than MAX_PARAMS. */
	if (MAX_PARAMS < size)
	{
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
			"Number of parameters to entryref '%s' exceeds %d.", namebuf, MAX_PARAMS);
	}

	/* Process all arguments, input-only and input-output, the same way, since we would still have to copy the
	 * input-output (in addition to output-only) values on the way out.
	 */
	for (i = 0; i < size; i++, j += incr)
	{
		GET_ARG(env, namebuf, args, arg, type, i, offset, jstr_idx, jarr_idx);
		switch (type)
		{
			case GTM_BOOLEAN:
				int_value = (*env)->GetBooleanField(env, arg, value_field_ids[GTM_BOOLEAN]) ? 1 : 0;
				*(gtm_int_t *)(arg_list + j) = int_value;
				break;
			case GTM_INTEGER:
				int_value = (*env)->GetIntField(env, arg, value_field_ids[GTM_INTEGER]);
				*(gtm_int_t *)(arg_list + j) = int_value;
				break;
			case GTM_LONG:
				jlong_value = (*env)->GetLongField(env, arg, value_field_ids[GTM_LONG]);
				*(jlong *)(arg_list + j) = jlong_value;
				break;
			case GTM_FLOAT:
				float_value = (*env)->GetFloatField(env, arg, value_field_ids[GTM_FLOAT]);
				*(gtm_float_t *)(arg_list + j) = float_value;
				break;
			case GTM_DOUBLE:
				double_value = (*env)->GetDoubleField(env, arg, value_field_ids[GTM_DOUBLE]);
				*(gtm_double_t *)(arg_list + j) = double_value;
				break;
			case GTM_STRING:
				jstr = (*env)->GetObjectField(env, arg, value_field_ids[GTM_STRING]);
				if (NULL == jstr)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Passing a null reference in the 'value' field of arg #%d to entryref '%s'.",
						i + 1, namebuf);
				}
				len = (*env)->GetStringUTFLength(env, jstr);
				if (MAX_STRLEN < len)
				{
				    CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
					"Length of 'value' field in arg #%d to entryref '%s' exceeds the capacity of M variables.",
					i + 1, namebuf);
				}
				string_value = (*env)->GetStringUTFChars(env, jstr, NULL);
				if (NULL == string_value)
				{
				    CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
					"Obtaining UTF-8 representation of the 'value' field for arg #%d of entryref '%s' failed.",
					i + 1, namebuf);
				}
				gtm_strs[j].length = len;
				gtm_strs[j].address = (gtm_char_t *)string_value;
				*(gtm_string_t **)(arg_list + j) = gtm_strs + j;
				/* Cache references to UTF-8 string allocated by JVM and the original Java string to be able
				 * to release them later.
				 */
				jstr_ptrs[jstr_idx] = jstr;
				str_ptrs[jstr_idx++] = string_value;
				break;
			case GTM_BYTE_ARRAY:
				byte_array = (jbyteArray)(*env)->GetObjectField(env, arg, value_field_ids[GTM_BYTE_ARRAY]);
				if (NULL == byte_array)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Passing a null reference in the 'value' field of arg #%d to entryref '%s'.",
						i + 1, namebuf);
				}
				len = (*env)->GetArrayLength(env, byte_array);
				if (MAX_STRLEN < len)
				{
				    CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
					"Length of 'value' field in arg #%d to entryref '%s' exceeds the capacity of M variables.",
					i + 1, namebuf);
				}
				byte_array_ptr = (*env)->GetByteArrayElements(env, byte_array, NULL);
				if (NULL == byte_array_ptr)
				{
				    CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
					"Obtaining byte array reference from 'value' field of arg #%d to entryref '%s' failed.",
					i + 1, namebuf);
				}
				byte_array_lens[i + offset] = len;
				gtm_strs[j].length = len;
				gtm_strs[j].address = (gtm_char_t *)byte_array_ptr;
				*(gtm_string_t **)(arg_list + j) = gtm_strs + j;
				/* Cache references to byte array copy allocated by JVM and original byte array to be able
				 * to release them later.
				 */
				byte_array_indices[i + offset] = jarr_idx;
				jarr_ptrs[jarr_idx] = byte_array;
				arr_ptrs[jarr_idx++] = byte_array_ptr;
				break;
			case JAVA_STRING:
				jstr = (jstring)arg;
				assert(NULL != jstr);
				len = (*env)->GetStringUTFLength(env, jstr);
				if (MAX_STRLEN < len)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Value of arg #%d to entryref '%s' exceeds the capacity of M variables.",
						i + 1, namebuf);
				}
				string_value = (*env)->GetStringUTFChars(env, jstr, NULL);
				if (NULL == string_value)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Obtaining UTF-8 representation of arg #%d to entryref '%s' failed.",
						i + 1, namebuf);
				}
				gtm_strs[j].length = len;
				gtm_strs[j].address = (gtm_char_t *)string_value;
				*(gtm_string_t **)(arg_list + j) = gtm_strs + j;
				/* Cache references to UTF-8 string allocated by JVM and the original Java string to be able
				 * release them later.
				 */
				jstr_ptrs[jstr_idx] = jstr;
				str_ptrs[jstr_idx++] = string_value;
				break;
			case JAVA_BYTE_ARRAY:
				byte_array = (jbyteArray)arg;
				assert(NULL != byte_array);
				len = (*env)->GetArrayLength(env, byte_array);
				if (MAX_STRLEN < len)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Length of arg #%d to entryref '%s' exceeds the capacity of M variables.",
						i + 1, namebuf);
				}
				byte_array_ptr = (*env)->GetByteArrayElements(env, byte_array, NULL);
				if (NULL == byte_array_ptr)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Obtaining byte array reference from arg #%d to entryref '%s' failed.",
						i + 1, namebuf);
				}
				byte_array_lens[i + offset] = len;
				gtm_strs[j].length = len;
				gtm_strs[j].address = (gtm_char_t *)byte_array_ptr;
				*(gtm_string_t **)(arg_list + j) = gtm_strs + j;
				/* Cache references to byte array copy allocated by JVM and original byte array to be able
				 * to release them later.
				 */
				byte_array_indices[i + offset] = jarr_idx;
				jarr_ptrs[jarr_idx] = byte_array;
				arr_ptrs[jarr_idx++] = byte_array_ptr;
				break;
			case JAVA_BIG_DECIMAL:
				jstr = (jstring)(*env)->CallObjectMethod(env, arg, java_big_decimal_tostring);
				if (NULL == jstr)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Obtaining a String representation of arg #%d to entryref '%s' failed.", i + 1,
						namebuf);
				}
				len = (*env)->GetStringUTFLength(env, jstr);
				if (MAX_STRLEN < len)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Value of arg #%d to entryref '%s' exceeds the capacity of M variables.", i + 1,
						namebuf);
				}
				string_value = (*env)->GetStringUTFChars(env, jstr, NULL);
				if (NULL == string_value)
				{
					CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
						"Obtaining UTF-8 representation of arg #%d to entryref '%s' failed.", i + 1,
						namebuf);
				}
				gtm_strs[j].length = len;
				gtm_strs[j].address = (gtm_char_t *)string_value;
				*(gtm_string_t **)(arg_list + j) = gtm_strs + j;
				/* Cache references to UTF-8 string allocated by JVM and the original Java string to be able
				 * to release them later.
				 */
				jstr_ptrs[jstr_idx] = jstr;
				str_ptrs[jstr_idx++] = string_value;
				break;
			default:
				CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
					"Type of arg #%d to entryref '%s' is unrecognized.", i + 1, namebuf);
		}
	}

	/* Actually invoke the call-in method. */
	has_return = (JNI_TRUE == needs_return);
	status = gtm_cij_fptr(namebuf, arg_list, size, &arg_types[0], &io_vars_mask, &has_return);
	if (0 != status)
	{
        	gtm_zstatus_fptr(msgbuf, BUF_LEN);
		/* Note the negative count to avoid prepending GTM's error prefix twice. */
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, -2, "%s", msgbuf);
	}

	/* Error out if we expect a return from a void function or expect no return from a non-void function. */
	if (needs_return && !has_return)
	{
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 2,
			"Expecting a return value from a void entryref '%s'.", namebuf);
	} else if (!needs_return && has_return)
	{
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 2,
			"Expecting void return from a non-void entryref '%s'.", namebuf);
	}

	/* If arg_types[0] was reset to a negative value, gtm_cij() has detected a faulty condition that it now wants us to report.
	 * In particular, -1 indicates a type mismatch and -2 indicates that the specified type is not recognized.
	 */
	if (-1 == arg_types[0])
	{	/* Return type specified is wrong. */
		if (0 == arg_types[1])
		{
			CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 4,
				"Wrong return type for entryref '%s': %s expected but %s found.",
				namebuf, ret_type_names[arg_types[2]], ret_type_names[arg_types[3]]);
		} else
		{	/* For gtm_jstring_t either GTMString or String can be used, so differentiate; same for gtm_jbyte_t type,
			 * for which both GTMByteArray and byte[] are good.
			 */
			if ((GTM_STRING != arg_types[2]) && (GTM_BYTE_ARRAY != arg_types[2]))
			{
				CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 5,
					"Wrong type used for arg #%d to entryref '%s': %s expected but %s found.",
					arg_types[1], namebuf, arg_type_names[arg_types[2]], arg_type_names[arg_types[3]]);
			} else
			{
				CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 6,
					"Wrong type used for arg #%d to entryref '%s': %s or %s expected but %s found.",
					arg_types[1], namebuf, arg_type_names[arg_types[2]],
					arg_type_names[arg_types[3]], arg_type_names[arg_types[4]]);
			}
		}
	} else if (-2 == arg_types[0])
	{
		CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
			"Invalid expected type for arg #%d to entryref '%s'.", arg_types[1], namebuf);
	}

	/* Now we need to copy back every variable that has been masked as input-output or output-only. */
	for (i = 0, j = 0, k = 0, size += offset; i < size; i++, k += incr)
	{
		if ((0 == i) && has_return)
		{	/* Unconditionally mask the return value for output. */
			mask = 1;
			non_ret_arg = 0;
		} else
		{	/* For regular arguments check the mask that GT.M returned. */
			mask = io_vars_mask & 1;
			io_vars_mask >>= 1;
			non_ret_arg = 1;
		}

		/* Set the value back inside the container. */
		if (mask)
		{
			switch (arg_types[i])
			{
				case GTM_BOOLEAN:
					if (non_ret_arg)
						(*env)->SetBooleanField(env, arg_conts[i], value_field_ids[GTM_BOOLEAN],
							*(int *)(&arg_list[k]) ? JNI_TRUE : JNI_FALSE);
					break;
				case GTM_INTEGER:
					if (non_ret_arg)
						(*env)->SetIntField(env, arg_conts[i], value_field_ids[GTM_INTEGER],
							*(int *)(&arg_list[k]));
					break;
				case GTM_LONG:
					if (non_ret_arg)
						(*env)->SetLongField(env, arg_conts[i], value_field_ids[GTM_LONG],
							*(jlong *)(&arg_list[k]));
					break;
				case GTM_FLOAT:
					if (non_ret_arg)
						(*env)->SetFloatField(env, arg_conts[i], value_field_ids[GTM_FLOAT],
							*(float *)(&arg_list[k]));
					break;
				case GTM_DOUBLE:
					if (non_ret_arg)
						(*env)->SetDoubleField(env, arg_conts[i], value_field_ids[GTM_DOUBLE],
							*(double *)(&arg_list[k]));
					break;
				case GTM_STRING:
					assert(non_ret_arg);
					/* When dealing with a GTMString object masked for output, we temporarily introduce the null
					 * terminator in memory where it should logically be found. To do that, we need to guarantee
					 * that the byte we are temporarily changing is in the stringpool, as the string itself. We
					 * ensure that on the GT.M side by allocating an extra byte in the stringpool if the string
					 * is sitting at the very end of it.
					 */
					gtm_string_value = (gtm_string_t *)arg_list[k];
					assert(NULL != gtm_string_value);
					if (0 != gtm_string_value->length)
					{
						if ('\0' != gtm_string_value->address[gtm_string_value->length])
						{
							temp_char = gtm_string_value->address[gtm_string_value->length];
							gtm_string_value->address[gtm_string_value->length] = '\0';
							jstr = (*env)->NewStringUTF(env, (char *)gtm_string_value->address);
							gtm_string_value->address[gtm_string_value->length] = temp_char;
						} else	/* If we are already NULL-terminated, avoid the above assignments. */
							jstr = (*env)->NewStringUTF(env, (char *)gtm_string_value->address);
					} else
					{
						temp_char = '\0';
						jstr = (*env)->NewStringUTF(env, &temp_char);
					}
					if (NULL == jstr)
					{
						CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 3,
							"Updating UTF-8 'value' field of arg #%d to entryref '%s' failed.",
							i + offset + 1, namebuf);
					}
					(*env)->SetObjectField(env, arg_conts[i], value_field_ids[GTM_STRING], jstr);
					break;
				case GTM_BYTE_ARRAY:
					assert(non_ret_arg);
					gtm_string_value = (gtm_string_t *)arg_list[k];
					assert(NULL != gtm_string_value);
					/* Since we do not want to allocate resources unnecessarily, only create a new byte array
					 * object if the length of the updated storage exceeds the length of the original array,
					 * which we captured in byte_array_lens[i]. Otherwise, update a portion of the array.
					 */
					if (byte_array_lens[i] < gtm_string_value->length)
					{
						byte_array = (*env)->NewByteArray(env, gtm_string_value->length);
						(*env)->SetByteArrayRegion(env, byte_array, 0, gtm_string_value->length,
							(jbyte *)gtm_string_value->address);
						(*env)->SetObjectField(env, arg_conts[i],
							value_field_ids[GTM_BYTE_ARRAY], byte_array);
					} else
						(*env)->SetByteArrayRegion(env, jarr_ptrs[byte_array_indices[i]], 0,
							gtm_string_value->length, (jbyte *)gtm_string_value->address);
					break;
				case JAVA_STRING:
					/* Java Strings are immutable, so only create an object if we are dealing with a return. */
					if (non_ret_arg)
						break;
					/* When dealing with a GTMString object masked for output, we temporarily introduce the null
					 * terminator in memory where it should logically be found. To do that, we need to guarantee
					 * that the byte we are temporarily changing is in the stringpool, as the string itself. We
					 * ensure that on the GT.M side by allocating an extra byte in the stringpool if the string
					 * is sitting at the very end of it.
					 */
					gtm_string_value = (gtm_string_t *)arg_list[k];
					assert(NULL != gtm_string_value);
					if (0 != gtm_string_value->length)
					{
						temp_char = gtm_string_value->address[gtm_string_value->length];
						gtm_string_value->address[gtm_string_value->length] = '\0';
						jstr = (*env)->NewStringUTF(env, (char *)gtm_string_value->address);
						gtm_string_value->address[gtm_string_value->length] = temp_char;
					} else
					{
						temp_char = '\0';
						jstr = (*env)->NewStringUTF(env, &temp_char);
					}
					if (NULL == jstr)
					{
						CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 2,
							"Constructing UTF-8-encoded return from entryref '%s' failed.", namebuf);
					}
					arg_conts[0] = jstr;
					break;
				case JAVA_BYTE_ARRAY:
					/* In case of a return value, we construct a new byte[] object; otherwise, we update the
					 * existing (passed) argument.
					 */
					gtm_string_value = (gtm_string_t *)arg_list[k];
					assert(NULL != gtm_string_value);
					if (non_ret_arg)
					{	/* Only copy back what fits in the original array; however, only copy what is
						 * required, not necessarily the full length of the array. */
						(*env)->SetByteArrayRegion(env, jarr_ptrs[byte_array_indices[i]], 0,
							(byte_array_lens[i] < gtm_string_value->length)
								? byte_array_lens[i]
								: gtm_string_value->length,
							(jbyte *)gtm_string_value->address);
					} else
					{
						arg_conts[0] = (*env)->NewByteArray(env, gtm_string_value->length);
						if (NULL == arg_conts[0])
						{
							CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_ERR, 2,
								"Constructing a byte[] return value from entryref '%s' failed.",
								namebuf);
						}
						(*env)->SetByteArrayRegion(env, arg_conts[0], 0, gtm_string_value->length,
							(jbyte *)gtm_string_value->address);
					}
					break;
			}
		}
	}

	CLEAN_UP_AND_RETURN(env, jstr_idx, jarr_idx, JNI_OK, 0, " ");
}
