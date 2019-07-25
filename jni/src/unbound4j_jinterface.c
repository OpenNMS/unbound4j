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

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <limits.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <jni.h>
#include <pthread.h>
#include <unbound.h>
#include <sys/select.h>

#if 0
#pragma export on
#endif
#include "unbound4j_java_interface.h"
#if 0
#pragma export reset
#endif

#include <unbound.h>

#include "jniutils.h"
#include "unbound4j.h"
#include "log.h"

struct ub4j_java_refs {
    jclass completableFuture;
    jmethodID completableFuture_complete;
    jmethodID completableFuture_constructor;
};

struct ub4j_java_refs g_java_refs;

struct ub4j_java_callback_context {
    jobject future;
};

JavaVM* g_vm;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    printf("unbound4j: Loaded with libunbound v%s\n", ub_version());
    fflush(stdout);
    g_vm = vm;

    JNIEnv *env;
    if ((*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK) {
        return JNI_ERR;
    }

    memset(&g_java_refs, 0, sizeof(struct ub4j_java_refs));
    // Lookups classes/methods that we know we'll need and convert these to global refs so that they will remain
    // accessible by other threads
    g_java_refs.completableFuture = (*env)->FindClass(env, "java/util/concurrent/CompletableFuture");
    if (g_java_refs.completableFuture == NULL) {
        log_fatal("unbound4j: Failed to find class for CompletableFuture.");
        fflush(stdout);
        return JNI_ERR;
    }
    g_java_refs.completableFuture = (*env)->NewGlobalRef(env, g_java_refs.completableFuture);
    if (g_java_refs.completableFuture == NULL) {
        log_fatal("unbound4j: Failed to convert CompletableFuture class to global reference.");
        fflush(stdout);
        return JNI_ERR;
    }
    g_java_refs.completableFuture_constructor = (*env)->GetMethodID(env, g_java_refs.completableFuture, "<init>", "()V");
    if (g_java_refs.completableFuture_constructor == NULL) {
        log_fatal("unbound4j: Failed to find constructor on CompletableFuture.");
        fflush(stdout);
        return JNI_ERR;
    }
    g_java_refs.completableFuture_complete = (*env)->GetMethodID(env, g_java_refs.completableFuture, "complete", "(Ljava/lang/Object;)Z");
    if (g_java_refs.completableFuture_complete == NULL) {
        log_fatal("unbound4j: Failed to find complete method on CompletableFuture.");
        fflush(stdout);
        return JNI_ERR;
    }

    ub4j_init();

    return JNI_VERSION_1_8;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    printf("unbound4j: Unloaded\n");
    fflush(stdout);

    ub4j_destroy();
}

JNIEXPORT jstring JNICALL Java_org_opennms_unbound4j_impl_Interface_version(JNIEnv *env, jclass clazz) {
    return (*env)->NewStringUTF(env, ub_version());
}

JNIEXPORT jint JNICALL Java_org_opennms_unbound4j_impl_Interface_create_1context(JNIEnv *env, jclass clazz, jobject config) {
    // Map the configuration from the POJO to the C struct
    struct ub4j_config ub4jconf;

    char *unbound4jConfigClassName = "org/opennms/unbound4j/api/Unbound4jConfig";
    jclass unbound4jConfigClazz = (*env)->FindClass(env, unbound4jConfigClassName);
    if (unbound4jConfigClazz == NULL) {
        throwNoClassDefError( env, unbound4jConfigClassName);
        return -1;
    }

    // javap -cp unbound4j-1.0.0-SNAPSHOT.jar -s org.opennms.unbound4j.api.Unbound4jConfig
    // public boolean isUseSystemResolver();
    //     descriptor: ()Z
    jmethodID isUseSystemResolverMethod = (*env)->GetMethodID(env, unbound4jConfigClazz, "isUseSystemResolver", "()Z");
    if (isUseSystemResolverMethod == NULL) {
        throwRuntimeException(env, "isUseSystemResolver method not found.");
        return -1;
    }

    // public java.lang.String getUnboundConfig();
    //    descriptor: ()Ljava/lang/String;
    jmethodID getUnboundConfigMethod = (*env)->GetMethodID(env, unbound4jConfigClazz, "getUnboundConfig", "()Ljava/lang/String;");
    if (getUnboundConfigMethod == NULL) {
        throwRuntimeException(env, "getUnboundConfigMethod method not found.");
        return -1;
    }

    //  public int getRequestTimeoutSeconds();
    //    descriptor: ()I
    jmethodID getRequestTimeoutSecondsMethod = (*env)->GetMethodID(env, unbound4jConfigClazz, "getRequestTimeoutSeconds", "()I");
    if (getRequestTimeoutSecondsMethod == NULL) {
        throwRuntimeException(env, "getRequestTimeoutSeconds method not found.");
        return -1;
    }

    ub4jconf.use_system_resolver = (*env)->CallBooleanMethod(env, config, isUseSystemResolverMethod);
    jobject unboundConfig = (*env)->CallObjectMethod(env, config, getUnboundConfigMethod);
    const char *unboundConfigStr = NULL;
    if (unboundConfig != NULL) {
        unboundConfigStr = (*env)->GetStringUTFChars(env, unboundConfig, NULL);
    }
    ub4jconf.unbound_config = unboundConfigStr;
    ub4jconf.request_timeout_secs = (*env)->CallIntMethod(env, config, getRequestTimeoutSecondsMethod);

    char error_str[256];
    size_t error_str_len = sizeof(error_str);
    struct ub4j_context *ctx = ub4j_create_context(&ub4jconf, error_str, error_str_len);
    long nret;
    if (ctx == NULL) {
        throwRuntimeException(env, error_str);
        nret = -1;
    } else {
        nret = ctx->id;
    }

    if (unboundConfigStr != NULL) {
        (*env)->ReleaseStringUTFChars(env, unboundConfig, unboundConfigStr);
    }

    return nret;
}

