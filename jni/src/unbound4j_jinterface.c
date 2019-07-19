/*******************************************************************************
 * This file is part of OpenNMS(R).
 *
 * Copyright (C) 2010-2019 The OpenNMS Group, Inc.
 * OpenNMS(R) is Copyright (C) 1999-2019 The OpenNMS Group, Inc.
 *
 * OpenNMS(R) is Copyright (C) 2002-2019 The OpenNMS Group, Inc.  All rights
 * reserved.  OpenNMS(R) is a derivative work, containing both original code,
 * included code and modified code that was published under the GNU General
 * Public License.  Copyrights for modified and included code are below.
 *
 * OpenNMS(R) is a registered trademark of The OpenNMS Group, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License with the Classpath
 * Exception; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * For more information contact:
 *     OpenNMS(R) Licensing <license@opennms.org>
 *     http://www.opennms.org/
 *     http://www.opennms.com/
 *******************************************************************************/
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

#include "jniutils.h"
#include "unbound4j.h"

JavaVM* g_vm;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    printf("unbound4j: Loaded\n");
    fflush(stdout);
    g_vm = vm;

    ub4j_init();

    return JNI_VERSION_1_8;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    printf("unbound4j: Unloaded\n");
    fflush(stdout);

    ub4j_destroy();
}

JNIEXPORT jlong JNICALL Java_org_opennms_unbound4j_impl_Interface_create_1context(JNIEnv *env, jclass clazz, jobject config) {
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

    ub4jconf.use_system_resolver = (*env)->CallBooleanMethod(env, config, isUseSystemResolverMethod);
    jobject unboundConfig = (*env)->CallObjectMethod(env, config, getUnboundConfigMethod);
    const char *unboundConfigStr = NULL;
    if (unboundConfig != NULL) {
        unboundConfigStr = (*env)->GetStringUTFChars(env, unboundConfig, NULL);
    }
    ub4jconf.unbound_config = unboundConfigStr;

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

JNIEXPORT void JNICALL Java_org_opennms_unbound4j_impl_Interface_delete_1context(JNIEnv *env, jclass clazz, jlong ctx_id) {
    char error_str[256];
    size_t error_str_len = sizeof(error_str);
    if(ub4j_delete_context(ctx_id, error_str, error_str_len)) {
        throwRuntimeException(env, error_str);
    }
}

typedef struct {
    jclass string, byteArray, completableFuture;
} Classes;

typedef struct {
    jclass futureClazz;
    jmethodID completeMethodOnFuture;
    jobject future;
} CallbackContext;

int findClasses(JNIEnv *env, Classes* classes);

void callback(void* mydata, const char* err_str, char* result) {
    CallbackContext* ctx = (CallbackContext*)mydata;

    // We can't share the JNIEnv reference between threads, so we need to grab a new one here
    JNIEnv *env;
    int getEnvStat = (*g_vm)->GetEnv(g_vm, (void **)&env, JNI_VERSION_1_8);
    if (getEnvStat == JNI_EDETACHED) {
        if ((*g_vm)->AttachCurrentThread(g_vm, (void **)&env, NULL) != 0) {
            printf("unbound4j: Fatal error. Failed to attached current thread to VM.");
            goto cleanup;
        }
    } else if (getEnvStat == JNI_OK) {
        // all good
    } else  {
        printf("unbound4j: Fatal error. Failed to attached current thread to VM.");
        goto cleanup;
    }

    jclass completableFuture = (*env)->FindClass(env, "java/util/concurrent/CompletableFuture");
    jmethodID complete = (*env)->GetMethodID(env, completableFuture, "complete", "(Ljava/lang/Object;)Z");
    if (err_str != NULL) {
        completeCompletableFutureExceptionally(env, ctx->future, err_str);
    } else if (result != NULL) {
        jstring hostname = (*env)->NewStringUTF(env, result);
        (*env)->CallVoidMethod(env, ctx->future, complete, hostname);
    } else {
        (*env)->CallVoidMethod(env, ctx->future, complete, NULL);
    }

    (*g_vm)->DetachCurrentThread(g_vm);
    cleanup:
        free(ctx);
        if (result != NULL) {
            free(result);
        }
}

JNIEXPORT jobject JNICALL Java_org_opennms_unbound4j_impl_Interface_reverse_1lookup(JNIEnv *env, jclass clazz, jlong ctx_id, jbyteArray addr_bytes) {
    // Grab references to the classes we may need
    Classes classes;
    if (findClasses(env, &classes) == -1) {
        return NULL; // Exception already thrown
    }

    jmethodID constructor = (*env)->GetMethodID(env, classes.completableFuture, "<init>", "()V");
    jobject future = (*env)->NewObject(env, classes.completableFuture, constructor);
    // convert the future to a global reference (otherwise the local ref will die after this method call)
    future = (*env)->NewGlobalRef(env, future);

    CallbackContext* callback_context = malloc(sizeof(CallbackContext));
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

int findClasses(JNIEnv *env, Classes* classes) {
    classes->completableFuture = (*env)->FindClass(env, "java/util/concurrent/CompletableFuture");
    if(classes->completableFuture == NULL || (*env)->ExceptionOccurred(env) != NULL) {
        return -1;
    }

    classes->string = (*env)->FindClass(env, "java/lang/String");
    if(classes->string == NULL || (*env)->ExceptionOccurred(env) != NULL) {
        return -1;
    }

    classes->byteArray = (*env)->FindClass(env, "[B");
    if(classes->byteArray == NULL || (*env)->ExceptionOccurred(env) != NULL) {
        return -1;
    }

    return 0;
}
