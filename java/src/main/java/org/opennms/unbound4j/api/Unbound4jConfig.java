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
