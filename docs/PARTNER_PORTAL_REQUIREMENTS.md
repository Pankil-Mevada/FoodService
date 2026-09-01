# Restaurant Partner Portal — Product Requirements

Status: real frontend/backend foundation, 2026-09-01.

## Product outcome

Restaurant owners and authorized staff can onboard a business, maintain its
profile and menu, submit it for independent review, and inspect server activity.
Customer and partner experiences share the central User Service identity but
never share restaurant authorization or browser-session storage. A partner can
register and sign in without leaving `partner.html`.

## Release scope and status

| Capability | Status | Acceptance criteria / remaining work |
| --- | --- | --- |
| Responsive partner workspace | ✅ Implemented | Separate partner page, in-portal registration/login, isolated partner browser session, responsive navigation and recoverable server feedback. |
| Restaurant draft and menu editor | ✅ Core implemented | Restaurant Service persists private drafts/menu items; optimistic versions return 409 on stale writes. |
| Owner/manager/staff policy | ✅ Implemented foundation | Central policy denies unknown roles; every implemented resource resolves ACTIVE membership server-side. |
| Secure ownership enforcement | ✅ Implemented for current routes | Gateway and Restaurant Service verify JWT; Restaurant Service checks user, restaurant and role. Browser selection is never authority. |
| Compliance onboarding | 🔴 Launch blocker | FSSAI/GST/KYC verification, consent, expiry, object storage, malware scanning and reviewer workflow require legal/provider decisions. |
| Approval and suspension | 🟡 Partial | Partner submits to PENDING_REVIEW and cannot self-approve; separate admin API/UI, dual control and suspension remain. |
| Menu/category/options CRUD | 🟡 Partial | Secure item CRUD, integer paise and versions exist; categories, variants, add-ons, tax, stock and bulk import remain. |
| Hours/closures/service area | 🔴 Launch blocker | Radius/profile inputs exist; timezone schedules, holiday overrides and production polygon operations remain. |
| Incoming-order operations | 🔴 Launch blocker | Restaurant-scoped paid-order feed, transitions and timeout policy remain. |
| Team invitations | 🔴 Launch blocker | Disabled until verified email, expiring one-time invites, last-owner protection, revocation and audit exist. |
| Audit history | ✅ Core implemented | Server success events are restaurant-scoped and atomic with writes; denied-event/outbox/retention work remains. |
| Analytics and payouts | 🔴 Future | Read models, settlement provider and reconciliation are not implemented. |

## Core data

- User identity stays in User Service `foodservice.db`.
- Restaurant profile, lifecycle and version stay in Restaurant Service `restaurant.db`.
- `restaurant_partners` links a JWT user ID to a restaurant and role.
- `partner_menu_items` stores integer-paise items and optimistic versions.
- `partner_audit_events` stores actor, action, resource, result, correlation ID and time.
- Browser storage is never restaurant authority.

## Non-functional requirements

- p95 reads under 300 ms and writes under 500 ms excluding providers.
- Durable idempotency for create, submit, invitation and order transitions.
- Optimistic versions prevent silent lost updates; conflicts return 409.
- Logs contain correlation IDs but no JWTs, passwords, bank data, documents or unnecessary PII.
- Data retention, export and deletion policies are enforced by data class.
- WCAG 2.2 AA, responsive layouts, retry-safe commands and degraded read-only mode.

## Launch gate

The portal has a real frontend/backend path, but it is not launch-ready until
email verification/recovery, independent admin approval, compliance/KYC,
restaurant order operations, durable command idempotency/outbox, rate limiting,
managed database migrations/backups, accessibility, load, penetration and
incident-response gates pass. See `RESTAURANT_PARTNER_BACKEND.md`.
