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

import static org.awaitility.Awaitility.await;
import static org.hamcrest.CoreMatchers.equalTo;

import java.net.InetAddress;
import java.util.Arrays;
import java.util.LinkedList;
import java.util.List;
import java.util.Optional;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Ignore;
import org.junit.Test;
import org.opennms.unbound4j.api.Unbound4jConfig;
import org.opennms.unbound4j.api.Unbound4jContext;

import com.codahale.metrics.ConsoleReporter;
import com.codahale.metrics.Meter;
import com.codahale.metrics.MetricRegistry;
import com.google.common.base.Stopwatch;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.net.InetAddresses;

public class Unbound4PerfTest {

    Unbound4jImpl ub4j = new Unbound4jImpl();
    Unbound4jContext ctx = ub4j.newContext(Unbound4jConfig.newBuilder()
            .useSystemResolver(false)
            .withUnboundConfig("/tmp/unbound.conf")
            .withRequestTimeout(5, TimeUnit.SECONDS)
            .build());

    Cache<InetAddress, Optional<String>> cache = CacheBuilder.newBuilder()
            .expireAfterWrite(1, TimeUnit.MINUTES)
            .build();

    private final Random r = new Random(1);

    private final AtomicBoolean stop = new AtomicBoolean(false);

    private final MetricRegistry metrics = new MetricRegistry();

    private final Meter request = metrics.meter("request");

    private final Meter responseSuccess = metrics.meter("response-success");
    private final Meter responseFailed = metrics.meter("response-failed");
    private final Meter responseCached = metrics.meter("response-cached");

    private class Worker implements Runnable {

        @Override
        public void run() {
            while(!stop.get()) {
                // Generate 3 IP address
                InetAddress src = getNextIPv4Address();
                InetAddress dst = getNextIPv4Address();
                InetAddress gw = getNextHop();
                // Perform lookups for each of these
                Arrays.asList(src, dst, gw).forEach(addr -> {
                    request.mark();
                    Stopwatch stopwatch = Stopwatch.createStarted();
                    Optional<String> hostname = cache.getIfPresent(addr);
                    if (hostname != null) {
                        responseCached.mark();
                    } else {
                        ub4j.reverseLookup(ctx, addr).whenComplete((hostnameFromDns, ex) -> {
                            if (ex == null) {
                                cache.put(addr, hostnameFromDns);
                                System.out.printf("Reverse lookup for %s completed successfully in %dms with: %s\n", addr, stopwatch.elapsed(TimeUnit.MILLISECONDS), hostnameFromDns);
                                responseSuccess.mark();
                            } else {
                                System.out.printf("Reverse lookup for %s failed after %dms with: %s\n", addr, stopwatch.elapsed(TimeUnit.MILLISECONDS), ex);
                                responseFailed.mark();
                            }
                        });
                    }
                });
            }
            System.out.println("Done");
        }
    }

    @Test
    @Ignore
    public void canPerformReverseLookupsWithManyThreads() throws Exception {
        ConsoleReporter reporter = ConsoleReporter.forRegistry(metrics)
                .convertRatesTo(TimeUnit.SECONDS)
                .convertDurationsTo(TimeUnit.MILLISECONDS)
                .build();
        reporter.start(5, TimeUnit.SECONDS);

        List<Thread> threads = new LinkedList<>();
        for (int i = 0; i < 12; i++) {
            Thread t = new Thread(new Worker());
            t.start();
            threads.add(t);
        }

        Thread.sleep(TimeUnit.MINUTES.toMillis(1));

        // Stop the threads
        stop.set(true);
        for (Thread t : threads) {
            t.join();
        }

        // Wait until all the requests have completed
        await().atMost(30, TimeUnit.SECONDS).until(request::getCount, equalTo(responseCached.getCount() + responseSuccess.getCount() + responseFailed.getCount()));
    }

    private AtomicInteger nextIpAddress = new AtomicInteger(16843009); // Start at 1.1.1.1

    private InetAddress getNextIPv4Address() {
        return InetAddresses.fromInteger(nextIpAddress.incrementAndGet());
    }

    private InetAddress getRandomIPv4Address() {
        return InetAddresses.fromInteger(r.nextInt());
    }

    private InetAddress getNextHop() {
        return InetAddresses.fromInteger(0);
    }
}
