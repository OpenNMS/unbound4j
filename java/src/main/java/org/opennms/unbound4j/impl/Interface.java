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

import java.io.File;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import org.opennms.unbound4j.api.Unbound4jConfig;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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

    private static final Logger LOG = LoggerFactory.getLogger(Interface.class);

    private static final String LIBRARY_NAME = "unbound4j";

    private static final String PROPERTY_NAME = "opennms.library.unbound4j";

    private static boolean m_loaded = false;

    protected static native long create_context(Unbound4jConfig config);

    protected static native void delete_context(long ctx_id);

    protected static native CompletableFuture<String> reverse_lookup(long ctx_id, byte[] addr);

    /**
     * Load the unbound4j library and create the singleton instance of the interface.
     *
     * @throws SecurityException
     *             if we don't have permission to load the library
     * @throws UnsatisfiedLinkError
     *             if the library doesn't exist
     */
    public static synchronized void init() throws SecurityException, UnsatisfiedLinkError {
        if (m_loaded) {
            return;
        }

        final String jniPath = System.getProperty(PROPERTY_NAME);
        try {
            LOG.debug("System property '{}' set to '{}'. Attempting to load {} library from this location.", PROPERTY_NAME,  System.getProperty(PROPERTY_NAME), LIBRARY_NAME);
            System.load(jniPath);
        } catch (final Throwable t) {
            LOG.debug("System property '{}' not set or failed loading. Attempting to find library.", PROPERTY_NAME, LIBRARY_NAME);
            loadLibrary();
        }
        LOG.info("Successfully loaded {} library.", LIBRARY_NAME);
    }

    private static void loadLibrary() {
        final Set<String> searchPaths = new LinkedHashSet<String>();

        if (System.getProperty("java.library.path") != null) {
            searchPaths.addAll(Arrays.asList(System.getProperty("java.library.path").split(File.pathSeparator)));
        }

        searchPaths.addAll(Arrays.asList("/usr/lib64/jni",
                "/usr/lib64",
                "/usr/local/lib64",
                "/usr/lib/jni",
                "/usr/lib",
                "/usr/local/lib"));

        for (final String path : searchPaths) {
            for (final String prefix : new String[] { "", "lib" }) {
                for (final String suffix : new String[] { ".jnilib", ".dylib", ".so" }) {
                    final File f = new File(path + File.separator + prefix + LIBRARY_NAME + suffix);
                    if (f.exists()) {
                        try {
                            System.load(f.getCanonicalPath());
                            return;
                        } catch (final Throwable t) {
                            LOG.trace("Failed to load {} from file {}", LIBRARY_NAME, f, t);
                        }
                    }
                }
            }
        }
        LOG.debug("Unable to locate '{}' in common paths.  Attempting System.loadLibrary() as a last resort.", LIBRARY_NAME);
        System.loadLibrary(LIBRARY_NAME);
    }

    public static synchronized void reload() throws SecurityException, UnsatisfiedLinkError {
        m_loaded = false;
        init();
    }
}
