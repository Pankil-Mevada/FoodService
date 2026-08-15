# FoodService test harness

`e2e_test.py` is a dependency-free, black-box test application for the local
microservices. It checks health, registration/login/JWT protection, users,
restaurants, order creation, the resulting payment, and the resulting
notification. Payment completion is polled to accommodate asynchronous
provider/webhook processing.

## Run

Start all six executables, then from the repository root run:

```powershell
python tests/e2e_test.py
```

Use `python tests/e2e_test.py --help` to override URLs and timing. The defaults
are ports 8080 through 8085. Test records are deleted on a best-effort basis;
pass `--keep-data` while debugging.

## Test a payment manually

From the repository root in PowerShell, simulate a successful provider callback
for an existing order:

```powershell
.\scripts\test-dummy-payment.ps1 -OrderId 3
```

Use `-Status processing`, `-Status failed`, or `-Status cancelled` to exercise
other allowed payment states. A terminal state cannot transition again.

The default local webhook secret is `test-webhook-secret`; the harness uses it
to drive one test payment from pending through processing to succeeded. Override
it with `--webhook-secret` when the service uses a different local test secret,
or pass an empty value to skip the authenticated transition test.

## Payment safety

The harness creates an order for `12.34` and expects the backend's configured
test/mock provider to handle it. It does not collect or transmit card data and
must only be run with the backend in payment test mode. Never put API keys in
this repository or command output.
