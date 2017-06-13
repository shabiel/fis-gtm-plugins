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
#include <jni.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#ifdef DEBUG
#  include <assert.h>
#endif

#include "gtm_sizeof.h"
#include "gtm_common_defs.h"
#include "gtmxc_types.h"
#include "gtmji.h"
#include "xc_gateway.h"

/* Obtain a reference to the specified class and cache it for further call-out invocations. */
#define FIND_CLASS_REFERENCE(ENV, CLASS, TEMP_VAR, VAR)									\
{															\
	TEMP_VAR = (*ENV)->FindClass(ENV, CLASS);									\
	if (NULL == TEMP_VAR)												\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, 0, type_descr_ptr, no_class_def_found_error_class,				\
			2, "Class %s is not found.", CLASS);								\
	}														\
	VAR = (*ENV)->NewGlobalRef(ENV, TEMP_VAR);									\
}

/* Find the ID of the specified field. */
#define FIND_FIELD_ID(ENV, CLS, NAME, TYPE, VAR)									\
{															\
	VAR = (*ENV)->GetFieldID(ENV, CLS, NAME, TYPE);									\
	if (NULL == VAR)												\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, 0, type_descr_ptr, no_such_field_error_class,				\
			2, "Cannot find field id for %s.", NAME);							\
	}														\
}

/* Find the ID of the specified method. */
#define FIND_METHOD_ID(ENV, CLS, NAME, TYPE, VAR)									\
{															\
	VAR = (*ENV)->GetMethodID(ENV, CLS, NAME, TYPE);								\
	if (NULL == VAR)												\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, 0, type_descr_ptr, no_such_method_error_class,				\
			2, "Cannot find method id for %s.", NAME);							\
	}														\
}

/* Release malloced buffers, set an error return code, prepare an error message for rts_error(), if needed, and return.
 * The IGNORED_EXCEPTION argument is specified whenever we know of an error condition set by JVM (and detected by us)
 * that we opt to report via our own rather than JVM-specific error message.
 */
#define CLEAN_UP_AND_RETURN_ERR(ENV, MB_INDEX, ERR_INDICATOR, IGNORED_EXCEPTION, VAR_ARG_CNT, ...)			\
{															\
	int 			i, len;											\
	jboolean		unhandled_exception = JNI_FALSE, handled_exception = JNI_FALSE;				\
	gtm_char_t		**error_msg_ptr;									\
	const gtm_char_t	*string_value;										\
	jthrowable		exception;										\
	jstring			exception_msg;										\
	const gtm_char_t	utf_error_msg[] =									\
				"Obtaining UTF-8 representation of error message occurred in entryref '%s' failed.",	\
				unknown_error_msg[] = "Unknown error.";							\
															\
	/* Release all malloced buffers. */										\
	for (i = 0; i < MB_INDEX; i++)											\
		gtm_free(malloced_buffs[i]);										\
	/* Set the specified indicator to NULL to notify GT.M about the error. */					\
	*(ERR_INDICATOR) = (char)0xFF;											\
	/* The error_msg_ptr variable will point to the error message; preset it to NULL just to be safe that we would	\
	 * not try to free unmalloced storage in op_fnfgncal.c.								\
	 */														\
	error_msg_ptr = (gtm_char_t **)(ERR_INDICATOR + SIZEOF(char *));						\
	*error_msg_ptr = error_msg_buff;										\
	/* If there is some exception that we want to ignore, better check if one is raised. NOTE the assignment. */	\
	if ((IGNORED_EXCEPTION) && (exception = (*ENV)->ExceptionOccurred(ENV)))					\
	{	/* If we have a match, clear the exception; but either way, set the flags accordingly, to prevent	\
		 * redundant look-ups below.										\
		 */													\
		if ((*ENV)->IsInstanceOf(ENV, exception, IGNORED_EXCEPTION))						\
		{													\
			(*ENV)->ExceptionClear(ENV);									\
			handled_exception = JNI_TRUE;									\
		} else													\
			unhandled_exception = JNI_TRUE;									\
	}														\
	/* If there is any unhandled exception, prepare a string with its content and clear it. NOTE the assignment. */	\
	if (!(handled_exception) && ((unhandled_exception) || (exception = (*ENV)->ExceptionOccurred(ENV))))		\
	{														\
		(*ENV)->ExceptionClear(ENV);										\
		exception_msg = (jstring)(*ENV)->CallObjectMethod(ENV, exception, jthrowable_to_string);		\
		len = (*ENV)->GetStringUTFLength(ENV, exception_msg);							\
		if (0 < len)												\
		{													\
			string_value = (*ENV)->GetStringUTFChars(ENV, exception_msg, NULL);				\
			if (NULL == string_value)									\
			{	/* Could not get UTF-8 chars; set a respective message. */				\
				snprintf(*error_msg_ptr, BUF_LEN, "%s", utf_error_msg);					\
				return JNI_ERR;										\
			}												\
			snprintf(*error_msg_ptr, BUF_LEN, "%s", string_value);						\
			(*ENV)->ReleaseStringUTFChars(ENV, exception_msg, string_value);				\
			return JNI_ERR;											\
		}													\
	} else if (0 < VAR_ARG_CNT)											\
	{	/* There are no unhandled exceptions, but we want to set our specific error message. */			\
		snprintf(*error_msg_ptr, BUF_LEN, __VA_ARGS__);								\
		return JNI_ERR;												\
	}														\
	/* No error message is specified, so indicate an unknown error. */						\
	snprintf(*error_msg_ptr, BUF_LEN, "%s", unknown_error_msg);							\
	return JNI_ERR;													\
}

