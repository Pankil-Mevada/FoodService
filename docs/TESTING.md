# Testing strategy

## Correlation-ID test

`correlation_id_test` verifies accepted characters, the 64-character limit,
header-injection rejection, preservation of a safe client ID, and unique valid
fallback generation. For a visible manual check, send
`X-Correlation-ID: manual-123` to a gateway route, inspect the same response
header, and watch the gateway terminal for matching `request.start` and
`request.finish` lines.

For the customer checkout UI, verify that the selected saved address is shown
as a locked summary, **Change** returns to the address book, successful order
creation navigates directly to `payment.html?orderId=...`, and **Orders** opens
the dedicated history dialog. The E2E suite asserts that `POST /orders` returns
the ID of the order that was persisted.

## Continuous integration

Every pull request to `develop` or `main` runs the GitHub Actions workflow in
`.github/workflows/ci.yml`. The `quality` job performs fast source checks and
dependency-free C++ tests. The `build-and-e2e` job performs a clean Release
build, CTest, and the HTTP E2E suite against all six running services. CI uses
temporary databases and test-only secrets; it never performs a real payment.

Run the fast Windows preflight from the repository root:

```powershell
.\scripts\ci-local.ps1
```

Run the exact Linux fast check, or its full build variant, from WSL:

```bash
bash scripts/ci-local.sh
VCPKG_ROOT="$HOME/vcpkg" bash scripts/ci-local.sh --full
```

See [Git toolchain and automated pipeline](GIT_PIPELINE.md) for the complete
branch workflow, required checks, GitHub settings, and log troubleshooting.

### Gateway status and transport mapping

`tests/http_result_test.cpp` verifies that normal downstream statuses are
preserved, timeouts map to `504`, connection/other failures map to `502`, and a
response without either a valid status or failure is treated as bad gateway.
It is registered with CTest and compiled by the fast Linux CI script.

```bash
g++ -std=c++20 -Wall -Wextra -Werror \
  -Iservices/ApiGateway/include tests/http_result_test.cpp \
  -o /tmp/http_result_test
/tmp/http_result_test
```

The full CTest suite also runs it as `http_result`.

## Razorpay sandbox checkout

Configure `rzp_test_...` credentials using `docs/SETUP.md`, restart Payment
Service and Gateway, and hard-refresh the frontend. Create an order, click
**Pay now**, then **Pay securely**. Complete Razorpay's mock success flow and
confirm the UI shows `succeeded` only after server-side signature verification.
Exercise the mock failure flow and confirm it is not marked successful. No real
money moves in Test Mode. Automated E2E tests retain the deterministic dummy
provider, so hosted Razorpay Checkout is intentionally a manual test.

The complete positive and negative matrix is in
`docs/PAYMENT_DELIVERY_TEST_CASES.md`. Safe runtime logs are written to
`.run/gateway.log` and `.run/payments.log`; keys, signatures, card data, UPI
PINs, and banking credentials must never be logged.

## Reading structured C++ flow logs

Application messages use stable prefixes:

- `[order-flow]`: order validation, persistence, and pending-payment creation.
- `[payment-flow]`: idempotent creation and allowed/rejected status changes.
- `[razorpay]`: provider order creation and signature-verification outcome.
- `[delivery-gate]`: Order Service's paid-before-delivery decision.
- Crow `Tracking ...`: Gateway tracking rejection, acceptance, and progress.

A successful test payment and first driver assignment includes lines similar to:

```text
[payment-flow] create accepted ... status=pending provider=test
[razorpay] create-order accepted providerOrderId=order_...
[razorpay] signature-verification passed orderId=order_...
[payment-flow] transition accepted ... to=succeeded
[delivery-gate] ... paymentStatus=succeeded decision=allow
Tracking snapshot ... status=ASSIGNED progress=5
```

Expected failure examples include `credentials missing`, `provider order
mismatch`, `signature-verification failed`, `payment-not-succeeded`, and
`transition rejected`. These messages intentionally omit request secrets,
signatures, card/UPI details, OTPs, JWTs, and passwords.

Updated 2026-08-15. All provider, payment, restaurant, and courier behavior in
this test plan is local/dummy behavior unless a section explicitly says otherwise.

## Automated local acceptance test

Run `python tests/e2e_test.py`. The harness reports each capability separately,
continues after failures, and exits nonzero if any test fails. It uses randomized
`.test` email and restaurant values so repeated runs do not collide.

The end-to-end scenario is:

```text
register -> login -> verify JWT identity -> create geocoded restaurant
         -> reject out-of-zone order -> create serviceable order/payment
         -> verify driver assignment/delivery -> verify payment/notification
```

Polling verifies real-time/eventual behavior without assuming that provider
webhooks complete during the original HTTP request. Defaults are a 15-second
eventual timeout and 500-ms interval.

