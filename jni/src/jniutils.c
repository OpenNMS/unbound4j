//
// Created by jesse on 18/07/19.
//

#include "jniutils.h"

#include <stdlib.h>

jint throwNoClassDefError( JNIEnv *env, char *message )
{
    jclass exClass;
    char *className = "java/lang/NoClassDefFoundError";

    exClass = (*env)->FindClass( env, className);
    if (exClass == NULL) {
        return throwNoClassDefError( env, className );
    }

    return (*env)->ThrowNew( env, exClass, message );
}

jint throwNoSuchFieldError( JNIEnv *env, char *message )
{
    jclass exClass;
    char *className = "java/lang/NoSuchFieldError" ;

    exClass = (*env)->FindClass( env, className );
    if ( exClass == NULL ) {
        return throwNoClassDefError( env, className );
    }

    return (*env)->ThrowNew( env, exClass, message );
}

jint throwRuntimeException( JNIEnv *env, char *message )
{
    jclass exClass;
    char *className = "java/lang/RuntimeException" ;

    exClass = (*env)->FindClass( env, className );
    if ( exClass == NULL ) {
        return throwNoClassDefError( env, className );
    }

    return (*env)->ThrowNew( env, exClass, message );
}

jint throwOutOfMemoryError( JNIEnv *env, char *message )
{
    jclass exClass;
    char *className = "java/lang/OutOfMemoryError" ;

    exClass = (*env)->FindClass( env, className );
    if ( exClass == NULL ) {
        return throwNoClassDefError( env, className );
    }

    return (*env)->ThrowNew( env, exClass, message );
}

unsigned char* as_unsigned_char_array(JNIEnv *env, jbyteArray array) {
    int len = (*env)->GetArrayLength(env, array);
    unsigned char* buf = malloc(sizeof(unsigned char) * len);
    (*env)->GetByteArrayRegion (env, array, 0, len, (jbyte*)buf);
    return buf;
}