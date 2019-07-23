/*
 * Copyright 2019, The OpenNMS Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jniutils.h"

#include <stdlib.h>

// Sourced from https://stackoverflow.com/questions/230689/best-way-to-throw-exceptions-in-jni-code/12215825

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

jint completeCompletableFutureExceptionally( JNIEnv *env, jobject future, const char *message )
{
    jclass cfClass;
    jclass exClass;
    char *cfClassName = "java/util/concurrent/CompletableFuture" ;
    char *exClassName = "java/lang/RuntimeException" ;

    cfClass = (*env)->FindClass( env, cfClassName );
    if ( cfClass == NULL ) {
        return throwNoClassDefError( env, cfClassName );
    }

    jmethodID completeExceptionally = (*env)->GetMethodID(env, cfClass, "completeExceptionally", "(Ljava/lang/Throwable;)Z");
    if (completeExceptionally == NULL) {
        return throwRuntimeException(env, "completeExceptionally method not found.");
    }

    int ret = 0;
    jobject ex = NULL;

    exClass = (*env)->FindClass( env, exClassName );
    if ( exClass == NULL ) {
        throwNoClassDefError( env, exClassName );
        ret = -1;
        goto complete;
    }

    jmethodID exConstructor = (*env)->GetMethodID(env, exClass, "<init>", "(Ljava/lang/String;)V");
    if (exConstructor == NULL) {
        throwRuntimeException(env, "Constructor on RuntimeException not found.");
        ret = -1;
        goto complete;
    }

    jstring messageString = (*env)->NewStringUTF(env, message);
    if (messageString == NULL) {
        throwRuntimeException(env, "Failed to create Java string for exception message");
        ret = -1;
        goto complete;
    }

    ex = (*env)->NewObject(env, exClass, exConstructor, messageString);
    if (ex == NULL) {
        throwRuntimeException(env, "Failed to create new RuntimeException");
        ret = -1;
        goto complete;
    }

    complete:
    (*env)->CallBooleanMethod(env, future, completeExceptionally, ex);
    return ret;
}

int as_uint8_array(JNIEnv *env, jbyteArray array, uint8_t** buf) {
    int len = (*env)->GetArrayLength(env, array);
    *buf = malloc(sizeof(uint8_t) * len);
    (*env)->GetByteArrayRegion(env, array, 0, len, (jbyte*)*buf);
    return len;
}
