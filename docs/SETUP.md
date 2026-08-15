# Setup and run guide

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

## Configuration and secrets

The current backend is a safe provider simulator: it persists a `pending`
payment but makes no external processor call. Configuration variables are
`PAYMENT_SERVICE_PORT` (default 8083), `PAYMENT_DATABASE_PATH` (default
`payment.db`), `PAYMENT_WEBHOOK_SECRET` (local default `test-webhook-secret`),
and `PAYMENT_SERVICE_URL` for Order Service/API Gateway discovery. Set a strong
webhook secret outside local development. Store all secrets in environment
variables, never source files, frontend JavaScript, logs, or Git.

After all health endpoints respond, run `python tests/e2e_test.py`.