/* Prepare the specified message, set a special error code, and return an error status. This macro is to be used instead
 * of CLEAN_UP_AND_RETURN_ERR() prior to JVM being created.
 */
#define SET_MSG_AND_RETURN_ERR(ERR_INDICATOR, ...)									\
{															\
	gtm_char_t		**error_msg_ptr;									\
															\
	*(ERR_INDICATOR) = (char)0xFF;											\
	error_msg_ptr = (gtm_char_t **)(ERR_INDICATOR + SIZEOF(char *));						\
	*error_msg_ptr = error_msg_buff;										\
	snprintf(*error_msg_ptr, BUF_LEN, __VA_ARGS__);									\
	return JNI_ERR;													\
}

/* Ensure that no other than expected type is used for each argument. */
#define CHECK_TYPE_MISMATCH(ENV, CLASS, METHOD, ARG, EXP_TYPE, MB_INDEX, ERR_INDICATOR, INDEX)				\
{															\
	if (!(*ENV)->IsInstanceOf(ENV, ARG, class_refs[EXP_TYPE]))							\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, MB_INDEX, ERR_INDICATOR, NULL, 5, 						\
			"Arg #%d to method %s.%s is expected to be of type %s, but different type found.",		\
			INDEX, CLASS, METHOD, arg_type_names[EXP_TYPE]);						\
	}														\
}

/* Construct a proper Java String object from a UTF-8-encoded native char array. */
#define MAKE_JAVA_STRING(ENV, CLASS, METHOD, VAR, CHAR_STAR_VALUE, JSTR, MB_INDEX, ERR_INDICATOR, INDEX)		\
{															\
	CHAR_STAR_VALUE = va_arg(VAR, char *);										\
	JSTR = (*ENV)->NewStringUTF(ENV, *(char **)(CHAR_STAR_VALUE + SIZEOF(xc_long_t)));				\
	if (NULL == JSTR)												\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, MB_INDEX, ERR_INDICATOR, NULL, 4,						\
			"Obtaining UTF-8 string representation of arg #%d to method %s.%s failed.",			\
			INDEX, CLASS, METHOD);										\
	}														\
	*(char **)(CHAR_STAR_VALUE + SIZEOF(xc_long_t)) = NULL;								\
}

