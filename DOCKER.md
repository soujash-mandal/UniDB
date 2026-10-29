# UniDB Docker server

The image runs the foreground SQL server, stores the database at
`/data/database.db`, and uses a named volume for durable storage. It runs as
an unprivileged user. The default non-container server bind remains
`127.0.0.1`; the image sets `UNIDB_BIND_ADDRESS=0.0.0.0` so Docker port
publishing can reach the listener.

## Build and run

```bash
docker build -t unidb:local .
docker volume create unidb-data
docker run -d --name unidb \
  -p 127.0.0.1:7432:7432 \
  -v unidb-data:/data \
  unidb:local
```

The example publishes the port only on the host's loopback interface. Connect
with `./build/unidb connect --port 7432` from a local build, or use any client
that implements UniDB's newline-delimited SQL/JSON TCP protocol.

UniDB currently has no authentication or transport encryption. Do not publish
port 7432 on a public or untrusted interface. The server handles one client
connection at a time.

## Verify persistence across container recreation

```bash
bash scripts/docker-persistence-test.sh
```

The script builds the image, creates a named volume, creates a table and row,
gracefully stops and removes the container, then starts a fresh container with
the same volume and checks that the row remains. It removes its test container
and volume on exit. Requires Docker Engine and a working Docker daemon.

For a manual restart, stop and remove the container but keep the volume:

```bash
docker stop unidb
docker rm unidb
docker run -d --name unidb \
  -p 127.0.0.1:7432:7432 \
  -v unidb-data:/data \
  unidb:local
```

Do not run `docker volume rm unidb-data` if you need to retain the database.

## Platform and durability notes

The image is built on Debian Bookworm and is intended for Docker-supported
Linux environments and compatible Docker Desktop installations. Docker named
volumes preserve files across container replacement; they do not provide
transactional crash recovery. Graceful shutdown flushes dirty pages, but abrupt
power loss or forced termination is not equivalent to a database recovery
protocol.

Docker Hub publishing is intentionally not part of the verification script.
Publish only after the image build and named-volume persistence test have been
run successfully on the target release environment.