JNIEXPORT void JNICALL Java_org_opennms_unbound4j_impl_Interface_delete_1context(JNIEnv *env, jclass clazz, jint ctx_id) {
    char error_str[256];
    size_t error_str_len = sizeof(error_str);
    if(ub4j_delete_context(ctx_id, error_str, error_str_len)) {
        throwRuntimeException(env, error_str);
    }
}

void callback(void* mydata, const char* err_str, char* result) {
    struct ub4j_java_callback_context* ctx = (struct ub4j_java_callback_context*)mydata;

    // We can't share the JNIEnv reference between threads, so we need to grab a new one here
    JNIEnv *env;
    int getEnvStat = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_8);
    if (getEnvStat == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, (void **)&env, NULL) != 0) {
            log_fatal("unbound4j: Fatal error. Failed to attached current thread to VM.");
            goto cleanup;
        }
    } else if (getEnvStat == JNI_OK) {
        // all good
    } else  {
        log_fatal("unbound4j: Fatal error. Failed to attached current thread to VM.");
        goto cleanup;
    }

    if (err_str != NULL) {
        completeCompletableFutureExceptionally(env, ctx->future, err_str);
    } else if (result != NULL) {
        jstring hostname = (*env)->NewStringUTF(env, result);
        (*env)->CallVoidMethod(env, ctx->future, g_java_refs.completableFuture_complete, hostname);
        jthrowable exc = (*env)->ExceptionOccurred(env);
        if (exc) {
            log_error("unbound4j: Error calling complete on future with host!");
        }
    } else {
        (*env)->CallVoidMethod(env, ctx->future, g_java_refs.completableFuture_complete, NULL);
        jthrowable exc = (*env)->ExceptionOccurred(env);
        if (exc) {
            log_error("unbound4j: Error calling complete on future.");
        }
    }

    (*g_vm)->DetachCurrentThread(g_vm);
    cleanup:
        free(ctx);
        if (result != NULL) {
            free(result);
        }
}

JNIEXPORT jobject JNICALL Java_org_opennms_unbound4j_impl_Interface_reverse_1lookup(JNIEnv *env, jclass clazz, jint ctx_id, jbyteArray addr_bytes) {
    jobject future = (*env)->NewObject(env, g_java_refs.completableFuture, g_java_refs.completableFuture_constructor);
    // convert the future to a global reference (otherwise the local ref will die after this method call)
    future = (*env)->NewGlobalRef(env, future);

    struct ub4j_java_callback_context* callback_context = malloc(sizeof(struct ub4j_java_callback_context));
    callback_context->future = future;

    uint8_t* addr;
    size_t addr_len = as_uint8_array(env, addr_bytes, &addr);

    char error_str[256];
    size_t error_str_len = sizeof(error_str);

    if (ub4j_reverse_lookup(ctx_id, addr, addr_len,
            callback_context, callback,
            error_str, error_str_len)) {
        throwRuntimeException(env, error_str);
    }

    free(addr);
    return future;
}
