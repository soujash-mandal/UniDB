# Publishing and pulling UniDB from Docker Hub

The public image is published to `soujash/unidb` by the GitHub Actions
workflow `Publish Docker Image`.

## Pull

For the first release, replace `0.1.0` with the numeric image tag from the
GitHub release. The workflow removes the leading `v` from semantic version
tags, so a GitHub release tagged `v0.1.0` is published as `0.1.0`:

```bash
docker pull soujash/unidb:0.1.0
# Latest stable release:
docker pull soujash/unidb:latest
```

## Local development

The image defaults to development mode. Start it with a localhost-only host port mapping:

```bash
docker run -d --name unidb \\
  -p 127.0.0.1:7432:7432 \\
  -v unidb-data:/data \\
  soujash/unidb:latest
```

Connect to `127.0.0.1:7432` without TLS or client certificates. Do not publish this development-mode port with `-p 7432:7432`: Docker host port binding is outside the container, so UniDB cannot verify that the host mapping is loopback-only.

## Configure publishing

1. Create the public Docker Hub repository `soujash/unidb`.
2. Create a Docker Hub access token with the minimum permissions needed to push.
3. In GitHub, open **Settings → Secrets and variables → Actions** and add:
   - `DOCKERHUB_USERNAME`: your Docker Hub username.
   - `DOCKERHUB_TOKEN`: the access token, not your account password.
4. Publish a GitHub release with a semantic version tag such as `v0.1.0`.

The workflow runs `scripts/docker-persistence-test.sh` first. Publishing
starts only if the Docker build, named-volume persistence, and mutual-TLS
checks pass. It then publishes the release's version tag. For the mutable
`latest` alias, each release run looks up GitHub's current latest stable
(non-prerelease) release and points `latest` at that release's versioned image.
This prevents prereleases and older maintenance releases from moving `latest`
backwards. Updates to the shared alias are serialized; version-tag publishes
can run independently. Both `linux/amd64` and `linux/arm64` are built,
with BuildKit provenance and SBOM metadata.

## Secure run requirements

The server listens on TCP port `7432` and stores its database at `/data/database.db`. Mount a persistent named volume at `/data`. For shared or remote access, explicitly set `UNIDB_SECURITY_MODE=production`; production mode requires mutual TLS. Supply `UNIDB_TLS_CERT_FILE`, `UNIDB_TLS_KEY_FILE`, and `UNIDB_TLS_CA_FILE` and mount the corresponding server certificate, private key, and CA certificate read-only. Do not expose the service to an untrusted network.

The container runs as the unprivileged `unidb` user (UID `10001`). The
mounted TLS directory itself must be searchable (have directory execute
permission) by UID `10001`. The server
certificate, private key, and CA certificate must all be readable by that UID. Keep the private key restricted; do not make
it world-readable. For a dedicated host-side `./tls` directory, one possible
permission setup is:

```bash
# Include this in the docker run command:
# -e UNIDB_SECURITY_MODE=production
sudo chown 10001:10001 ./tls
sudo chmod 0700 ./tls
sudo chown 10001:10001 ./tls/server.key ./tls/server.crt ./tls/ca.crt
sudo chmod 0400 ./tls/server.key
sudo chmod 0444 ./tls/server.crt ./tls/ca.crt
```

Adjust the paths to match the files you mount into the container. The private
key remains owner-readable only, while the public certificate files are
readable by the container user.
