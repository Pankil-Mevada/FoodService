#!/usr/bin/env bash
set -eu
cd /mnt/c/Users/Pankil/Documents/ChatGPT/FoodService
mkdir -p .run
# A developer restart may be initiated from another terminal. If Test keys are
# not in that shell, inherit only the three named variables from the currently
# running Payment Service before it is stopped. Values are never printed.
if [ -z "${RAZORPAY_KEY_ID:-}" ] || [ -z "${RAZORPAY_KEY_SECRET:-}" ]; then
  current_payment_pid=$(pgrep -f 'build-wsl/services/PaymentService/PaymentService$' | head -n 1 || true)
  if [ -n "$current_payment_pid" ] && [ -r "/proc/$current_payment_pid/environ" ]; then
    while IFS= read -r -d '' entry; do
      case "$entry" in
        RAZORPAY_KEY_ID=*|RAZORPAY_KEY_SECRET=*|RAZORPAY_WEBHOOK_SECRET=*) export "$entry" ;;
      esac
    done < "/proc/$current_payment_pid/environ"
  fi
fi
# Stop older FoodService instances that may predate the PID files. Process
# names are exact, so unrelated WSL applications are not touched.
for service_name in UserService RestaurantService OrderService PaymentService NotificationService ApiGateway; do
  pkill -f "build-wsl/services/$service_name/$service_name$" 2>/dev/null || true
done
for pidfile in .run/*.pid; do
  [ -e "$pidfile" ] || continue
  pid=$(head -n 1 "$pidfile" | tr -d '\r')
  [[ "$pid" =~ ^[0-9]+$ ]] || continue
  kill "$pid" 2>/dev/null || true
done
sleep 1
start_service() {
  local name="$1"
  local executable="$2"
  nohup "$executable" > ".run/$name.log" 2>&1 &
  echo $! > ".run/$name.pid"
}
start_service users ./build-wsl/services/UserService/UserService
start_service restaurants ./build-wsl/services/RestaurantService/RestaurantService
start_service notifications ./build-wsl/services/NotificationService/NotificationService
sleep 1
start_service payments ./build-wsl/services/PaymentService/PaymentService
start_service orders ./build-wsl/services/OrderService/OrderService
sleep 1
start_service gateway ./build-wsl/services/ApiGateway/ApiGateway
nohup python3 -m http.server 5173 --directory frontend > .run/frontend.log 2>&1 &
echo $! > .run/frontend.pid
sleep 2
failed=0
for endpoint in http://127.0.0.1:8080/health http://127.0.0.1:8081/health http://127.0.0.1:8082/health http://127.0.0.1:8083/health http://127.0.0.1:8084/health http://127.0.0.1:8085/health http://127.0.0.1:5173/; do
  if curl -fsS --max-time 3 "$endpoint" >/dev/null; then
    printf 'ONLINE  %s\n' "$endpoint"
  else
    printf 'FAILED  %s\n' "$endpoint"
    failed=1
  fi
done
exit "$failed"
