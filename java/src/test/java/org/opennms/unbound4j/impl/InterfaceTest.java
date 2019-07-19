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

import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.hasSize;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.opennms.unbound4j.api.UnboundConfig;

import com.google.common.base.Stopwatch;
import com.google.common.net.InetAddresses;

public class InterfaceTest {

    @Rule
    public TemporaryFolder tempFolder = new TemporaryFolder();

    private long ctx;

    @BeforeClass
    public static void setUpClass() {
        librarySearch: for (final String prefix : new String[] { "", "lib" }) {
            for (final String suffix : new String[] { ".so", ".dll", ".jnilib" }) {
                final Path library = Paths.get(System.getProperty("user.dir"), "..", "dist", prefix + "unbound4j" + suffix);
                if (library.toFile().exists()) {
                    System.setProperty("opennms.library.unbound4j", library.toString());
                    break librarySearch;
                }
            }
        }
        Interface.init();
    }
    
    @Before
    public void setUp() {
        ctx = Interface.create_context(UnboundConfig.newBuilder().build());
    }

    @After
    public void tearDown() {
        Interface.delete_context(ctx);
        ctx = -1;
    }

    @Test
    public void canReverseLoookup() throws UnknownHostException, ExecutionException, InterruptedException {
        byte[] addr = InetAddress.getByName("1.1.1.1").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), equalTo("one.one.one.one"));
    }

    @Test
    public void canDoItQuick() throws UnknownHostException, InterruptedException {
        final List<CompletableFuture<String>> futures = new ArrayList<>();
        final Set<String> results = new LinkedHashSet<>();

        Stopwatch stopwatch = Stopwatch.createStarted();
        for (InetAddress addr = InetAddress.getByName("10.0.0.1");
             !addr.equals(InetAddress.getByName("10.2.255.255"));
             addr = InetAddresses.increment(addr)) {
            final CompletableFuture<String> future = Interface.reverse_lookup(ctx, addr.getAddress());
            future.whenComplete((hostname,ex) -> results.add(hostname));
            futures.add(future);
        }
        System.out.printf("Issued %d asynchronous reverse lookups.\n", futures.size());

        // Wait
        try {
            CompletableFuture.allOf(futures.toArray(new CompletableFuture[]{})).get();
        } catch (ExecutionException e) {
            System.out.println("One or more queries failed.");
        }
        stopwatch.stop();
        System.out.printf("Processed %d requests in %dms.\n", futures.size(), stopwatch.elapsed(MILLISECONDS));

        final long numLookupsSuccessful = futures.stream().filter(f -> !f.isCompletedExceptionally()).count();
        final long numLookupsFailed = futures.stream().filter(CompletableFuture::isCompletedExceptionally).count();
        System.out.printf("%d lookups were successful and %d lookups failed.\n", numLookupsSuccessful, numLookupsFailed);

        // Validate
        assertThat(results, hasSize(1));
        assertThat(results, contains((String)null));
    }
}
