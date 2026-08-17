#!/usr/bin/env bash
set -eu
cd /mnt/c/Users/Pankil/Documents/ChatGPT/FoodService
for service_name in UserService RestaurantService OrderService PaymentService NotificationService ApiGateway; do
  pkill -f "build-wsl/services/$service_name/$service_name$" 2>/dev/null || true
done
for pidfile in .run/*.pid; do
  [ -e "$pidfile" ] || continue
  pid=$(head -n 1 "$pidfile" | tr -d '\r')
  [[ "$pid" =~ ^[0-9]+$ ]] || continue
  kill "$pid" 2>/dev/null || true
done
