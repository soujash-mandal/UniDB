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
  local statement="$1"
  local output
  local response
  if ! output="$(printf '%s\n' "$statement" | timeout 15 openssl s_client \
      -quiet -no_ign_eof -verify_return_error -verify_ip 127.0.0.1 \
      -CAfile "$TLS_DIR/ca.crt" \
      -cert "$TLS_DIR/client.crt" -key "$TLS_DIR/client.key" \
      -connect "127.0.0.1:$PORT" 2>"$TLS_DIR/s_client.log")"; then
    echo "Mutual TLS SQL request failed." >&2
    cat "$TLS_DIR/s_client.log" >&2 || true
    return 1
  fi
  response="$(printf '%s\n' "$output" | grep -F '"ok":' | head -n 1 || true)"
  if [[ -z "$response" ]]; then
    echo "No SQL JSON response received over mutual TLS." >&2
    printf '%s\n' "$output" >&2
    cat "$TLS_DIR/s_client.log" >&2 || true
    return 1
  fi
  printf '%s\n' "$response"
}


client_sql() {
  local statement="$1"
  local output
  local response
  if ! output="$(printf '%s\n' "$statement" | docker exec -i "$CONTAINER" \
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

# A client without a trusted certificate must never receive an SQL response.
unauthenticated_output="$(printf '%s\n' \
  "CREATE TABLE should_not_exist (id INT);" | timeout 8 openssl s_client \
  -quiet -no_ign_eof -verify_ip 127.0.0.1 -CAfile "$TLS_DIR/ca.crt" \
  -connect "127.0.0.1:$PORT" 2>"$TLS_DIR/unauthenticated.log" || true)"
if grep -Fq '"ok":true' <<<"$unauthenticated_output"; then
  echo "Server executed SQL from a client without a certificate." >&2
  echo "$unauthenticated_output" >&2
  exit 1
fi

client_sql "CREATE TABLE docker_persistence (id INT, name TEXT);" | grep -q '"ok":true'
host_sql "INSERT INTO docker_persistence (id, name) VALUES (7, 'Ada');" | grep -q '"affected_rows":1'

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
