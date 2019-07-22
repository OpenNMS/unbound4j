/*******************************************************************************
 * This file is part of OpenNMS(R).
 *
 * Copyright (C) 2019 The OpenNMS Group, Inc.
 * OpenNMS(R) is Copyright (C) 1999-2019 The OpenNMS Group, Inc.
 *
 * OpenNMS(R) is a registered trademark of The OpenNMS Group, Inc.
 *
 * OpenNMS(R) is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * OpenNMS(R) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with OpenNMS(R).  If not, see:
 *      http://www.gnu.org/licenses/
 *
 * For more information contact:
 *     OpenNMS(R) Licensing <license@opennms.org>
 *     http://www.opennms.org/
 *     http://www.opennms.com/
 *******************************************************************************/

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

    protected static native long create_context(Unbound4jConfig config);

    protected static native void delete_context(long ctx_id);

    protected static native CompletableFuture<String> reverse_lookup(long ctx_id, byte[] addr);

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
