# Customer and Partner Portal Test Catalogue

## Automated unit foundation

- Partner access policy test: roles, unknown-role denial, approval bypass denial, transitions, price, name and coordinate bounds.
- Existing customer tests: account security, delivery quote, payment/order state, HTTP failures and correlation IDs.

## Partner cases

| Area | Positive | Negative and corner cases |
| --- | --- | --- |
| Authentication | Valid active JWT | Missing, malformed, expired, wrong issuer/audience, revoked, clock skew, replay |
| Ownership | Owner updates own restaurant | Cross-restaurant enumeration, revoked member, suspended partner, resource alias changed after authorization |
| Roles | Manager edits menu; staff availability | Staff changes legal/bank/team data, manager grants owner, self-approval |
| Restaurant | Save and submit valid draft | blank, Unicode/control chars, huge payload, invalid polygon, stale version, reused idempotency key with changed body |
| Menu | Item/variant/options CRUD | negative/overflow/NaN price, duplicate SKU, circular options, concurrent edits, 10k-item import |
| Hours | Overnight and holiday | DST, timezone change, overlaps, closed forever, exact boundary |
| Documents | Valid clean upload | MIME spoof, polyglot, malware, decompression bomb, oversized, expired URL, foreign download |
| Orders | Paid order lifecycle | unpaid, cancelled, delivered, duplicate, stale, other restaurant, timeout race, duplicate/out-of-order event |
| Team | Verified invitation accepted once | guessed, expired, reused, email mismatch, last-owner removal, revoke race |
| Audit | Actor/action/result/request ID | secret/PII leak, missing denied event, tampering, retention expiry |
| Availability | Safe retry/read-only mode | DB/bus/provider failure, timeout, partial commit, outbox backlog, retry storm |

## Customer regression

Registration/login/logout; per-user photo/address isolation; GPS/manual address; serviceability; cart and discounts; idempotent order; payment success/failure/cancel/timeout/webhook replay; no driver before payment; delivered order has no cancel/track; stale GPS; output encoding; accessibility and mobile overflow.

## Required layers before launch

Unit, repository/migration, API contract, integration, end-to-end, security, reliability, performance, accessibility and browser/device testing are mandatory. Load evidence must record revision, environment, dataset, commands and results; thread count alone never proves 1,000-order capacity.
