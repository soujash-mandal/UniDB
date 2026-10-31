# UniDB Docker server

The image defaults to local development mode. It needs no certificates or passwords and stores data at `/data/database.db`.

## Local development

Use Docker Engine 28.0.0 or newer. Older versions may expose ports published to host loopback to other machines on the same layer-2 network.

Publish the port on host loopback only:

```bash
docker pull soujash/unidb:latest
docker run -d --name unidb \
  -p 127.0.0.1:7432:7432 \
  -v unidb-data:/data \
  soujash/unidb:latest
```

Connect with the UniDB CLI or Node client at `127.0.0.1:7432`; no TLS flags are needed.

**Important:** Docker forwards host traffic to a non-loopback interface inside the container. The server cannot inspect the host-side port mapping. Do not use `-p 7432:7432` or publish this development-mode port to an untrusted network. Use production mode for shared or remote access.

## Create TLS credentials

For production, use your organization’s certificate authority and a managed secret distribution process. The following commands create a local test CA and separate server/client certificates.

```bash
mkdir -p unidb-tls
umask 077

cat > unidb-tls/ca.cnf <<'EOF'
[ req ]
prompt = no
distinguished_name = dn
x509_extensions = v3_ca
[ dn ]
CN = UniDB Development CA
[ v3_ca ]
basicConstraints = critical, CA:TRUE
keyUsage = critical, keyCertSign, cRLSign
subjectKeyIdentifier = hash
EOF

openssl req -x509 -newkey rsa:3072 -nodes -sha256 -days 3650 \
  -config unidb-tls/ca.cnf \
  -keyout unidb-tls/ca.key -out unidb-tls/ca.crt

openssl req -new -newkey rsa:3072 -nodes -sha256 -subj "/CN=127.0.0.1" \
  -keyout unidb-tls/server.key -out unidb-tls/server.csr
cat > unidb-tls/server.ext <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = IP:127.0.0.1
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
openssl x509 -req -sha256 -days 365 -in unidb-tls/server.csr \
  -CA unidb-tls/ca.crt -CAkey unidb-tls/ca.key -CAcreateserial \
  -out unidb-tls/server.crt -extfile unidb-tls/server.ext

openssl req -new -newkey rsa:3072 -nodes -sha256 \
  -subj "/CN=unidb-client" \
  -keyout unidb-tls/client.key -out unidb-tls/client.csr
cat > unidb-tls/client.ext <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
openssl x509 -req -sha256 -days 365 -in unidb-tls/client.csr \
  -CA unidb-tls/ca.crt -CAkey unidb-tls/ca.key -CAcreateserial \
  -out unidb-tls/client.crt -extfile unidb-tls/client.ext

# Certificates are public; the container needs to read the CA and server cert.
chmod 644 unidb-tls/ca.crt unidb-tls/server.crt unidb-tls/client.crt
# The container runs as UID 10001. Keep the server private key readable only
# by that UID, and keep CA/client private keys restricted to their owner.
sudo chown 10001:10001 unidb-tls/server.key
sudo chmod 600 unidb-tls/server.key
```

## Build and run

Use Docker Engine 28.0.0 or newer. Older Engine releases may expose ports
published to host loopback to other machines on the same layer-2 network.
Upgrade or apply host firewall rules that block this access before running
UniDB.

Create a dedicated network and do not attach untrusted containers to it. The
example publishes the SQL port only on host loopback and configures server-side
mutual TLS credentials:

```bash
docker build -t unidb:local .
docker network create unidb-net
docker volume create unidb-data
docker run -d --name unidb --network unidb-net \
  -e UNIDB_SECURITY_MODE=production \
  -e UNIDB_TLS_CERT_FILE=/run/unidb-tls/server.crt \
  -e UNIDB_TLS_KEY_FILE=/run/unidb-tls/server.key \
  -e UNIDB_TLS_CA_FILE=/run/unidb-tls/ca.crt \
  -p 127.0.0.1:7432:7432 \
  -v unidb-data:/data \
  -v "$PWD/unidb-tls/server.crt:/run/unidb-tls/server.crt:ro" \
  -v "$PWD/unidb-tls/server.key:/run/unidb-tls/server.key:ro" \
  -v "$PWD/unidb-tls/ca.crt:/run/unidb-tls/ca.crt:ro" \
  unidb:local
```

Connect from a local UniDB build using the client certificate and private key:

```bash
./build/unidb connect --host 127.0.0.1 --port 7432 \
  --tls-ca-file unidb-tls/ca.crt \
  --tls-cert-file unidb-tls/client.crt \
  --tls-key-file unidb-tls/client.key
```

Production mode requires all three server environment variables: `UNIDB_TLS_CERT_FILE`, `UNIDB_TLS_KEY_FILE`, and `UNIDB_TLS_CA_FILE`. The server presents its certificate and verifies a client certificate issued by the configured CA. The client verifies the server certificate against its CA file and destination IPv4 address. TLS 1.2 or newer is required. Do not share private keys, commit them, or copy them into the Docker build context; `.dockerignore` excludes the local `unidb-tls/` directory. Development mode disables TLS and client authentication and must remain restricted to a localhost-only host port mapping.

The server progresses pending TLS handshakes without blocking new accepts. It
allows up to 32 pending handshakes globally and up to 4 per source IPv4 address;
each handshake has a 5-second deadline. Authenticated sessions run independently
(up to 32 concurrent sessions), while SQL actions are serialized before they
access the shared database services. An idle authenticated client therefore
does not block other clients from issuing queries.

## Verify persistence across container recreation

```bash
bash scripts/docker-persistence-test.sh
```

The script creates temporary mutual-TLS credentials, builds the image, creates
a named volume, creates a table and row, gracefully stops and removes the
container, then starts a fresh container with the same volume and verifies the
row. It also verifies that a client without a certificate cannot execute SQL.
Temporary resources and credentials are removed on exit. Requires Docker
Engine, OpenSSL, and a working Docker daemon.

For a manual restart, stop and remove the container but keep the volume. Pass
the same TLS settings and mounts again when creating the replacement container:

```bash
docker stop unidb
docker rm unidb
docker run -d --name unidb --network unidb-net \
  -e UNIDB_TLS_CERT_FILE=/run/unidb-tls/server.crt \
  -e UNIDB_TLS_KEY_FILE=/run/unidb-tls/server.key \
  -e UNIDB_TLS_CA_FILE=/run/unidb-tls/ca.crt \
  -p 127.0.0.1:7432:7432 \
  -v unidb-data:/data \
  -v "$PWD/unidb-tls/server.crt:/run/unidb-tls/server.crt:ro" \
  -v "$PWD/unidb-tls/server.key:/run/unidb-tls/server.key:ro" \
  -v "$PWD/unidb-tls/ca.crt:/run/unidb-tls/ca.crt:ro" \
  unidb:local
```

Do not run `docker volume rm unidb-data` if you need to retain the database.
Keep the replacement container on the same dedicated network.

## Platform and durability notes

The image is built on Debian Bookworm and is intended for Docker-supported
Linux environments and compatible Docker Desktop installations. Docker named
volumes preserve files across container replacement; they do not provide
transactional crash recovery. Graceful shutdown flushes dirty pages, but abrupt
power loss or forced termination is not equivalent to a database recovery
protocol.

Docker Hub publishing is intentionally not part of the verification script.
Publish only after the image build and mutual-TLS named-volume persistence test
have passed on the target release environment.
