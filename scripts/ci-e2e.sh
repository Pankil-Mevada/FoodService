#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"
build_dir=$(realpath "${1:-build-ci}")
run_dir=$(mktemp -d)
pids=()

cleanup() {
  for pid in "${pids[@]:-}"; do kill "$pid" 2>/dev/null || true; done
  wait 2>/dev/null || true
  rm -rf "$run_dir"
}
trap cleanup EXIT

start_service() {
  local name=$1 executable=$2
  (cd "$run_dir" && "$executable") >"$run_dir/$name.log" 2>&1 &
  pids+=("$!")
}

export FOODSERVICE_DB_DIR="$run_dir"
export PAYMENT_SERVICE_DB="$run_dir/payment.db"

start_service users "$build_dir/services/UserService/UserService"
start_service restaurants "$build_dir/services/RestaurantService/RestaurantService"
start_service notifications "$build_dir/services/NotificationService/NotificationService"
start_service payments "$build_dir/services/PaymentService/PaymentService"
start_service orders "$build_dir/services/OrderService/OrderService"
start_service gateway "$build_dir/services/ApiGateway/ApiGateway"

health_urls=(
  http://127.0.0.1:8080/health http://127.0.0.1:8081/health
  http://127.0.0.1:8082/health http://127.0.0.1:8083/health
  http://127.0.0.1:8084/health http://127.0.0.1:8085/health
)

for attempt in {1..40}; do
  ready=true
  for url in "${health_urls[@]}"; do curl --fail --silent --max-time 1 "$url" >/dev/null || ready=false; done
  if [[ "$ready" == true ]]; then break; fi
  if [[ "$attempt" == 40 ]]; then
    echo "Services failed to become healthy" >&2
    for log in "$run_dir"/*.log; do echo "===== $log ====="; tail -n 100 "$log"; done
    exit 1
  fi
  sleep 0.5
done

python3 tests/e2e_test.py \
  --webhook-secret "${PAYMENT_WEBHOOK_SECRET:-test-webhook-secret}" \
  --driver-token "${DRIVER_LOCATION_TOKEN:-ci-only-driver-location-token}"
