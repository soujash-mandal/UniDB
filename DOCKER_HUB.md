# Publishing and pulling UniDB from Docker Hub

The public image is published to `soujash/unidb` by the GitHub Actions
workflow `Publish Docker Image`.

## Pull

For the first release, replace `0.1.0` with the version shown in the GitHub
release:

```bash
docker pull soujash/unidb:0.1.0
# Latest non-prerelease release:
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
checks pass. It then publishes the version tag and, for non-prerelease
releases, `latest`. Both `linux/amd64` and `linux/arm64` are built, with
BuildKit provenance and SBOM metadata.

## Secure run requirements

The server listens on TCP port `7432` and stores its database at
`/data/database.db`. Mount a persistent named volume at `/data`. The
container binds to `0.0.0.0`, so **all non-loopback connections require
mutual TLS**. Supply `UNIDB_TLS_CERT_FILE`, `UNIDB_TLS_KEY_FILE`, and
`UNIDB_TLS_CA_FILE` and mount the corresponding server certificate, private
key, and CA certificate read-only. Prefer publishing the host port on
`127.0.0.1:7432:7432`; do not expose it to an untrusted network.
