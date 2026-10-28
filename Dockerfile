# Build UniDB in a reproducible Linux environment.
FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       ca-certificates cmake g++ make libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target unidb --parallel

# Keep the runtime image small. The working directory is the mounted data
# directory because the current CLI stores its database as ./database.db.
FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /src/build/unidb /usr/local/bin/unidb

WORKDIR /data
VOLUME ["/data"]

ENTRYPOINT ["/usr/local/bin/unidb"]
