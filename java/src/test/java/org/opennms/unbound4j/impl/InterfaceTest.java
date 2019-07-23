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

import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.hasSize;
import static org.hamcrest.Matchers.nullValue;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.opennms.unbound4j.api.Unbound4jConfig;

import com.google.common.base.Stopwatch;
import com.google.common.net.InetAddresses;

public class InterfaceTest {

    @Rule
    public TemporaryFolder tempFolder = new TemporaryFolder();

    private long ctx;

    @BeforeClass
    public static void setUpClass() {
        Interface.init();
    }
    
    @Before
    public void setUp() {
        ctx = Interface.create_context(Unbound4jConfig.newBuilder().build());
    }

    @After
    public void tearDown() {
        Interface.delete_context(ctx);
        ctx = -1;
    }

    @Test
    public void canReverseLoookup() throws UnknownHostException, ExecutionException, InterruptedException {
        // IPv4
        byte[] addr = InetAddress.getByName("1.1.1.1").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), equalTo("one.one.one.one."));

        // IPv6
        addr = InetAddress.getByName("2606:4700:4700::1111").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), equalTo("one.one.one.one."));

        // No result
        addr = InetAddress.getByName("198.51.100.1").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), nullValue());
    }

    @Test
    @Ignore
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
        System.out.printf("Issued %d asynchronous reverse lookups in %dms.\n", futures.size(), stopwatch.elapsed(MILLISECONDS));

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
