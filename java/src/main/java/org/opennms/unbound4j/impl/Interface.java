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

package org.opennms.unbound4j.impl;

import java.util.concurrent.CompletableFuture;

import org.opennms.unbound4j.api.Unbound4jConfig;

/**
 * A native interface to libunbound.
 *
 * Use org.opennms.unbound4j.impl.Unbound4jImpl instead of making
 * calls directly to this interface.
 *
 * @author jwhite
 * @version 1.0.0
 */
public class Interface {

    protected static native String version();

    protected static native int create_context(Unbound4jConfig config);

    protected static native void delete_context(int ctx_id);

    protected static native CompletableFuture<String> reverse_lookup(int ctx_id, byte[] addr);

    /** Load the unbound4j runtime C library. */
    static void init() {
        try {
            NativeLibrary.load();
        } catch (Exception e) {
            /*
             * This code is called during static initialization of this and of other classes.
             * If this fails then a NoClassDefFoundError is thrown however this does not
             * include a cause. Printing the exception manually here ensures that the
             * necessary information to fix the problem is available.
             */
            System.err.println("Failed to load unbound4j native library");
            e.printStackTrace();
            throw e;
        }
    }

    static {
        init();
    }
}
