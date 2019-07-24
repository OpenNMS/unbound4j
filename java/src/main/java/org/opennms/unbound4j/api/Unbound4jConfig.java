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
import java.util.concurrent.TimeUnit;

public class Unbound4jConfig {
    private final boolean useSystemResolver;
    private final int requestTimeoutSeconds;
    private final String unboundConfig;

    private Unbound4jConfig(Builder builder) {
        this.useSystemResolver = builder.useSystemResolver;
        this.requestTimeoutSeconds = builder.requestTimeoutSeconds;
        this.unboundConfig = builder.unboundConfig;
    }

    public static Builder newBuilder() {
        return new Builder();
    }

    public static final class Builder {
        private boolean useSystemResolver = true;
        private int requestTimeoutSeconds = 5;
        private String unboundConfig;

        public Builder useSystemResolver(boolean useSystemResolver) {
            this.useSystemResolver = useSystemResolver;
            return this;
        }

        public Builder withUnboundConfig(String unboundConfig) {
            this.unboundConfig = unboundConfig;
            return this;
        }

        public Builder withRequestTimeout(long duration, TimeUnit unit) {
            requestTimeoutSeconds = (int)unit.toSeconds(duration);
            return this;
        }

        public Unbound4jConfig build() {
            return new Unbound4jConfig(this);
        }
    }

    public boolean isUseSystemResolver() {
        return useSystemResolver;
    }

    public int getRequestTimeoutSeconds() {
        return requestTimeoutSeconds;
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
                requestTimeoutSeconds == that.requestTimeoutSeconds &&
                Objects.equals(unboundConfig, that.unboundConfig);
    }

    @Override
    public int hashCode() {
        return Objects.hash(useSystemResolver, requestTimeoutSeconds, unboundConfig);
    }

    @Override
    public String toString() {
        return "Unbound4jConfig{" +
                "useSystemResolver=" + useSystemResolver +
                ", requestTimeoutSeconds=" + requestTimeoutSeconds +
                ", unboundConfig='" + unboundConfig + '\'' +
                '}';
    }
}