The suite also sends the same test payment twice with one `Idempotency-Key`,
checks order-based payment lookup, reads the finite SSE-compatible snapshot, and
submits an unsigned/unknown webhook that must be rejected without mutation.
It now verifies `succeeded -> CONFIRMED`, `failed -> PAYMENT_FAILED`, and
`cancelled -> CANCELLED`, repeats a succeeded callback to prove idempotency, and
proves that an invalid internal order-sync secret is rejected.
It resolves `GET /me`, rejects anonymous order creation, and submits a forged
`userId` to prove the gateway ignores client identity and uses the JWT claim.
The suite also reads the resulting order list with that JWT; anonymous order
listing is rejected and the Gateway returns only that customer's rows.
The location scenario creates a geocoded restaurant, places an order at a
serviceable delivery point, persists its address/coordinates, and verifies the
real GPS ingestion response, rejects an invalid driver token, verifies that no
position is invented before a fix, and completes delivery at customer coordinates.
It verifies driver/contact/vehicle details, then uses the internal local test
endpoint to advance the order without waiting three minutes and proves that
`DELIVERED`, 100% progress, and zero ETA are persisted and returned.

Live provider discovery is intentionally a manual test so the automated suite
does not repeatedly consume a public service. Click **Bengaluru demo**, wait for
the provider status to finish, and verify that nearby OpenStreetMap restaurants
show distance and **Order here**, while Ahmedabad fixtures show **Outside area**.

Latest verified local result on 2026-08-15: **21 passed, 0 failed, 0 skipped**.
The WSL C++ build, JavaScript syntax check, Python compilation, and
`git diff --check` also passed.

## Focused payment/order transition test

The production transition policy is dependency-free and is tested directly:

```bash
g++ -std=c++20 -Wall -Wextra -Werror \
  -I services/OrderService/include \
  tests/payment_order_status_test.cpp \
  -o /tmp/payment_order_status_test
/tmp/payment_order_status_test
```

The test covers processing, success, failure, cancellation, duplicate callbacks,
non-regression after confirmation/delivery, and invalid transitions. Latest
verified result on 2026-08-24: **payment/order transition tests passed**.

The full service E2E suite was expanded for the same flow, but must be run after
regenerating the machine-specific CMake/vcpkg build. The checked-in `build/`
cache references the removed `/workspaces/FoodService` environment and is not
valid evidence for the current source tree.

## Manual checks

- Invalid JSON and missing required fields return 4xx rather than crashing.
- Duplicate email and duplicate restaurant rules are enforced.
- Missing/invalid/expired JWT cannot access protected user routes.
- Unknown IDs return 404.
- Invalid restaurant prevents order/payment creation.
- Duplicate webhook delivery does not duplicate a payment or notification.
- Invalid webhook signature is rejected and does not mutate state.
- Provider timeout leaves a recoverable pending/failed payment.
- Amount and order identity cannot be overridden by client input.
- Restarting services preserves SQLite state.
- Provider timeout leaves existing database restaurants available and shows a recoverable error.
- Repeated nearby discovery does not create duplicate restaurant rows.
- Restaurant `imageUrl` persists through create/list; invalid or missing UI images show a neutral fallback.
- Test-menu checkout persists item summary, subtotal, discount, delivery fee, and matching final total.
- Profile photo is resized to 256×256, favourites survive refresh, and invalid test UPI IDs are rejected; no UPI PIN is requested.
- Craving shortcuts preselect the matching menu item; rating and favourite filters update the restaurant grid.
- Tracking starts assigned at three minutes and persists delivered at zero ETA.
- Dummy credentials only: plaintext password storage/logging is open security debt.

## Scope and limitations

The harness is intentionally black-box and mutates local data. Cleanup is best
effort because the APIs do not expose transactional fixtures. Run it against a
disposable local/test database, never production. It does not call a live
payment network, validate provider dashboards, load-test concurrency, or prove
PCI compliance.

The automated suite does not call OpenStreetMap, wait three wall-clock minutes,
grant GPS permission, or validate real restaurant/driver dispatch. It advances
delivery through the internal localhost-only test route.

The restaurant acceptance scenario now verifies `imageUrl` persistence. Browser
photo rendering still requires a rebuilt Restaurant Service and API Gateway;
the legacy `build/` cache is machine-specific and must be regenerated when its
recorded source or vcpkg paths do not exist.
# Delivery address and serviceability tests

The dependency-free `tests/delivery_quote_test.cpp` covers near/far radius decisions, polygon inclusion/exclusion, rain pricing, and ETA. UI/API testing should also verify: address JWT isolation; invalid phone/coordinates; selecting/deleting another user's address returns no data/change; GPS denial leaves manual/map entry available; Nominatim failure leaves manual/GPS available; an outside-zone quote and order both return 422; browser fee tampering is replaced by the server quote; and surge/rain/late-night flags appear in the quote.

For local rain/surge checks, start API Gateway with `DELIVERY_RAIN_MODE=1` and/or `DELIVERY_SURGE_MODE=1`. These flags are development controls, not real weather or demand detection.
