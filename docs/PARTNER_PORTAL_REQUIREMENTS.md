# Restaurant Partner Portal — Product Requirements

Status: launch foundation, 2026-09-01.

## Product outcome

Restaurant owners and authorized staff can onboard a business, maintain serviceability, hours and menu data, operate paid orders, and inspect an immutable activity history. Customer and partner experiences share identity but never share authorization.

## Release scope and status

| Capability | Status | Acceptance criteria / remaining work |
| --- | --- | --- |
| Responsive partner workspace | ✅ Preview | Separate partner page; keyboard-friendly navigation and mobile layout. |
| Restaurant draft and menu editor | 🟡 Preview | Validated local draft works. Server persistence and optimistic concurrency are required. |
| Owner/manager/staff policy | ✅ Domain foundation | Central policy denies unknown roles and reserves team management for owners. |
| Secure ownership enforcement | 🔴 Launch blocker | Every write must authenticate JWT and authorize user, restaurant and role in the service database. Browser state is never authority. |
| Compliance onboarding | 🔴 Launch blocker | FSSAI/GST/KYC verification, consent, expiry, object storage, malware scanning and reviewer workflow require contracted providers. |
| Approval and suspension | 🟡 Designed | Partner may submit draft; only a separate admin role may approve or suspend. |
| Menu/category/options CRUD | 🟡 Preview | Item draft exists; API, categories, variants, add-ons, tax, versioning and bulk import remain. |
| Hours/closures/service area | 🔴 Launch blocker | Time-zone schedules, holiday overrides and radius/polygon validation remain. |
| Incoming-order operations | 🔴 Launch blocker | Restaurant-scoped paid-order feed, transitions and timeout policy remain. |
| Team invitations | 🔴 Launch blocker | Disabled until verified email, expiring one-time invites, revocation and audit exist. |
| Audit history | 🟡 Preview | Local history is illustrative. Production needs append-only server events. |
| Analytics and payouts | 🔴 Future | Read models, settlement provider and reconciliation are not implemented. |

## Core data

- Partner membership: user, restaurant, role, state, invitation and revocation times.
- Restaurant profile: legal/display names, contact, coordinates, service area, timezone, status and version.
- Compliance document: type, encrypted object reference, hash, verification and expiry. Never store document bytes in SQLite.
- Operating schedule and closure overrides.
- Menu, category, item, variant and option group. Money is integer paise.
- Restaurant-scoped order projection with customer/payment secrets excluded.
- Audit event with actor, action, resource, result, request ID, time and redacted change summary.

## Non-functional requirements

- p95 reads under 300 ms and writes under 500 ms excluding external verification.
- Idempotency for create, publish, invitation and order-transition commands.
- Optimistic versions prevent silent lost updates; conflicts return 409.
- Logs contain correlation IDs but no JWTs, passwords, bank data, documents or unnecessary PII.
- Data retention, export and deletion policies are enforced by data class.
- WCAG 2.2 AA, responsive layouts, retry-safe commands and degraded read-only mode.

## Launch gate

The preview is not production-ready until all red items pass security review, migration rehearsal, backup/restore, load, accessibility, penetration and incident-response tests.
