//
// Created by jesse on 18/07/19.
//

#ifndef UNBOUND4J_JNIUTILS_H
#define UNBOUND4J_JNIUTILS_H

#include <jni.h>
#include <stdint.h>

jint throwNoClassDefError( JNIEnv *env, char *message );
jint throwNoSuchFieldError( JNIEnv *env, char *message );
jint throwRuntimeException( JNIEnv *env, char *message );
jint throwOutOfMemoryError( JNIEnv *env, char *message );
jint completeCompletableFutureExceptionally( JNIEnv *env, jobject future, const char *message );

int as_uint8_array(JNIEnv *env, jbyteArray array, uint8_t** buf);

#endif //UNBOUND4J_JNIUTILS_H
