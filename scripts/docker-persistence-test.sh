#!/usr/bin/env bash
set -euo pipefail

RUN_ID="${GITHUB_RUN_ID:-local}-${GITHUB_RUN_ATTEMPT:-1}-$$"
IMAGE="unidb:persistence-${RUN_ID}"
CONTAINER="unidb-persistence-test-${RUN_ID}"
VOLUME="unidb-persistence-test-data-${RUN_ID}"
NETWORK="unidb-persistence-test-net-${RUN_ID}"
TLS_DIR=""
PORT_OVERRIDE="${UNIDB_DOCKER_TEST_PORT:-}"
PORT=""
if [[ -n "$PORT_OVERRIDE" ]]; then
  if [[ ! "$PORT_OVERRIDE" =~ ^[0-9]{1,5}$ ]] ||
     (( 10#$PORT_OVERRIDE < 1 || 10#$PORT_OVERRIDE > 65535 )); then
    echo "UNIDB_DOCKER_TEST_PORT must be a TCP port from 1 through 65535." >&2
    exit 2
  fi
fi
TLS_DIR="$(mktemp -d "${TMPDIR:-/tmp}/unidb-persistence-tls.XXXXXX")"
IMAGE_CREATED=0
VOLUME_CREATED=0
CONTAINER_CREATED=0

cleanup() {
  if [[ "$CONTAINER_CREATED" == "1" ]]; then
    docker rm -f "$CONTAINER" >/dev/null 2>&1 || true
  fi
  if [[ "$VOLUME_CREATED" == "1" ]]; then
    docker volume rm "$VOLUME" >/dev/null 2>&1 || true
  fi
  docker network rm "$NETWORK" >/dev/null 2>&1 || true
  rm -rf "$TLS_DIR"
  if [[ "$IMAGE_CREATED" == "1" ]]; then
    docker image rm "$IMAGE" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

# Generate disposable mutual-TLS credentials for the persistence integration test.
cat > "$TLS_DIR/ca.cnf" <<'EOF'
[ req ]
prompt = no
distinguished_name = dn
x509_extensions = v3_ca
[ dn ]
CN = UniDB Persistence Test CA
[ v3_ca ]
basicConstraints = critical, CA:TRUE
keyUsage = critical, keyCertSign, cRLSign
subjectKeyIdentifier = hash
EOF
openssl req -x509 -newkey rsa:2048 -nodes -sha256 -days 2 \
  -config "$TLS_DIR/ca.cnf" -keyout "$TLS_DIR/ca.key" \
  -out "$TLS_DIR/ca.crt" >/dev/null 2>&1

openssl req -new -newkey rsa:2048 -nodes -sha256 -subj "/CN=127.0.0.1" \
  -keyout "$TLS_DIR/server.key" -out "$TLS_DIR/server.csr" >/dev/null 2>&1
cat > "$TLS_DIR/server.ext" <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = IP:127.0.0.1
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
openssl x509 -req -sha256 -days 2 -in "$TLS_DIR/server.csr" \
  -CA "$TLS_DIR/ca.crt" -CAkey "$TLS_DIR/ca.key" -CAcreateserial \
  -out "$TLS_DIR/server.crt" -extfile "$TLS_DIR/server.ext" >/dev/null 2>&1

openssl req -new -newkey rsa:2048 -nodes -sha256 \
  -subj "/CN=unidb-persistence-test-client" \
  -keyout "$TLS_DIR/client.key" -out "$TLS_DIR/client.csr" >/dev/null 2>&1
cat > "$TLS_DIR/client.ext" <<'EOF'
basicConstraints = critical, CA:FALSE
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
EOF
openssl x509 -req -sha256 -days 2 -in "$TLS_DIR/client.csr" \
  -CA "$TLS_DIR/ca.crt" -CAkey "$TLS_DIR/ca.key" -CAcreateserial \
  -out "$TLS_DIR/client.crt" -extfile "$TLS_DIR/client.ext" >/dev/null 2>&1

# The test server runs as UID 10001. This temporary server key is disposable;
# production keys must be readable only by the service account.
chmod 755 "$TLS_DIR"
chmod 644 "$TLS_DIR/ca.crt" "$TLS_DIR/server.crt" "$TLS_DIR/server.key" \
  "$TLS_DIR/client.crt" "$TLS_DIR/client.key"
chmod 600 "$TLS_DIR/ca.key"

docker build -t "$IMAGE" .
IMAGE_CREATED=1
docker network create "$NETWORK" >/dev/null
docker volume create "$VOLUME" >/dev/null
VOLUME_CREATED=1

start_container() {
  local -a publish_args
  if [[ -n "$PORT_OVERRIDE" ]]; then
    PORT="$PORT_OVERRIDE"
    publish_args=(-p "127.0.0.1:${PORT}:7432")
  else
    publish_args=(-p "127.0.0.1::7432")
  fi

  docker create --name "$CONTAINER" --network "$NETWORK" \
    --env UNIDB_TLS_CERT_FILE=/run/unidb-tls/server.crt \
    --env UNIDB_TLS_KEY_FILE=/run/unidb-tls/server.key \
    --env UNIDB_TLS_CA_FILE=/run/unidb-tls/ca.crt \
    "${publish_args[@]}" \
    -v "$VOLUME:/data" \
    -v "$TLS_DIR:/run/unidb-tls:ro" "$IMAGE" >/dev/null
  CONTAINER_CREATED=1
  docker start "$CONTAINER" >/dev/null

  # Docker assigns a free host port when no explicit override was supplied.
  if [[ -z "$PORT_OVERRIDE" ]]; then
    local published_address
    published_address="$(docker port "$CONTAINER" 7432/tcp | head -n 1)"
    PORT="${published_address##*:}"
    if [[ ! "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
      echo "Could not determine Docker's published host port (got: '$published_address')" >&2
      docker logs "$CONTAINER" >&2 || true
      return 1
    fi
  fi
}

host_sql() {
  python3 - "$TLS_DIR" "$PORT" "$1" <<'PY'
import socket
import ssl
import sys
from pathlib import Path

tls_dir = Path(sys.argv[1])
port = int(sys.argv[2])
statement = sys.argv[3]
context = ssl.create_default_context(cafile=str(tls_dir / "ca.crt"))
context.load_cert_chain(
    certfile=str(tls_dir / "client.crt"),
    keyfile=str(tls_dir / "client.key"),
)
with socket.create_connection(("127.0.0.1", port), timeout=5) as raw_socket:
    with context.wrap_socket(raw_socket, server_hostname="127.0.0.1") as connection:
        connection.settimeout(5)
        connection.sendall((statement + "\n").encode("utf-8"))
        response = bytearray()
        while b"\n" not in response:
            chunk = connection.recv(4096)
            if not chunk:
                break
            response.extend(chunk)
        if not response:
            raise RuntimeError("server closed the TLS connection without a SQL response")
        print(bytes(response).split(b"\n", 1)[0].decode("utf-8"))
PY
}

client_sql() {
  local statement="$1"
  local output
  local response
  if ! output="$(printf '%s\n' "$statement" | timeout 15 docker exec -i "$CONTAINER" \
      /usr/local/bin/unidb connect --host 127.0.0.1 --port 7432 \
      --tls-ca-file /run/unidb-tls/ca.crt \
      --tls-cert-file /run/unidb-tls/client.crt \
      --tls-key-file /run/unidb-tls/client.key)"; then
    echo "Built-in client mutual TLS request failed." >&2
    printf '%s\n' "$output" >&2
    docker logs "$CONTAINER" >&2 || true
    return 1
  fi
  response="$(printf '%s\n' "$output" | grep -F '"ok":' | head -n 1 || true)"
  if [[ -z "$response" ]]; then
    echo "Built-in client did not receive a SQL JSON response over mutual TLS." >&2
    printf '%s\n' "$output" >&2
    return 1
  fi
  printf '%s\n' "$response"
}


client_sql_batch() {
  local statements="$1"
  local output
  local responses
  if ! output="$(printf '%s' "$statements" | timeout 120 docker exec -i "$CONTAINER" \
      /usr/local/bin/unidb connect --host 127.0.0.1 --port 7432 \
      --tls-ca-file /run/unidb-tls/ca.crt \
      --tls-cert-file /run/unidb-tls/client.crt \
      --tls-key-file /run/unidb-tls/client.key)"; then
    echo "Built-in client failed while sending a multi-statement TLS batch." >&2
    printf '%s\n' "$output" >&2
    docker logs "$CONTAINER" >&2 || true
    return 1
  fi
  responses="$(printf '%s\n' "$output" | grep -F '"ok":' || true)"
  if [[ -z "$responses" ]]; then
    echo "Built-in client received no SQL responses for the TLS batch." >&2
    printf '%s\n' "$output" >&2
    return 1
  fi
  printf '%s\n' "$responses"
}

wait_for_server() {
  for attempt in $(seq 1 50); do
    if host_sql "SELECT * FROM docker_persistence;" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.1
  done
  echo "UniDB container state at readiness timeout:" >&2
  docker inspect --format 'status={{.State.Status}} exit={{.State.ExitCode}} error={{.State.Error}}' "$CONTAINER" >&2 || true
  docker logs "$CONTAINER" >&2 || true
  echo "Last mutual-TLS client diagnostic:" >&2
  cat "$TLS_DIR/s_client.log" >&2 || true
  echo "UniDB did not become ready through the published host port" >&2
  return 1
}

start_container
wait_for_server

# A client without a trusted certificate must not receive any SQL response.
unauthenticated_result="$(python3 - "$TLS_DIR" "$PORT" <<'PY'
import socket
import ssl
import sys
from pathlib import Path

tls_dir = Path(sys.argv[1])
context = ssl.create_default_context(cafile=str(tls_dir / "ca.crt"))
try:
    raw_socket = socket.create_connection(("127.0.0.1", int(sys.argv[2])), timeout=5)
except OSError as error:
    raise SystemExit(f"Could not reach the published UniDB listener: {error}")
try:
    connection = context.wrap_socket(raw_socket, server_hostname="127.0.0.1")
except ssl.SSLError:
    print("REJECTED_NO_CLIENT_CERTIFICATE")
    raise SystemExit(0)

with connection:
    connection.settimeout(5)
    try:
        connection.sendall(b"CREATE TABLE should_not_exist (id INT);\n")
        response = connection.recv(4096)
    except (ssl.SSLError, OSError):
        print("REJECTED_NO_CLIENT_CERTIFICATE")
        raise SystemExit(0)
    if response:
        print("UNEXPECTED_SQL_RESPONSE:" + response.decode("utf-8", errors="replace"))
        raise SystemExit(3)
    print("REJECTED_NO_CLIENT_CERTIFICATE")
PY
)"
if [[ "$unauthenticated_result" != "REJECTED_NO_CLIENT_CERTIFICATE" ]]; then
  echo "Server did not reject a TLS client without a certificate:" >&2
  echo "$unauthenticated_result" >&2
  exit 1
fi


client_sql "CREATE TABLE docker_persistence (id INT, name TEXT);" | grep -q '"ok":true'
host_sql "INSERT INTO docker_persistence (id, name) VALUES (7, 'Ada');" | grep -q '"affected_rows":1'

# 1.1 An unauthenticated peer that stalls before ClientHello must not block
# another client from completing mutual TLS and executing a query.
python3 - "$TLS_DIR" "$PORT" <<'PY'
import socket
import ssl
import sys
import time
from pathlib import Path

tls_dir = Path(sys.argv[1])
port = int(sys.argv[2])
stalled_peer = socket.create_connection(("127.0.0.1", port), timeout=3)
try:
    # Let the listener accept the stalled socket before the valid connection.
    time.sleep(0.2)
    context = ssl.create_default_context(cafile=str(tls_dir / "ca.crt"))
    context.load_cert_chain(
        certfile=str(tls_dir / "client.crt"),
        keyfile=str(tls_dir / "client.key"),
    )
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw_socket:
        with context.wrap_socket(raw_socket, server_hostname="127.0.0.1") as connection:
            connection.settimeout(3)
            connection.sendall(b"SELECT * FROM docker_persistence;\n")
            response = bytearray()
            while b"\n" not in response:
                chunk = connection.recv(4096)
                if not chunk:
                    break
                response.extend(chunk)
            if b'"rows":[[7,"Ada"]]' not in response:
                raise RuntimeError(
                    "Valid TLS client did not receive the expected query response "
                    "while an unauthenticated peer was stalled: "
                    + bytes(response).decode("utf-8", errors="replace")
                )
finally:
    stalled_peer.close()
PY

# 1.2 More than four unauthenticated handshakes from one address must not
# occupy every pending TLS slot. The limit is per address as well as global.
python3 - "$PORT" "$CONTAINER" <<'PY'
import socket
import subprocess
import sys
import time

port = int(sys.argv[1])
container = sys.argv[2]
stalled_peers = []
try:
    for _ in range(5):
        stalled_peers.append(
            socket.create_connection(("127.0.0.1", port), timeout=3)
        )
        # Give the accept loop time to register each pending handshake.
        time.sleep(0.15)

    time.sleep(0.25)
    logs = subprocess.run(
        ["docker", "logs", container],
        check=True,
        capture_output=True,
        text=True,
        stderr=subprocess.STDOUT,
    ).stdout
    expected = "pending TLS handshake limit for this address reached"
    if expected not in logs:
        raise RuntimeError(
            "The server did not enforce the per-address pending TLS limit."
        )
finally:
    for peer in stalled_peers:
        peer.close()
PY
# Give the accept loop time to remove the closed pending handshakes.
sleep 0.3
host_sql "SELECT * FROM docker_persistence;" | grep -Fq '"rows":[[7,"Ada"]]'

# 1.3 An authenticated but idle client must not block another client from
# executing a query on the same server.
python3 - "$TLS_DIR" "$PORT" <<'PY'
import socket
import ssl
import sys
import time
from pathlib import Path

tls_dir = Path(sys.argv[1])
port = int(sys.argv[2])
context = ssl.create_default_context(cafile=str(tls_dir / "ca.crt"))
context.load_cert_chain(
    certfile=str(tls_dir / "client.crt"),
    keyfile=str(tls_dir / "client.key"),
)
idle_raw_socket = socket.create_connection(("127.0.0.1", port), timeout=3)
idle_connection = context.wrap_socket(
    idle_raw_socket, server_hostname="127.0.0.1"
)
try:
    # Allow the server to enter the idle client's blocking SSL_read.
    time.sleep(0.2)
    with socket.create_connection(("127.0.0.1", port), timeout=3) as raw_socket:
        with context.wrap_socket(raw_socket, server_hostname="127.0.0.1") as connection:
            connection.settimeout(3)
            connection.sendall(b"SELECT * FROM docker_persistence;\n")
            response = bytearray()
            while b"\n" not in response:
                chunk = connection.recv(4096)
                if not chunk:
                    break
                response.extend(chunk)
            if b'"rows":[[7,"Ada"]]' not in response:
                raise RuntimeError(
                    "The query was blocked or returned an unexpected result "
                    "while another authenticated client was idle: "
                    + bytes(response).decode("utf-8", errors="replace")
                )
finally:
    idle_connection.close()
PY

# 1.4 Send more than 64 KiB of piped SQL through one built-in TLS client.
# This exercises nonblocking writes while the client concurrently drains replies.
client_sql "CREATE TABLE tls_large_response (id INT, payload TEXT);" | grep -q '"ok":true'
batch=""
for index in $(seq 1 600); do
  payload="row-${index}-$(printf '%0120d' 0)"
  batch+="INSERT INTO tls_large_response (id, payload) VALUES (${index}, '${payload}');"$'\n'
done
batch_responses="$(client_sql_batch "$batch")"
insert_count="$(grep -Fc '"affected_rows":1' <<<"$batch_responses")"
if [[ "$insert_count" != "600" ]]; then
  echo "Expected 600 successful inserts from one TLS client; got $insert_count." >&2
  exit 1
fi

# 1.5 A response larger than the client read buffer must be fully drained.
large_response="$(client_sql "SELECT * FROM tls_large_response;")"
if (( ${#large_response} <= 4096 )); then
  echo "Expected TLS SELECT response larger than 4 KiB; got ${#large_response} bytes." >&2
  exit 1
fi
if ! grep -Fq 'row-600-' <<<"$large_response"; then
  echo "Large TLS response was truncated before the final row." >&2
  exit 1
fi

# Stop must trigger the server's graceful flush before the container is removed.
if ! docker stop --time 10 "$CONTAINER" >/dev/null; then
  echo "Failed to stop the UniDB persistence-test container cleanly." >&2
  docker logs "$CONTAINER" >&2 || true
  exit 1
fi
exit_code="$(docker inspect --format '{{.State.ExitCode}}' "$CONTAINER")"
if [[ "$exit_code" != "0" ]]; then
  echo "UniDB exited with status $exit_code during shutdown; expected 0." >&2
  docker logs "$CONTAINER" >&2 || true
  exit 1
fi
docker rm "$CONTAINER" >/dev/null
CONTAINER_CREATED=0

start_container
wait_for_server

response="$(host_sql "SELECT * FROM docker_persistence;")"
if ! grep -Fq '"rows":[[7,"Ada"]]' <<<"$response"; then
  echo "Persisted row was not found after container recreation:" >&2
  echo "$response" >&2
  docker logs "$CONTAINER" >&2
  exit 1
fi

echo "Docker named-volume persistence test passed."
