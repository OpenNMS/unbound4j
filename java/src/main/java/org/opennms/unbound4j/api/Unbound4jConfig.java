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

package org.opennms.unbound4j.api;

import java.util.Objects;

public class Unbound4jConfig {
    private final boolean useSystemResolver;
    private final String unboundConfig;

    private Unbound4jConfig(Builder builder) {
        this.useSystemResolver = builder.useSystemResolver;
        this.unboundConfig = builder.unboundConfig;
    }

    public static Builder newBuilder() {
        return new Builder();
    }

    public static final class Builder {
        private boolean useSystemResolver = true;
        private String unboundConfig;

        public void useSystemResolver(boolean useSystemResolver) {
            this.useSystemResolver = useSystemResolver;
        }

        public void withUnboundConfig(String unboundConfig) {
            this.unboundConfig = unboundConfig;
        }

        public Unbound4jConfig build() {
            return new Unbound4jConfig(this);
        }
    }

    public boolean isUseSystemResolver() {
        return useSystemResolver;
    }

    public String getUnboundConfig() {
        return unboundConfig;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof Unbound4jConfig)) return false;
        Unbound4jConfig that = (Unbound4jConfig) o;
        return useSystemResolver == that.useSystemResolver &&
                Objects.equals(unboundConfig, that.unboundConfig);
    }

    @Override
    public int hashCode() {
        return Objects.hash(useSystemResolver, unboundConfig);
    }

    @Override
    public String toString() {
        return "Unbound4jConfig{" +
                "useSystemResolver=" + useSystemResolver +
                ", unboundConfig='" + unboundConfig + '\'' +
                '}';
    }
}
