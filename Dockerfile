# Build the server with the same libc family used by the runtime image.
FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install --no-install-recommends -y cmake g++ make libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target unidb --parallel 2

FROM debian:bookworm-slim AS runtime

RUN useradd --system --uid 10001 --create-home unidb \
    && mkdir -p /data \
    && chown unidb:unidb /data

COPY --from=build /src/build/unidb /usr/local/bin/unidb

# Docker port forwarding requires a non-loopback listener. This is an explicit
# opt-in to the current plaintext/unauthenticated protocol; isolate the network.
ENV UNIDB_BIND_ADDRESS=0.0.0.0
ENV UNIDB_ALLOW_INSECURE_REMOTE_ACCESS=1
EXPOSE 7432
VOLUME ["/data"]
USER unidb

ENTRYPOINT ["/usr/local/bin/unidb"]
CMD ["--server", "--db-path", "/data/database.db", "--port", "7432"]
