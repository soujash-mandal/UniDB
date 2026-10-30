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

The server listens on TCP port `7432` and stores its database at
`/data/database.db`. Mount a persistent named volume at `/data`. The
container binds to `0.0.0.0`, so **all non-loopback connections require
mutual TLS**. Supply `UNIDB_TLS_CERT_FILE`, `UNIDB_TLS_KEY_FILE`, and
`UNIDB_TLS_CA_FILE` and mount the corresponding server certificate, private
key, and CA certificate read-only. Prefer publishing the host port on
`127.0.0.1:7432:7432`; do not expose it to an untrusted network.

The container runs as the unprivileged `unidb` user (UID `10001`). The
mounted TLS directory and every parent directory in its host path must be
searchable (have directory execute permission) by UID `10001`. The server
certificate, private key, and CA certificate must all be readable by that UID. Keep the private key restricted; do not make
it world-readable. For a dedicated host-side `./tls` directory, one possible
permission setup is:

```bash
sudo chown 10001:10001 ./tls
sudo chmod 0700 ./tls
sudo chown 10001:10001 ./tls/server.key ./tls/server.crt ./tls/ca.crt
sudo chmod 0400 ./tls/server.key
sudo chmod 0444 ./tls/server.crt ./tls/ca.crt
```

Adjust the paths to match the files you mount into the container. The private
key remains owner-readable only, while the public certificate files are
readable by the container user.
