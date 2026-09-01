# Service-owned partner authorization verification — 2026-09-01

Implemented from `develop` base `852e6f9`.

## Implemented evidence

- Partner HTTP boundary verifies bearer JWT in Restaurant Service.
- ACTIVE membership and role scope every implemented restaurant/menu/audit read
  and write.
- Gateway customer restaurant endpoints are read-only.
- Draft/rejected/pending restaurants are hidden from customer reads.
- Restaurant creation atomically adds OWNER membership and audit.
- Other partner writes atomically commit their success audit or roll back.
- Optimistic versions reject stale writes.
- Partner routes cannot write APPROVED.
- Two-user live test proves owner visibility, other-user empty list/404, private
  submission, menu persistence and audit visibility.

## Verification commands

```bash
cmake --build build-wsl -j2
ctest --test-dir build-wsl --output-on-failure
python3 -m unittest tests.portal_contract_test
python3 tests/partner_api_e2e_test.py
```

Result: CTest 7/7; portal 3/3; live partner API passed.

## Finding status

The original missing-ownership finding is remediated for the implemented
partner routes. Full production closure still requires email/session security,
rate limiting, independent admin authorization, compliance documents, durable
idempotency/outbox, denied-event audit, private network policy, versioned
migrations/backups, and penetration/load/accessibility evidence.
