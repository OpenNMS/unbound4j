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

import java.net.InetAddress;
import java.util.Optional;
import java.util.concurrent.CompletableFuture;

import org.opennms.unbound4j.api.Unbound4j;
import org.opennms.unbound4j.api.Unbound4jConfig;
import org.opennms.unbound4j.api.Unbound4jContext;

public class Unbound4jImpl implements Unbound4j {

    @Override
    public Unbound4jContext newContext(Unbound4jConfig config) {
        return new Unbound4jContextImpl(Interface.create_context(config));
    }

    @Override
    public CompletableFuture<Optional<String>> reverseLookup(Unbound4jContext ctx, InetAddress addr) {
        final byte[] bytes = addr.getAddress();
        return Interface.reverse_lookup(ctx.getId(), bytes)
                .thenApply(Optional::ofNullable);
    }

}
