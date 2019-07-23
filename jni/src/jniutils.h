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
