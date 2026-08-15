# Testing strategy

## Automated local acceptance test

Run `python tests/e2e_test.py`. The harness reports each capability separately,
continues after failures, and exits nonzero if any test fails. It uses randomized
`.test` email and restaurant values so repeated runs do not collide.

The end-to-end scenario is:

```text
register -> login -> validate JWT rejection -> create restaurant
         -> create order -> poll payment terminal state -> verify notification
```

Polling verifies real-time/eventual behavior without assuming that provider
webhooks complete during the original HTTP request. Defaults are a 15-second
eventual timeout and 500-ms interval.

The suite also sends the same test payment twice with one `Idempotency-Key`,
checks order-based payment lookup, reads the finite SSE-compatible snapshot, and
submits an unsigned/unknown webhook that must be rejected without mutation.
It resolves `GET /me`, rejects anonymous order creation, and submits a forged
`userId` to prove the gateway ignores client identity and uses the JWT claim.

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

## Scope and limitations

The harness is intentionally black-box and mutates local data. Cleanup is best
effort because the APIs do not expose transactional fixtures. Run it against a
disposable local/test database, never production. It does not call a live
payment network, validate provider dashboards, load-test concurrency, or prove
PCI compliance.
