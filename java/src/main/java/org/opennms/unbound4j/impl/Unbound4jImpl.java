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
        return Interface.reverse_lookup(ctx.getId(), addr.getAddress()).thenApply(Optional::ofNullable);
    }

}
