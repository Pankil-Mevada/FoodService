# 1,000 parallel order load test

See [High-concurrency order processing diagrams](CONCURRENCY_DIAGRAMS.md) for
the runtime, code-level, request-sequence, and verification-flow views.

This is a local test-mode capacity check. It does not open Razorpay Checkout,
submit card data, contact a restaurant, assign a real driver, or move money.

## Acceptance rule

The test passes only when successful HTTP responses, persisted orders, payment
rows, and unique payment order IDs all equal the requested count. A duplicate
payment or any network/server error fails the run.

## Run it and view logs

From PowerShell in the repository root:

```powershell
.\scripts\start-all.ps1
wsl bash -lc "cd /mnt/c/Users/Pankil/Documents/ChatGPT/FoodService && python3 tests/load_test.py --orders 1000"
Get-Content .\.run\orders.log -Wait
Get-Content .\.run\payments.log -Wait
Get-Content .\.run\load-1000-console.log
```

The machine-readable summary is `tests/results/load-1000-latest.json`. Service
output is in `.run/gateway.log`, `.run/orders.log`, `.run/payments.log`, and
`.run/notifications.log`. In WSL, use `tail -f .run/orders.log`.

## Latest measured result

The checked-in result submitted 1,000 clients together and recorded 1,000
successful responses, 1,000 persisted orders, 1,000 payment rows, and no
duplicates or errors. This demonstrates a burst on the tested 16-core local
WSL machine; it is not a production SLA.

SQLite still has one writer per service. Sustained production scale should use
PostgreSQL, a queue between order/payment work, multiple replicas, rate
limiting, metrics, and repeated soak testing.

## Capacity changes

- Gateway, Order, and Payment blocking worker pools are 128 threads.
- Restaurant and Notification pools are 64 threads.
- SQLite uses a 30-second busy timeout, WAL, and normal synchronous mode.
- Order INSERT and row-ID retrieval are locked as one operation.
- Payment creation uses an indexed lookup instead of a full-table scan.
- Downstream HTTP calls have bounded connect and total timeouts.
- Gateway INFO access logging is disabled to avoid Crow socket logger failures
  during close bursts; application flow logs remain enabled in each service.
