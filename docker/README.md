# UniDB Docker image (initial packaging groundwork)

> **Scope limitation:** this image currently runs UniDB's interactive SQL CLI.
> It is not yet a PostgreSQL-style background database server and does not
> expose a network protocol for application clients. Server mode and client
> connections remain follow-up work tracked in
> [SIM-62](https://linear.app/simple-things/issue/SIM-62/add-docker-image-for-unidb-server-with-persistent-volume-storage).

## Build

From the repository root:

```bash
docker build -t unidb:dev .
```

## Run with persistent storage

```bash
docker volume create unidb-data
docker run --rm -it -v unidb-data:/data unidb:dev
```

The current executable opens `database.db` relative to its working directory.
The image uses `/data` as its working directory, so mounting the named volume
there keeps the database file outside the container lifecycle.

To stop the interactive CLI, use `quit`. Persistence across container
recreation is expected when the same named volume is reused, but must be
verified with an end-to-end test before this image is considered production
ready.

## Not yet supported

- Background database server mode.
- Network port / application client protocol.
- Docker Hub publishing of a server-ready image.
- Crash recovery guarantees (a Docker volume alone does not provide recovery).