/* Construct a proper Java String object from a UTF-8-encoded native char array. */
#define MAKE_JAVA_BYTE_ARRAY(ENV, CLASS, METHOD, VAR, CHAR_STAR_VALUE, LEN, BYTE_ARRAY, MB_INDEX, ERR_INDICATOR, INDEX)	\
{															\
	CHAR_STAR_VALUE = va_arg(VAR, char *);										\
	LEN = *(xc_long_t *)CHAR_STAR_VALUE;										\
	BYTE_ARRAY = (*ENV)->NewByteArray(ENV, LEN);									\
	if (NULL == BYTE_ARRAY)												\
	{														\
		CLEAN_UP_AND_RETURN_ERR(ENV, MB_INDEX, ERR_INDICATOR, NULL, 4,						\
			"Obtaining a byte[] representation of arg #%d to method %s.%s failed.",	INDEX, CLASS, METHOD);	\
	}														\
	(*ENV)->SetByteArrayRegion(ENV, BYTE_ARRAY, 0, LEN, *(jbyte **)(CHAR_STAR_VALUE + SIZEOF(xc_long_t)));		\
	*(char **)(CHAR_STAR_VALUE + SIZEOF(xc_long_t)) = NULL;								\
}

/* Do not allow to create more than one JVM within the same process. */
#define NUM_OF_JVMS	1

/* Set the maximum number of user-specified JVM options. */
#define NUM_OF_JVM_OPTS	50

/* Reference to Object, Throwable, and NoXXXError classes to avoid frequent look-ups. */
STATICDEF jclass	object_class, jthrowable_class;
STATICDEF jclass	no_class_def_found_error_class, no_such_method_error_class, no_such_field_error_class;

/* Reference to toString() method of Throwable class, used to pass the message to GT.M for rts_error(). */
STATICDEF jmethodID	jthrowable_to_string;

/* Cached class, constructor, and attribute references for faster look-ups. */
STATICDEF jclass	class_refs[JTYPES_SIZE];
STATICDEF jmethodID	const_method_ids[JTYPES_SIZE];
STATICDEF jfieldID	value_field_ids[JTYPES_SIZE];

/* The virtual machine instance and variables related to its initialization. */
STATICDEF JavaVM	*jvm;
STATICDEF int		jvm_is_created = FALSE;
STATICDEF char		*classpath, *options;
STATICDEF JNIEnv 	*env;

/* Buffers malloced for argument or return value passing. */
STATICDEF char		*malloced_buffs[MAX_PARAMS];
STATICDEF gtm_char_t	error_msg_buff[BUF_LEN];
jlong			jlong_buffer;

#ifdef __sparc
/* This is a replacement function for strsep(), which is missing on Solaris. */
char *strsep_sol(char **str, const char *delims);
char *strsep_sol(char **str, const char *delims)
{
	char		*token, *pos, *best_pos = NULL;
	const char	*delim_ptr;

	/* If there is nothing to work with, return. */
	if (NULL == *str)
		return NULL;

	/* Set the delimiter pointer to the first delimiter. */
	delim_ptr = delims;

	/* Either way the first token starts at the beginning of the string. */
	token = *str;

	/* Keep checking until we are out of delimiters. */
	while ('\0' != *delim_ptr)
	{	/* If a (closer) delimiter match is found, update the best position. */
		if (NULL != (pos = strchr(*str, *delim_ptr)))
			if ((NULL == best_pos) || (pos < best_pos))
				best_pos = pos;
		/* Move to the next delimiter. */
		delim_ptr++;
	}

	/* If at least one delimiter was found, nullify its place in the string and repoint *str past it. */
	if (NULL != best_pos)
	{
		*str = best_pos + 1;
		*best_pos = '\0';
		return token;
	}

	/* No token found, so the entire string is assumed to be the token. */
	*str = NULL;
	return token;
}
#  define STRSEP	strsep_sol
#else
#  define STRSEP	strsep
#endif

/* Do the Java VM initialization on the first invocation and delegate the function invocation
 * to the do_job function.
 */
