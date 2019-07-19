//
// Created by jesse on 18/07/19.
//

#ifndef UNBOUND4J_JNIUTILS_H
#define UNBOUND4J_JNIUTILS_H

#include <jni.h>

jint throwNoClassDefError( JNIEnv *env, char *message );
jint throwNoSuchFieldError( JNIEnv *env, char *message );
jint throwRuntimeException( JNIEnv *env, char *message );
jint throwOutOfMemoryError( JNIEnv *env, char *message );

unsigned char* as_unsigned_char_array(JNIEnv *env, jbyteArray array);

#endif //UNBOUND4J_JNIUTILS_H
