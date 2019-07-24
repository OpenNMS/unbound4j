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

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.anyOf;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.nullValue;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.opennms.unbound4j.api.Unbound4jConfig;

public class InterfaceTest {

    @Rule
    public TemporaryFolder tempFolder = new TemporaryFolder();

    private int ctx;

    @BeforeClass
    public static void setUpClass() {
        Interface.init();
    }
    
    @Before
    public void setUp() {
        ctx = Interface.create_context(Unbound4jConfig.newBuilder()
                .useSystemResolver(true)
                .withRequestTimeout(15, TimeUnit.SECONDS)
                .build());
    }

    @After
    public void tearDown() {
        Interface.delete_context(ctx);
        ctx = -1;
    }

    @Test(timeout = 30000)
    public void canReverseLoookup() throws UnknownHostException, ExecutionException, InterruptedException {
        // IPv4
        byte[] addr = InetAddress.getByName("1.1.1.1").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), anyOf(equalTo("one.one.one.one."), nullValue()));

        // IPv6
        addr = InetAddress.getByName("2606:4700:4700::1111").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), anyOf(equalTo("one.one.one.one."), nullValue()));

        // No result
        addr = InetAddress.getByName("198.51.100.1").getAddress();
        assertThat(Interface.reverse_lookup(ctx, addr).get(), nullValue());
    }

}
