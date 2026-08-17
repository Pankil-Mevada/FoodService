# Setup and run guide

For low-volume nearby discovery, the gateway defaults to the public Overpass
development endpoint. Set `FOODSERVICE_OVERPASS_URL` before startup to use a
contracted or self-hosted instance.

## Requirements

- CMake 3.16+
- C++20 compiler
- vcpkg
- Python 3.10+ for the E2E harness
- Dependencies from `vcpkg.json`: SQLite, OpenSSL, jwt-cpp, Asio, Argon2

## Configure and build

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug
```

Use a fresh Windows build directory if an existing `build/` was generated on
Linux; CMake caches are not portable between operating systems.

## Run locally

Start executables for User, Restaurant, Order, Payment, Notification, then API
Gateway. They listen on ports 8080, 8081, 8082, 8083, 8084, and 8085. Start the
dependency services before Order and Gateway so cross-service calls succeed.

Database files are opened relative to each process working directory. For
predictable local data placement, launch all executables with the repository
root as the working directory.

### `.run` PID and log files

`scripts/start-all.ps1` creates `.run/` as local runtime state. A `.pid` file
contains one Linux process ID, not application output or customer data. For
example, `.run/payments.pid` identifies the Payment Service process so the
launcher can stop that exact instance on restart. The matching `.log` file
contains stdout/stderr:

| Process | PID file | Log file |
|---|---|---|
| API Gateway | `.run/gateway.pid` | `.run/gateway.log` |
| Payment Service | `.run/payments.pid` | `.run/payments.log` |
| Order Service | `.run/orders.pid` | `.run/orders.log` |
| Other services/frontend | same base name | same base name |

Watch a flow from separate PowerShell windows:

```powershell
Get-Content .run\gateway.log -Wait
Get-Content .run\payments.log -Wait
Get-Content .run\orders.log -Wait
```

The directory is ignored by Git. PID files can become stale after a crash;
the launcher also finds exact FoodService executable names before restart.

## Razorpay Test Mode and secrets

The dummy provider remains the fallback when Razorpay variables are absent.
For Razorpay's real sandbox, set Test Mode keys only in the terminal that
launches Payment Service:

```powershell
$env:RAZORPAY_KEY_ID = "rzp_test_..."
$env:RAZORPAY_KEY_SECRET = "..."
$env:RAZORPAY_WEBHOOK_SECRET = "a-separate-random-test-secret"
$env:WSLENV = "RAZORPAY_KEY_ID/u:RAZORPAY_KEY_SECRET/u:RAZORPAY_WEBHOOK_SECRET/u"
```

`WSLENV` passes those named variables to a subsequently launched WSL process.
Restart Payment Service and API Gateway, then hard-refresh the frontend. The
public Test Key ID may reach the browser; the Key Secret must stay exclusively
in Payment Service and must never be committed, logged, or pasted into chat.

Other payment configuration variables are
`PAYMENT_SERVICE_PORT` (default 8083), `PAYMENT_DATABASE_PATH` (default
`payment.db`), `PAYMENT_WEBHOOK_SECRET` (local default `test-webhook-secret`),
and `PAYMENT_SERVICE_URL` for Order Service/API Gateway discovery. Set a strong
webhook secret outside local development. Store all secrets in environment
variables, never source files, frontend JavaScript, logs, or Git.

After all health endpoints respond, run `python tests/e2e_test.py`.
