# Restaurant Partner Portal Architecture

## Implemented architecture

The partner page signs in through User Service and calls authenticated Gateway
partner routes. Restaurant Service owns profiles, `restaurant_partners`,
`partner_menu_items`, lifecycle/version fields and `partner_audit_events` in
`restaurant.db`. PartnerAccessPolicy owns reusable role/validation rules.
Gateway customer restaurant routes are read-only; customer repository reads
expose only APPROVED restaurants.

~~~mermaid
flowchart LR
  P[Partner browser] -->|HTTP locally; HTTPS in deployment| G[API Gateway :8085]
  C[Customer browser] --> G
  G --> I[User Service :8080]
  G --> R[Restaurant Service :8081]
  G --> O[Order Service :8082]
  I --> UDB[(foodservice.db)]
  R --> RDB[(restaurant.db)]
  O --> ODB[(order.db)]
  RDB --> PROF[restaurants + lifecycle/version]
  RDB --> MEM[restaurant_partners]
  RDB --> MENU[partner_menu_items]
  RDB --> AUDIT[partner_audit_events]
  ODB --> FLOW[restaurant_order_workflows]
  ODB --> CMD[restaurant_order_commands]
  ODB --> OEVENT[restaurant_order_events]
  ADMIN[Future admin portal] -. separate admin role .-> G
~~~

The gateway is the only browser-facing backend. Restaurant Service verifies the
JWT again. For order operations, Gateway asks Restaurant Service to resolve the
ACTIVE membership and role, then calls an internal Order Service route using a
server-only shared secret and derived partner user ID. Order Service always
scopes reads and writes by both restaurant ID and order ID. Production network
policy must make ports 8080–8084 private.

## Authorization sequence

1. Verify JWT signature, issuer, audience, expiry and token version.
2. Derive the user ID from the verified token.
3. Resolve the final restaurant ID and ACTIVE membership in Restaurant DB.
4. Evaluate OWNER/MANAGER/STAFF permission centrally.
5. Execute the write plus success audit in one transaction.

An owner of Restaurant A receives 404 for Restaurant B. Partner routes cannot
approve, suspend, alter payout identity or access payment credentials.

## Data and consistency

Current partner writes use service-owned SQLite. Membership has a composite
primary key and versions prevent lost updates. Restaurant profile/menu writes
and their audits commit together in Restaurant Service. Kitchen workflow,
command idempotency, and order audit commit together in Order Service; a unique
`(restaurant_id, idempotency_key)` key performs backend deduplication. Profile
and menu commands still need durable idempotency. A cross-service outbox/event
bus remains target architecture.

## Generic expansion

- Versioned APIs and migrations with backward-compatible responses.
- Policy independent of Crow and SQLite.
- Provider interfaces for KYC, object storage, maps, notifications and payouts.
- Versioned outbox events rather than cross-service database access.
- Feature flags/capabilities for chains, franchises, grocery and dine-out.
- Resource IDs on every row; no global mutable restaurant selection.

## Security

- Argon2id passwords, one-hour JWTs and environment-managed production secret.
- Configured CORS origin; HTTPS, CSP and secure headers required for deployment.
- Parameterized SQL, field bounds, output encoding and no secrets in logs.
- Customer reads include only APPROVED restaurants.
- Separate admin duties; no partner self-approval.
- Future: account/IP/restaurant rate limits, email verification, invitation
  revocation, KYC object security, denied-event audit and secret rotation.

## Migration and launch work

Existing approved rows need reviewed ownership backfill before production.
SQLite startup alterations must become versioned migrations to managed SQL.
Add reconciliation, backups/restore drills, private service networking, admin
approval, remaining command idempotency/outbox, load/accessibility/security evidence and
operational rollback before an external pilot.
