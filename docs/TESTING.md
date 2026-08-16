# Testing strategy

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
It resolves `GET /me`, rejects anonymous order creation, and submits a forged
`userId` to prove the gateway ignores client identity and uses the JWT claim.
The location scenario creates a geocoded restaurant, places an order at a
serviceable delivery point, persists its address/coordinates, and verifies the
explicitly simulated driver-location and ETA response.
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