long gtm_xcj(int count, char *type_descr_ptr, char *class, char *method, ...)
{
	char		buff[BUF_LEN], *tmp_options_buf;
	JavaVMInitArgs	vm_args;
	JavaVMOption	vm_options[NUM_OF_JVM_OPTS], *vm_option_ptr;
	int		vm_option_count = 0, options_len;
	va_list		var;
	jclass		temp_class;
	jsize		jvm_count;
	JavaVM		*jvm_list[NUM_OF_JVMS];

	va_start(var, method);

	/* If either destination class or method is not specified, there is nothing for us to invoke. */
	if (NULL == class)
	{
		SET_MSG_AND_RETURN_ERR(type_descr_ptr, "Java class is not specified.");
	}
	if (NULL == method)
	{
		SET_MSG_AND_RETURN_ERR(type_descr_ptr, "Java method is not specified.");
	}

	if (!jvm_is_created)
	{	/* The jvm_is_created flag is not set, but if this is a call-out done from a call-in, then the JVM has
		 * actually been started, so detect that fact.
		 */
		if (0 > JNI_GetCreatedJavaVMs(jvm_list, NUM_OF_JVMS, &jvm_count))
		{
			SET_MSG_AND_RETURN_ERR(type_descr_ptr, "Cannot verify existence of JVMs.");
		}

		/* If no JVMs are running, go through the whole initialization; otherwise, simply attach to one.  */
		if (0 == jvm_count)
		{	/* Get the location of either the top directory of the class file or the full path of the jar
			 * file (with the JAR itself in the path), such as /usr/lib/ or /usr/lib/somejar.jar.
			 */
			classpath = getenv("GTMXC_classpath");
			if (NULL == classpath)
			{	/* The env variable is not set yet, so cannot use the CLEAN... macro. */
				SET_MSG_AND_RETURN_ERR(type_descr_ptr, "GTMXC_classpath environment variable not defined.");
			}

			/* Include the user's classpath in JVM configuration. */
			snprintf(buff, BUF_LEN, "-Djava.class.path=%s", classpath);
			vm_options[vm_option_count++].optionString = buff;

			/* Indicate that JVM is launched via GT.M call-outs. */
			vm_options[vm_option_count++].optionString = "-Dgtm.callouts=1";

			/* Reduce JVM's signal usage to avoid conflicts with signal management. */
			vm_options[vm_option_count++].optionString = "-Xrs";
			/* Retrieve any user-specified JVM options. */

			tmp_options_buf = getenv("GTMXC_jvm_options");
			if (NULL != tmp_options_buf)
			{
				options_len = strlen(tmp_options_buf);
				if (1 < options_len)
				{
					options = malloc(options_len + 1);
					strncpy(options, tmp_options_buf, options_len);
					for (vm_option_ptr = &vm_options[vm_option_count];
						NULL != ((*vm_option_ptr).optionString = STRSEP(&options, " |"));)
					{
						if ('\0' != *((*vm_option_ptr).optionString))
						{
							if (!strncmp((*vm_option_ptr).optionString, "-Xrs", 4)
								|| !strncmp((*vm_option_ptr).optionString, "-Djava.class.path", 17))
								continue;
							vm_option_count++;
							if (++vm_option_ptr >= &vm_options[NUM_OF_JVM_OPTS])
								break;
						}
					}
				}
			}

			/* Specify other JVM arguments. */
			vm_args.version = CUR_JNI_VERSION;
			vm_args.options = vm_options;
			vm_args.nOptions = vm_option_count;
			/* Ignore any unrecognized options that begin -X or _ */
			vm_args.ignoreUnrecognized = JNI_TRUE;

			/* Actually create the JVM. */
			if (0 > JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args))
			{	/* The env variable is not set, so cannot use the CLEAN... macro. */
				SET_MSG_AND_RETURN_ERR(type_descr_ptr, "Starting the JVM failed.");
			}

			/* Register the rundown callback. */
			atexit(terminate);
		} else if (0 > (*jvm_list[0])->AttachCurrentThread(jvm_list[0], (void**)&env, NULL))
		{
			SET_MSG_AND_RETURN_ERR(type_descr_ptr, "Cannot attach GT.M thread to an existing JVM instance.");
		}

		jvm_is_created = TRUE;

		/* The following references describe the primitive and reference types in JNI as well as their signatures
		 * and mnemonics usage in field- and method-specific searches:
		 *   http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/types.html
		 *   http://docs.oracle.com/javase/1.5.0/docs/guide/jni/spec/functions.html
		 */

		/* Cache class references for possible types. */
		FIND_CLASS_REFERENCE(env, "java/lang/NoClassDefFoundError", temp_class, no_class_def_found_error_class);
		FIND_CLASS_REFERENCE(env, "java/lang/NoSuchMethodError", temp_class, no_such_method_error_class);
		FIND_CLASS_REFERENCE(env, "java/lang/NoSuchFieldError", temp_class, no_such_field_error_class);
		FIND_CLASS_REFERENCE(env, "java/lang/Object", temp_class, object_class);
		FIND_CLASS_REFERENCE(env, "java/lang/Throwable", temp_class, jthrowable_class);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMBoolean", temp_class, class_refs[GTM_BOOLEAN]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMInteger", temp_class, class_refs[GTM_INTEGER]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMLong", temp_class, class_refs[GTM_LONG]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMFloat", temp_class, class_refs[GTM_FLOAT]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMDouble", temp_class, class_refs[GTM_DOUBLE]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMString", temp_class, class_refs[GTM_STRING]);
		FIND_CLASS_REFERENCE(env, "com/fis/gtm/ji/GTMByteArray", temp_class, class_refs[GTM_BYTE_ARRAY]);

		/* Cache field ids for value lookups. */
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
		FIND_METHOD_ID(env, jthrowable_class, "toString", "()Ljava/lang/String;", jthrowable_to_string);
	}

	return do_job(count, type_descr_ptr, class, method, var);
}

