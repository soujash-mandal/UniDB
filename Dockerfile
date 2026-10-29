# Build the server with the same libc family used by the runtime image.
FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install --no-install-recommends -y cmake g++ make libgtest-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target unidb --parallel 2

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install --no-install-recommends -y libssl3 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --create-home unidb \
    && mkdir -p /data /run/unidb-tls \
    && chown unidb:unidb /data /run/unidb-tls

COPY --from=build /src/build/unidb /usr/local/bin/unidb

# Docker port forwarding requires a non-loopback listener. The server still
# requires explicit runtime opt-in before accepting plaintext remote connections.
ENV UNIDB_BIND_ADDRESS=0.0.0.0
EXPOSE 7432
VOLUME ["/data"]
USER unidb

ENTRYPOINT ["/usr/local/bin/unidb"]
CMD ["--server", "--db-path", "/data/database.db", "--port", "7432"]
