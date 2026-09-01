# Restaurant Partner Portal Architecture

## Implemented foundation

The partner page is an isolated responsive preview. PartnerAccessPolicy owns reusable role, transition and validation rules. It deliberately does not trust browser storage. Current public restaurant CRUD remains legacy and is not suitable for partner writes.

## Target launch architecture

~~~mermaid
flowchart LR
  P[Partner browser] -->|HTTPS, JWT| G[API Gateway]
  C[Customer browser] -->|HTTPS, JWT| G
  G --> I[Identity service]
  G --> R[Restaurant and Partner service]
  G --> O[Order service]
  R --> RDB[(Restaurant DB)]
  O --> ODB[(Order DB)]
  R --> OBJ[(Encrypted document storage)]
  R --> Q[Outbox and event bus]
  Q --> A[(Audit and analytics)]
  ADMIN[Admin portal] -->|separate admin role| G
~~~

The gateway authenticates and rate-limits; services verify the token again or a short-lived signed internal identity context. Restaurant service exclusively owns membership, profile and menu writes. Order service exclusively owns order state. No portal talks to a database or private port.

## Authorization sequence

1. Verify issuer, audience, expiry, signature and token version.
2. Resolve the final restaurant ID.
3. Load active membership for that user and restaurant.
4. Evaluate the central permission policy.
5. Execute write plus audit/outbox event in one transaction.

An owner of Restaurant A receives 404 or 403 for Restaurant B. Partner routes cannot approve, suspend, alter payout identity without dual control, or access payment credentials.

## Generic expansion

- Versioned commands and backward-compatible responses.
- Policy independent of Crow and SQLite.
- Provider interfaces for KYC, object storage, maps, notifications and payouts.
- Versioned outbox events instead of cross-service database access.
- Feature flags and capability grants for chains, franchises, grocery and dine-out.
- Resource IDs on every row and index; no global mutable restaurant selection.

## Data and consistency

Transactional writes use one service-owned SQL database. Unique constraints enforce membership and idempotency. Optimistic version columns prevent lost updates. Outbox publication is at least once; consumers deduplicate by event ID. Search and analytics are eventually consistent and never authorize writes.

## Security

- Argon2 passwords, short-lived JWTs and environment-managed secrets.
- HTTPS, strict CORS allowlist, CSP, secure headers and no secrets in URLs.
- Upload type/size checks, malware scan, encryption and signed download URLs.
- Parameterized SQL, request limits, schema validation and output encoding.
- Account/IP/restaurant rate limits, idempotency and replay detection.
- PII minimization, field encryption, retention, audited access and secret rotation.
- Separate admin duties and dual control for bank/legal changes.

## Migration

Legacy public restaurant writes must become internal or be removed before launch. Existing rows need an owner and approval migration. Use dual-read, backfill, reconcile and then enable enforcement behind a feature flag.