/* Updates the flag regarding JVM's existence. */
void terminate(void)
{
	jvm_is_created = FALSE;
	return;
}

/* Invokes a particular Java method and takes care of argument packaging and type conversion. */
long do_job(int count, char *type_descr_ptr, char *class, char *method, va_list var)
{
	int			i, mb_index = 0;
	xc_long_t		len;
	jclass 			cls;
	jmethodID 		mid;
	jstring 		jstr;
	jobjectArray 		args;
	jobject			arg;
	int			int_value;
	jlong			long_value;
	float			float_value;
	double			double_value;
	char			*char_star_value;
	const gtm_char_t	*string_value;
	jbyteArray		byte_array;
	jbyte			*byte_array_ptr;
	xc_status_t		int_ret_value;
	jlong			long_ret_value;
	char			ret_value_descr, *err_descr_ptr;
	int			exp_count;
	va_list			var_io;

	/* Need a copy, so that we can go through the list again for output values. */
	va_copy(var_io, var);

	assert(MAX_PARAMS >= count);

	/* Make two separate pointers to have direct access to both the return type and argument type descriptors without
	 * using arithmetic. The first byte of types_descr_ptr stores the number of expected arguments, according to the
	 * external calls table; the second byte describes the expected return type, and subsequent bytes---the expected
	 * argument types using a one-character mnemonic which gets deciphered in the JNI layer.
	 */
	err_descr_ptr = type_descr_ptr;
	exp_count = (int)*type_descr_ptr;
	type_descr_ptr++;
	ret_value_descr = *type_descr_ptr;
	type_descr_ptr++;

	/* Find the destination Java class. */
	cls = (*env)->FindClass(env, class);
	if (NULL == cls)
	{
		CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, no_class_def_found_error_class,
			2, "Class %s is not found.", class);
	}

	/* Subtract the type_descr_ptr, class, and method arguments from the count. */
	count -= 3;

	/* Create an array of Objects to pass to Java. */
	args = (*env)->NewObjectArray(env, exp_count, object_class, NULL);
	if (NULL == args)
	{
		CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 2,
			"Construction of arguments array to method %s.%s failed.", class, method);
	}

	/* Go through the argument type descriptors and extract the values accordingly. The lower-case characters mostly
	 * indicate the first letter in the type name; so does the upper-case, only that it is an output type. The two
	 * exceptions are String ('j') and byte[] ('a'). Below is the full table of argument type encodings:
	 * 	 _____________________________________
	 * 	|                                     |
	 * 	| type        non-output     output   |
	 * 	|_____________________________________|
	 * 	|                                     |
	 * 	| boolean        'b'          'B'     |
	 * 	| int            'i'          'I'     |
	 * 	| long           'l'          'L'     |
	 * 	| float          'f'          'F'     |
	 * 	| double         'd'          'D'     |
	 * 	| String         'j'          'J'     |
	 * 	| byte[]         'a'          'A'     |
	 * 	|_____________________________________|
	 *
	 * Note that both float/float * and double/double * pairs only have one block of logic each. The reason is that callg()
	 * function that we use in op_fnfgncal.c to put the passed arguments on the stack, is not smart enough to place floating-
	 * point values in proper registers. Furthermore, the registers designated for floating-point values may vary across
	 * different platforms. To overcome this deficiency, we always pass floats and doubles by reference.
	 */
	for (i = 0; i < count; i++)
	{
		switch (type_descr_ptr[i])
		{
			case 'b': /* boolean (non-output) */
				int_value = va_arg(var, int);
				arg = (*env)->NewObject(env, class_refs[GTM_BOOLEAN], const_method_ids[GTM_BOOLEAN],
					int_value ? JNI_TRUE : JNI_FALSE);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'B': /* boolean (output) */
				int_value = *va_arg(var, int *);
				arg = (*env)->NewObject(env, class_refs[GTM_BOOLEAN], const_method_ids[GTM_BOOLEAN],
					int_value ? JNI_TRUE : JNI_FALSE);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'i': /* int (non-output) */
				int_value = va_arg(var, int);
				arg = (*env)->NewObject(env, class_refs[GTM_INTEGER], const_method_ids[GTM_INTEGER], int_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'I': /* int (output) */
				int_value = *va_arg(var, int *);
				arg = (*env)->NewObject(env, class_refs[GTM_INTEGER], const_method_ids[GTM_INTEGER], int_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'l': /* long (non-output) */
				/* On 32-bit pass 8-byte values via a pointer. */
				long_value = GTM64_ONLY(va_arg(var, jlong)) NON_GTM64_ONLY(*va_arg(var, jlong *));
				arg = (*env)->NewObject(env, class_refs[GTM_LONG], const_method_ids[GTM_LONG], long_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'L': /* long (output) */
				long_value = *va_arg(var, jlong *);
				arg = (*env)->NewObject(env, class_refs[GTM_LONG], const_method_ids[GTM_LONG], long_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'f': /* float (non-output): FALL THROUGH */
			case 'F': /* float (output) */
				float_value = *va_arg(var, float *);
				arg = (*env)->NewObject(env, class_refs[GTM_FLOAT], const_method_ids[GTM_FLOAT], float_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'd': /* double (non-output): FALL THROUGH */
			case 'D': /* double (output) */
				double_value = *va_arg(var, double *);
				arg = (*env)->NewObject(env, class_refs[GTM_DOUBLE], const_method_ids[GTM_DOUBLE], double_value);
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'j': /* String (non-output) */
				MAKE_JAVA_STRING(env, class, method, var, char_star_value,
					jstr, mb_index, err_descr_ptr, i + 1);
				(*env)->SetObjectArrayElement(env, args, i, jstr);
				break;
			case 'J': /* String (output) */
				MAKE_JAVA_STRING(env, class, method, var, char_star_value,
					jstr, mb_index, err_descr_ptr, i + 1);
				arg = (*env)->NewObject(env, class_refs[GTM_STRING], const_method_ids[GTM_STRING], jstr);
				if (NULL == arg)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Construction of GTMString object for arg #%d to method %s.%s failed.",
						i + 1, class, method);
				}
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			case 'a': /* byte[] (non-output) */
				MAKE_JAVA_BYTE_ARRAY(env, class, method, var, char_star_value,
					len, byte_array, mb_index, err_descr_ptr, i + 1);
				(*env)->SetObjectArrayElement(env, args, i, byte_array);
				break;
			case 'A': /* byte[] (output) */
				MAKE_JAVA_BYTE_ARRAY(env, class, method, var, char_star_value,
					len, byte_array, mb_index, err_descr_ptr, i + 1);
				arg = (*env)->NewObject(env, class_refs[GTM_BYTE_ARRAY],
					const_method_ids[GTM_BYTE_ARRAY], byte_array);
				if (NULL == arg)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Construction of GTMByteArray object for arg #%d to method %s.%s failed.",
						i + 1, class, method);
				}
				(*env)->SetObjectArrayElement(env, args, i, arg);
				break;
			default:
				va_end(var);
				CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
					"Type of arg #%d to method %s.%s is unrecognized.", i + 1, class, method);
		}
	}
	va_end(var);

	/* If the user passed fewer arguments than Java side is expecting, make sure the missing
	 * arguments in the array are NULL.
	 */
	for (i = count; i < exp_count; i++)
		(*env)->SetObjectArrayElement(env, args, i, NULL);

	/* Determine the expected return type of the Java function and invoke it accordingly. */
	switch (ret_value_descr)
	{
		case 'v': /* void */
			mid = (*env)->GetStaticMethodID(env, cls, method, "([Ljava/lang/Object;)V");
			if (NULL == mid)
			{
				CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, no_such_method_error_class,
					3, "Method (void)%s.%s not found.", class, method);
			}
			(*env)->CallStaticVoidMethod(env, cls, mid, args);
			break;
		case 'i': /* int */
			mid = (*env)->GetStaticMethodID(env, cls, method, "([Ljava/lang/Object;)I");
			if (NULL == mid)
			{
				CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, no_such_method_error_class,
					3, "Method (int)%s.%s not found.", class, method);
			}
			int_ret_value = (*env)->CallStaticIntMethod(env, cls, mid, args);
			break;
		case 'l': /* long */
			mid = (*env)->GetStaticMethodID(env, cls, method, "([Ljava/lang/Object;)J");
			if (NULL == mid)
			{
				CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, no_such_method_error_class,
					3, "Method (long)%s.%s not found.", class, method);
			}
			long_ret_value = (*env)->CallStaticLongMethod(env, cls, mid, args);
			break;
		default:
			CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 3,
				"Invalid return type expected for method %s.%s.", class, method);
	}

	/* An exception might have occurred in the Java function, in which case prepare an error message for rts_error()
	 * to display and return an error status.
	 */
	if ((*env)->ExceptionCheck(env))
	{
		CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 3, "Method %s.%s failed.", class, method);
	}

	/* Now we need to copy back every variable that has been masked as IO and is of a valid type. */
	for (i = 0; i < count; i++)
	{
		switch (type_descr_ptr[i])
		{
			case 'b': /* boolean (non-output) */
				va_arg(var_io, int);
				break;
			case 'B': /* boolean (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_BOOLEAN, mb_index, err_descr_ptr, i + 1);
				int_value = (*env)->GetBooleanField(env, arg, value_field_ids[GTM_BOOLEAN]) ? 1 : 0;
				*va_arg(var_io, int *) = int_value;
				break;
			case 'i': /* int (non-output) */
				va_arg(var_io, int);
				break;
			case 'I': /* int (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_INTEGER, mb_index, err_descr_ptr, i + 1);
				int_value = (*env)->GetIntField(env, arg, value_field_ids[GTM_INTEGER]);
				*va_arg(var_io, int *) = int_value;
				break;
			case 'l': /* long (non-output) */
				GTM64_ONLY(va_arg(var_io, jlong)) NON_GTM64_ONLY(va_arg(var_io, jlong *));
				break;
			case 'L': /* long (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_LONG, mb_index, err_descr_ptr, i + 1);
				long_value = (*env)->GetLongField(env, arg, value_field_ids[GTM_LONG]);
				*va_arg(var_io, jlong *) = long_value;
				break;
			case 'f': /* float (non-output) */
				va_arg(var_io, float *);
				break;
			case 'F': /* float (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_FLOAT, mb_index, err_descr_ptr, i + 1);
				float_value = (*env)->GetFloatField(env, arg, value_field_ids[GTM_FLOAT]);
				*va_arg(var_io, float *) = float_value;
				break;
			case 'd': /* double (non-output) */
				va_arg(var_io, double *);
				break;
			case 'D': /* double (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_DOUBLE, mb_index, err_descr_ptr, i + 1);
				double_value = (*env)->GetDoubleField(env, arg, value_field_ids[GTM_DOUBLE]);
				*va_arg(var_io, double *) = double_value;
				break;
			case 'j': /* String (non-output) */
				va_arg(var_io, char *);
				break;
			case 'J': /* String (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_STRING, mb_index, err_descr_ptr, i + 1);
				jstr = (*env)->GetObjectField(env, arg, value_field_ids[GTM_STRING]);
				if (NULL == jstr)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Passing back a null reference in the 'value' field of arg #%d to method %s.%s.",
						i + 1, class, method);
				}
				string_value = (*env)->GetStringUTFChars(env, jstr, NULL);
				if (NULL == string_value)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Updating UTF-8-encoded 'value' field of arg #%d to method %s.%s failed.",
						i + 1, class, method);
				}
				len = USTRLEN(string_value);
				if (MAX_STRLEN < len)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Length of updated arg #%d to method %s.%s exceeds the capacity of M variables.",
						i + 1, class, method);
				}
				char_star_value = va_arg(var_io, char *);
				*(xc_long_t *)char_star_value = len;
				char_star_value += SIZEOF(xc_long_t);
				*(char **)char_star_value = (char *)gtm_malloc(len);
				malloced_buffs[mb_index++] = *(char **)char_star_value;
				memcpy(*(char **)char_star_value, string_value, len);
				(*env)->ReleaseStringUTFChars(env, jstr, string_value);
				break;
			case 'a': /* byte[] (non-output) */
				va_arg(var_io, char *);
				break;
			case 'A': /* byte[] (output) */
				arg = (*env)->GetObjectArrayElement(env, args, i);
				CHECK_TYPE_MISMATCH(env, class, method, arg, GTM_BYTE_ARRAY, mb_index, err_descr_ptr, i + 1);
				byte_array = (jbyteArray)(*env)->GetObjectField(env, arg, value_field_ids[GTM_BYTE_ARRAY]);
				if (NULL == byte_array)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Passing back a null reference in the 'value' field of arg #%d to method %s.%s.",
						i + 1, class, method);
				}
				len = (*env)->GetArrayLength(env, byte_array);
				if (MAX_STRLEN < len)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Length of updated arg #%d to method %s.%s exceeds the capacity of M variables.",
						i + 1, class, method);
				}
				byte_array_ptr = (*env)->GetByteArrayElements(env, byte_array, NULL);
				if (NULL == byte_array_ptr)
				{
					CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
						"Passing back an updated byte[] representation of arg #%d to method %s.%s failed.",
						i + 1, class, method);
				}
				char_star_value = va_arg(var_io, char *);
				*(xc_long_t *)char_star_value = len;
				char_star_value += SIZEOF(xc_long_t);
				*(char **)char_star_value = (char *)gtm_malloc(len);
				malloced_buffs[mb_index++] = *(char **)char_star_value;
				memcpy(*(char **)char_star_value, byte_array_ptr, len);
				(*env)->ReleaseByteArrayElements(env, byte_array, byte_array_ptr, 0);
				break;
			default:
				va_end(var_io);
				CLEAN_UP_AND_RETURN_ERR(env, mb_index, err_descr_ptr, NULL, 4,
					"Type of updated arg #%d to method %s.%s is unrecognized.", i + 1, class, method);
		}
		assert(MAX_PARAMS > mb_index);
	}
	va_end(var_io);

	/* Actually return the value now. */
	switch (ret_value_descr)
	{
		case 'v': /* void */
			return 0;
		case 'i': /* int */
			return int_ret_value;
		case 'l': /* long */
#			ifdef GTM64
				return long_ret_value;
#			else
				/* On 32-bit we cannot pass an 8-byte value in a 4-byte return slot, so do it by reference. */
				jlong_buffer = long_ret_value;
				return (long)(&jlong_buffer);
#			endif
	}
	return 0;
}
