#!/usr/bin/env bash
set -euo pipefail

RUN_ID="${GITHUB_RUN_ID:-local}-${GITHUB_RUN_ATTEMPT:-1}-$$"
IMAGE="unidb:persistence-${RUN_ID}"
CONTAINER="unidb-persistence-test-${RUN_ID}"
VOLUME="unidb-persistence-test-data-${RUN_ID}"
NETWORK="unidb-persistence-test-net-${RUN_ID}"
PORT="${UNIDB_DOCKER_TEST_PORT:-17432}"
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
  if [[ "$IMAGE_CREATED" == "1" ]]; then
    docker image rm "$IMAGE" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

docker build -t "$IMAGE" .
IMAGE_CREATED=1
docker network create "$NETWORK" >/dev/null
docker volume create "$VOLUME" >/dev/null
VOLUME_CREATED=1

start_container() {
  docker create --name "$CONTAINER" --network "$NETWORK" \
    -p "127.0.0.1:${PORT}:7432" \
    -v "$VOLUME:/data" "$IMAGE" >/dev/null
  CONTAINER_CREATED=1
  docker start "$CONTAINER" >/dev/null
}

host_sql() {
  local statement="$1"
  local response
  if ! exec 3<>"/dev/tcp/127.0.0.1/${PORT}"; then
    return 1
  fi
  printf '%s\n' "$statement" >&3
  if ! IFS= read -r -t 5 response <&3; then
    exec 3>&-
    return 1
  fi
  exec 3>&-
  printf '%s\n' "$response"
}

wait_for_server() {
  for attempt in $(seq 1 50); do
    if host_sql "SELECT * FROM docker_persistence;" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.1
  done
  docker logs "$CONTAINER"
  echo "UniDB did not become ready through the published host port" >&2
  return 1
}

start_container
wait_for_server
host_sql "CREATE TABLE docker_persistence (id INT, name TEXT);" | grep -q '"ok":true'
host_sql "INSERT INTO docker_persistence (id, name) VALUES (7, 'Ada');" | grep -q '"affected_rows":1'

# Stop must trigger the server's graceful flush before the container is removed.
docker stop --time 10 "$CONTAINER" >/dev/null
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
