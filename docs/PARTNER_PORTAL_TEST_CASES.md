# Customer and Partner Portal Test Catalogue

## Automated foundation

- `partner_access_policy_test`: roles, unknown-role denial, self-approval denial,
  transitions, price, name and coordinate bounds.
- `partner_repository_test`: owner isolation, cross-tenant denial, optimistic
  conflicts, menu ownership, submission, customer visibility and audit scope.
- `portal_contract_test.py`: gateway-backed login/resources, no private-port
  calls/browser draft authority, and no partner self-approval.
- `partner_api_e2e_test.py`: new-account empty arrays, anonymous denial, two-user
  isolation, private draft/menu, PENDING_REVIEW and audit.
- Existing customer tests: account security, delivery quote, payment/order state,
  HTTP failures and correlation IDs.

Run:

```bash
cmake --build build-wsl -j2
ctest --test-dir build-wsl --output-on-failure
python3 -m unittest tests.portal_contract_test
python3 tests/partner_api_e2e_test.py  # local stack required
```

Verified 2026-09-01: CTest 7/7, portal contracts 3/3, and live partner API passed.
The live test mutates local data; use disposable development/test databases.

## Partner cases

| Area | Positive | Negative and corner cases |
| --- | --- | --- |
| Authentication | Valid active JWT | missing, malformed, expired, wrong issuer/audience, revoked token version, brute force |
| Ownership | Owner accesses own restaurant | cross-restaurant enumeration, revoked member, resource alias changed after authorization |
| Roles | Owner/manager/staff allowed actions | staff changes team/legal data, manager grants owner, self-approval |
| Restaurant | Create/update/submit valid draft | blank/control chars, huge payload, invalid coordinates, stale version, invalid state |
| Menu | Item CRUD with integer paise | negative/overflow price, invalid diet, foreign item, stale version, concurrent edits |
| Documents | Future clean encrypted upload | MIME spoof, polyglot, malware, bomb, oversized, expired/foreign download |
| Orders | Future paid restaurant feed | unpaid/cancelled/delivered/duplicate/stale/other restaurant/timeout race |
| Team | Future verified invitation once | guessed/expired/reused/email mismatch/last-owner removal/revoke race |
| Audit | Actor/action/resource/request/time | missing denied event, PII/secret leak, tampering, partial commit, retention expiry |
| Availability | Retry/read-only behavior | DB/provider failure, timeout, outbox backlog, retry storm, backup restore |

## Customer regression

Registration/login/logout; per-user profile/address isolation; GPS/manual address;
serviceability; cart/discount; idempotent payment; provider success/failure/cancel;
no driver before payment; delivered order has no cancel/track; stale GPS; output
encoding; accessibility/mobile overflow; only APPROVED restaurants visible.

## Required launch layers

Unit, repository/migration, API contract, integration, browser E2E, security,
reliability, performance, accessibility and device testing are mandatory. Load
evidence must record revision, environment, dataset, command and result; thread
count alone never proves marketplace capacity.
