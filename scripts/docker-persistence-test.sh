#!/usr/bin/env bash
set -euo pipefail

IMAGE="${UNIDB_DOCKER_IMAGE:-unidb:local}"
CONTAINER="unidb-persistence-test"
VOLUME="unidb-persistence-test-data"
PORT="${UNIDB_DOCKER_TEST_PORT:-17432}"

cleanup() {
  docker rm -f "$CONTAINER" >/dev/null 2>&1 || true
  docker volume rm "$VOLUME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker build -t "$IMAGE" .
docker volume create "$VOLUME" >/dev/null
docker run -d --name "$CONTAINER" -p "127.0.0.1:${PORT}:7432" \
  -v "$VOLUME:/data" "$IMAGE" >/dev/null

wait_for_server() {
  for attempt in $(seq 1 50); do
    if docker exec "$CONTAINER" /usr/local/bin/unidb connect --port 7432 </dev/null >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.1
  done
  docker logs "$CONTAINER"
  echo "UniDB did not become ready" >&2
  return 1
}

sql() {
  printf '%s\n' "$1" | docker exec -i "$CONTAINER" /usr/local/bin/unidb connect --port 7432
}

wait_for_server
sql "CREATE TABLE docker_persistence (id INT, name TEXT);" | grep -q '"ok":true'
sql "INSERT INTO docker_persistence (id, name) VALUES (7, 'Ada');" | grep -q '"affected_rows":1'

# Stop must trigger the server's graceful flush before the container is removed.
docker stop --time 10 "$CONTAINER" >/dev/null
docker rm "$CONTAINER" >/dev/null

docker run -d --name "$CONTAINER" -p "127.0.0.1:${PORT}:7432" \
  -v "$VOLUME:/data" "$IMAGE" >/dev/null
wait_for_server

response="$(sql "SELECT * FROM docker_persistence;")"
if ! grep -Fq '"rows":[[7,"Ada"]]' <<<"$response"; then
  echo "Persisted row was not found after container recreation:" >&2
  echo "$response" >&2
  docker logs "$CONTAINER" >&2
  exit 1
fi

echo "Docker named-volume persistence test passed."
