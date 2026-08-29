# FoodService product requirements

Updated 2026-08-29. Statuses describe the exact locally implemented and
validated slice; they do not imply production restaurant or courier integration.
Section 16 is the authoritative whole-document status audit, including the
reason for partial/open work and its planned delivery phase.

## 1. Product vision

Build FoodService into a production-ready local food marketplace inspired by
apps such as Zomato, without copying their brand, content, or interface. The
platform should connect customers, restaurants, delivery partners, and
administrators through reliable web experiences and a secure service backend.

The first commercial milestone is a complete single-city ordering journey:

> A customer signs in, discovers a serviceable restaurant, selects menu items,
> checks out, completes a test or real payment, tracks the order, and receives
> status notifications. The restaurant accepts and prepares the order, while an
> administrator can observe and support the transaction.

## 2. Current baseline

### Available now

- C++20/Crow microservices and API Gateway
- user registration, login, JWT creation, and user CRUD
- restaurant CRUD and restaurant listing
- basic order creation and listing
- SQLite persistence per service
- test-mode payment intents, idempotency, webhooks, and live status snapshots
- authenticated payment-to-order synchronization for processing, success, failure, and cancellation
- notification persistence
- responsive Plated customer frontend
- automated end-to-end test harness and dummy-payment script
- authenticated `/me`; order/payment customer identity derived from JWT
- browser GPS or Bengaluru demo, OpenStreetMap nearby discovery/import, distance display, and radius enforcement
- optional provider-sourced restaurant photos with honest unavailable fallback
- delivery address/coordinates persisted on orders
- real driver browser GPS ingestion, freshness, distance ETA/timeline, and persisted `DELIVERED` state
- local three-item test menu with generated photos, quantities, `WELCOME10`, delivery fee, and persisted price breakdown
- browser-local customer profile photo, display name, phone, test UPI ID, and favourite restaurants
- location-first discovery UI with craving shortcuts, offer presentation, rating/favourite filters, and richer delivery cards
- latest local E2E evidence: 26 passed, 0 failed, 0 skipped

### Gaps that block a real marketplace

- no menu, category, item, variation, or add-on data model
- current menu is a frontend test fixture; prices are not yet sourced from an authoritative server-side catalogue
- no cart or itemized order lines
- passwords are stored/compared/logged as plaintext; hashing utility is incomplete
- no roles or authorization rules for customer, restaurant, delivery, or admin
- public restaurant/order mutation routes are insufficiently protected
- API Gateway uses bounded synchronous service calls and preserves downstream
  status codes; API versioning, correlation IDs, shared errors, retries, and
  circuit breaking remain
- SSE is reconnect-based status sampling rather than a durable event stream
- address book and serviceability work locally; production still needs encrypted profile storage, retention controls, and geospatial indexing
- map/geocoding and nearby discovery rely on public development endpoints; no production maps/routing contract
- OpenStreetMap photo coverage is sparse; production needs licensed photo ingestion and owner moderation
- driver assignment remains manual and the shared local driver token is not production driver identity
- no tax, fee, coupon, or authoritative pricing engine
- no restaurant, delivery-partner, or administrator web portal
- no production payment-provider integration
- profile photo, phone, favourites, and UPI ID are browser-local test data rather than synchronized account records
- no production database, deployment, monitoring, backup, or disaster recovery

## 3. Users and roles

| Role | Primary goals |
|---|---|
| Customer | Discover food, order, pay, track, review, and get support |
| Restaurant owner/manager | Manage restaurant, menus, availability, orders, and reports |
| Restaurant staff | Accept, reject, prepare, and hand off orders |
| Delivery partner | Go online, accept delivery tasks, navigate, and update delivery status |
| Platform administrator | Approve partners, manage users/orders, refunds, content, and operations |
| Support agent | Search orders, view event history, communicate, and resolve incidents |

Every authenticated request must derive user identity and role from a verified
token. Clients must not be trusted to submit arbitrary `userId` values.

## 4. Priority definitions

- **P0 — Marketplace MVP:** required for a safe end-to-end ordering pilot.
- **P1 — Growth:** required for a competitive single-city product.
- **P2 — Scale:** useful after product-market validation and operational scale.

Requirement status symbols used below:

- ✅ **Done:** implemented and covered by focused validation for the development MVP.
- 🟡 **Partial:** locally implemented, but a production provider, security, scale, or operational capability remains.
- ⬜ **Not started:** not implemented yet.

## 5. Functional requirements

### 5.1 Identity and accounts

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Register, sign in/out, and browser session | 🟡 Partial | Registration, login, JWT storage, and sign-out work; password storage and refresh/revocation are not production-safe. | Phase 0 |
| P0 | Access/refresh tokens, expiration, revocation | ⬜ Not started | Only a basic access JWT exists; there is no session store or rotated refresh token. | Phase 0 |
| P0 | Email or phone verification | ⬜ Not started | Verification tokens, delivery provider, endpoints, and UI are absent. | Phase 1 |
| P0 | Forgot/reset password | ⬜ Not started | Secure reset tokens, expiry, delivery, and password-change flow are absent. | Phase 1 |
| P0 | Customer profile and saved addresses | 🟡 Partial | JWT-owned addresses work; profile photo/phone/UPI are partly browser-local and address storage needs production privacy controls. | Phase 1 |
| P0 | Role-based API authorization | ⬜ Not started | JWT identifies a customer but does not enforce customer/restaurant/driver/admin roles. | Phase 0 |
| P0 | Server-derived customer ID | ✅ Done | `/me`, order, payment, and address flows derive identity from verified JWT claims. | Completed |
| P0 | Audit security-sensitive events | ⬜ Not started | There is no immutable account/authentication audit store. | Phase 0 |
| P1 | Social sign-in | ⬜ Not started | OAuth providers and account-linking rules are absent. | Phase 3 |
| P1 | Account deletion and data export | ⬜ Not started | Privacy export/deletion workflows and retention policies are absent. | Phase 2 |
| P1 | Dietary preferences and allergens | ⬜ Not started | Profile and catalogue metadata required to use these preferences do not exist. | Phase 3 |
| P1 | Multiple devices/session management | ⬜ Not started | There is no server-side session inventory or device revocation. | Phase 1 |

### 5.2 Location and serviceability

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Add/list/select/delete delivery addresses | ✅ Done | JWT-owned address CRUD is implemented. | Completed |
| P0 | Label, recipient, phone, coordinates, notes | ✅ Done | All required structured fields persist in the local address table. | Completed |
| P0 | Determine restaurant serviceability | ✅ Done | Gateway evaluates configured polygon or radius. | Completed |
| P0 | Block outside-zone checkout/order | ✅ Done | Quote and order creation independently reject non-serviceable coordinates. | Completed |
| P0 | Estimate distance, fee, and ETA | 🟡 Partial | Backend straight-line distance and formula ETA work; road routing/live traffic do not. | Phase 2 after maps decision |
| P0 | Production address privacy/storage | ⬜ Not started | Dedicated encrypted profile service, retention/deletion, and geospatial indexes are absent. | Phases 1–2 |
| P0 | Address/building verification | ⬜ Not started | Provider verification, apartment metadata, and instruction moderation are absent. | Phase 2 |
| P1 | Map picker and geocoding | 🟡 Partial | Leaflet/OpenStreetMap/Nominatim work locally without a production SLA, quota, or failover. | Phase 2 after provider decision |
| P1 | GPS permission with manual fallback | ✅ Done | GPS is user-triggered and manual/map entry remains available when denied. | Completed |
| P1 | Configurable radius or polygon | ✅ Done | Both restaurant-zone forms are persisted and enforced server-side. | Completed |
| P1 | Surge/rain/late-night/distance rules | 🟡 Partial | Distance and server-clock rules work; rain/surge use development flags instead of trusted feeds. | Phase 2 |

### 5.3 Restaurant discovery

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Browse only active/serviceable restaurants | 🟡 Partial | Nearby/radius display exists and checkout is authoritative, but cards do not evaluate polygon, opening hours, pause, or capacity. | Phase 1 |
| P0 | Complete restaurant cards | 🟡 Partial | Name, rating, address, distance/ETA, offer, and photo/fallback exist; real cuisine, price band, and availability are absent. | Phase 1 |
| P0 | Search restaurant, dish, or cuisine | 🟡 Partial | Restaurant/address text search works; dish/cuisine search needs the server catalogue. | Phase 1 |
| P0 | Cuisine/veg/rating/time/price/offer filters | 🟡 Partial | Rating and favourites filters work; remaining filters need catalogue and authoritative pricing. | Phase 1 |
| P0 | Restaurant detail, hours, policies, menu | ⬜ Not started | No detail route/page, schedule model, policies, or server-owned menu exists. | Phase 1 |
| P0 | Closed/busy/unavailable/paused states | ⬜ Not started | Operational availability and capacity models are absent. | Phase 1 |
| P1 | Personalized recommendations | ⬜ Not started | No behavioral events, customer preference model, or ranking system exists. | Phase 3 |
| P1 | Trending/healthy/budget/late-night collections | ⬜ Not started | Catalogue tags, popularity signals, and schedules are absent. | Phase 3 |
| P1 | Relevance/rating/time/cost/popularity sorting | 🟡 Partial | Rating display exists; consistent server-side ranking/sorting is absent. | Phase 3 |
| P1 | Favourites and recently viewed | 🟡 Partial | Favourites are browser-local; account sync and recently viewed are absent. | Phase 3 |

### 5.4 Restaurant onboarding and management

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Admin approval before visibility | ⬜ Not started | Approval state, verification workflow, admin RBAC, and portal are absent. | Phase 1 |
| P0 | Owner manages business information | 🟡 Partial | Restaurant CRUD exists, but it is not owner-scoped and has no management portal. | Phase 0 RBAC, Phase 1 UI |
| P0 | Hours, holidays, zones, prep time, capacity | 🟡 Partial | Radius/polygon and preparation minutes exist; hours, holidays, and capacity do not. | Phase 1 |
| P0 | Upload images/documents | 🟡 Partial | Image URLs display; secure upload, storage, licensing, moderation, and verification documents are absent. | Phase 1 |
| P0 | Pause ordering/item availability | ⬜ Not started | Restaurant/item operational-state models are absent. | Phase 1 |
| P0 | Assigned-restaurant authorization | ⬜ Not started | Restaurant staff identity and ownership policies are absent. | Phase 0–1 |
| P1 | Multiple outlets | ⬜ Not started | Brand/outlet hierarchy does not exist. | Phase 2 |
| P1 | Staff accounts and permissions | ⬜ Not started | Roles and restaurant assignments are absent. | Phase 1 |
| P1 | Scheduled menus/time availability | ⬜ Not started | Requires catalogue and schedule models. | Phase 2 |
| P1 | Settlement/performance reports | ⬜ Not started | Financial ledgers and analytics pipeline are absent. | Phase 3 |

### 5.5 Menu and catalogue

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Ordered menu categories | ⬜ Not started | Current foods are frontend fixtures; category schema/API is absent. | Phase 1, next major item |
| P0 | Authoritative item fields and tax category | ⬜ Not started | No restaurant-owned menu-item entity exists. | Phase 1 |
| P0 | Vegetarian/non-vegetarian/vegan indicators | ⬜ Not started | Food-type metadata is absent. | Phase 1 |
| P0 | Availability and stock | ⬜ Not started | Inventory/availability fields and workflows are absent. | Phase 1 |
| P0 | Variations | ⬜ Not started | Variant groups/pricing schemas are absent. | Phase 1 |
| P0 | Add-ons with min/max rules | ⬜ Not started | Add-on groups and selection validation are absent. | Phase 1 |
| P0 | Integer minor-unit prices | ⬜ Not started | Current order/restaurant amounts use floating point. | Phase 1 migration |
| P0 | Server calculates all prices | ⬜ Not started | Server recalculates delivery fee/total, but still accepts fixture subtotal/discount. | Phase 1 |
| P1 | Combos, scheduled pricing, inventory quantities | ⬜ Not started | These depend on the base catalogue. | Phase 3 |
| P1 | Allergens and nutrition | ⬜ Not started | Catalogue metadata and moderation are absent. | Phase 3 |
| P1 | Catalogue import/export | ⬜ Not started | Stable catalogue schema and permissions are prerequisites. | Phase 3 |

### 5.6 Cart and checkout

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Add/edit/remove quantities | 🟡 Partial | Browser quantity controls work for fixture items, not an authoritative cart. | Phase 1 |
| P0 | One restaurant per cart | 🟡 Partial | Current modal selects one restaurant, but no server cart invariant exists. | Phase 1 |
| P0 | Persist signed-in cart | ⬜ Not started | Cart/cart-item persistence and APIs are absent. | Phase 1 |
| P0 | Item preparation notes | ⬜ Not started | Itemized cart/order lines are absent. | Phase 1 |
| P0 | Select address/payment method | ✅ Done | Saved address selection and checkout payment-method UI exist. | Completed |
| P0 | Complete subtotal/tax/fees/discount/total | 🟡 Partial | Subtotal, test discount, delivery fee, and total display; tax/packaging and authoritative item prices are absent. | Phase 1 |
| P0 | Validate prices/availability | ⬜ Not started | No server catalogue exists to validate against. | Phase 1 |
| P0 | Recovery after price/availability changes | ⬜ Not started | Depends on authoritative repricing and availability validation. | Phase 1 |
| P0 | Explicit final confirmation | 🟡 Partial | Place Order is explicit, but there is no final server-repriced review screen. | Phase 1 |
| P1 | Scheduled orders | ⬜ Not started | Scheduling and restaurant-hours models are absent. | Phase 3 |
| P1 | Group ordering | ⬜ Not started | Shared cart/session model is absent. | Phase 3 |
| P1 | Delivery tips | ⬜ Not started | Authoritative pricing and courier payout ledger are absent. | Phase 3 |
| P1 | Reusable checkout preferences | ⬜ Not started | Secure synchronized profile preferences are absent. | Phase 3 |

### 5.7 Orders

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Itemized immutable purchase snapshot | 🟡 Partial | Text item summary and totals persist; immutable order-line names/unit prices/options do not. | Phase 1 |
| P0 | Customer sees only owned orders | ✅ Done | Gateway filters the list using verified JWT customer identity. | Completed |
| P0 | Restaurant sees only its orders | ⬜ Not started | Restaurant users, assignments, RBAC, and portal are absent. | Phase 1 |
| P0 | Complete state machine below | 🟡 Partial | Payment and delivery states work; restaurant acceptance/preparation/refund states and history are incomplete. | Phases 1–2 |

Target state machine:

```text
CREATED
  -> PAYMENT_PENDING
  -> CONFIRMED
  -> ACCEPTED
  -> PREPARING
  -> READY_FOR_PICKUP
  -> PICKED_UP
  -> OUT_FOR_DELIVERY
  -> DELIVERED

Failure paths:
PAYMENT_FAILED, REJECTED, CANCELLED, REFUND_PENDING, REFUNDED
```

| P0 | Allowed transitions with actor/timestamp | 🟡 Partial | Payment mapping and delivery progression are constrained; complete immutable actor/timestamp history is absent. | Phases 1–2 |
| P0 | Complete order detail/timeline | 🟡 Partial | Totals, address, summary, payment status, and delivery status display; item lines and full event history are absent. | Phase 1 |
| P0 | Restaurant accept/reject timeout | ⬜ Not started | Restaurant workflow and timeout scheduler are absent. | Phase 1 |
| P0 | State-based cancellation policy | 🟡 Partial | Cancellation works, but policy is not configurable by state/actor/time. | Phase 1 |
| P0 | Public order reference | ⬜ Not started | UI/API expose internal numeric database IDs. | Phase 0 |
| P0 | Idempotent order submission | ⬜ Not started | Payment has idempotency, but order creation lacks a unique order idempotency key. | Phase 0 |
| P1 | Reorder | ⬜ Not started | Requires immutable order lines and current catalogue matching. | Phase 3 |
| P1 | Substitutions/clarification | ⬜ Not started | Messaging and restaurant workflow are absent. | Phase 3 |
| P1 | Proof of delivery/PIN | ⬜ Not started | Delivery verification model and per-driver identity are absent. | Phase 3 |
| P1 | Downloadable invoice | ⬜ Not started | Tax/legal decisions and authoritative pricing are prerequisites. | Phase 2 |

### 5.8 Payments and refunds

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Dedicated payment page and progress/recovery UI | ✅ Done | Separate checkout, progress/retry, popup fallback, refresh recovery, and status polling/SSE exist. | Completed |
| P0 | Payment required before driver assignment | ✅ Done | Gateway and Order Service reject delivery tracking/transitions before verified success. | Completed |
| P0 | Razorpay sandbox checkout | ✅ Done | Real Razorpay test mode plus local dummy fallback is integrated without live charges. | Completed |
| P0 | Backend-only provider order creation | ✅ Done | Razorpay order API is called from Payment Service. | Completed |
| P0 | Never store raw card/UPI credentials | ✅ Done | Sensitive checkout fields remain provider-hosted; browser profile stores only a test UPI identifier. | Completed |
| P0 | Verify provider signature | ✅ Done | Server verifies Razorpay HMAC-SHA256 signature before success. | Completed |
| P0 | Idempotent payment creation | ✅ Done | Unique idempotency key returns the existing payment instead of duplicating it. | Completed |
| P0 | Webhook idempotency/event log | 🟡 Partial | State transitions reject duplicates/regressions; durable raw-event deduplication and append-only audit are absent. | Phase 0 |
| P0 | Payment/order synchronized states | ✅ Done | Authenticated callbacks map processing/success/failure/cancel and preserve later delivery states. | Completed |
| P0 | Success/failure/cancel/timeout cases | 🟡 Partial | Success, failure, cancellation, and duplicates are covered; provider timeout reconciliation is absent. | Phase 2 |
| P0 | Cash on delivery | 🟡 Partial | COD is visible as a separate UI method but lacks a backend lifecycle/policy. | Phase 1 |
| P0 | Refund lifecycle/reconciliation | ⬜ Not started | Refund records, provider operations, webhooks, and admin workflow are absent. | Phase 2 |
| P0 | Currency and integer minor units | ⬜ Not started | Current payment/order amounts use floating point and implicit INR. | Phase 1 migration |
| P1 | Saved provider payment tokens | ⬜ Not started | Provider tokenization/consent workflows are absent. | Phase 3 |
| P1 | Partial/multiple refunds | ⬜ Not started | Base refund model is not implemented. | Phase 3 |
| P1 | Restaurant settlement ledger | ⬜ Not started | Financial ledger and payout workflow are absent. | Phase 3 |
| P1 | Reconciliation reports/alerts | ⬜ Not started | Scheduled reconciliation, mismatch store, dashboard, and alerting are absent. | Phase 2–3 |

### 5.9 Live status and notifications

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Durable order/payment event history | 🟡 Partial | Current state and notifications persist; complete append-only state history does not. | Phase 1 |
| P0 | Authenticated SSE/WebSocket updates | 🟡 Partial | Reconnecting payment SSE snapshots plus polling work; stream auth and durable events are incomplete. | Phase 2 |
| P0 | Cursor reconnect without loss | ⬜ Not started | SSE has no durable cursor/event replay. | Phase 2 |
| P0 | In-app transition notifications | 🟡 Partial | Notification persistence exists; complete notification-center UI and all order transitions are not wired. | Phase 2 |
| P0 | Email/SMS/push abstraction | ⬜ Not started | Providers, templates, and channel interfaces are absent. | Phase 2 |
| P0 | Retry/backoff/dead letter | ⬜ Not started | No delivery-attempt queue or dead-letter store exists. | Phase 2 |
| P0 | Customer notification preferences | ⬜ Not started | Synchronized profile preference model is absent. | Phase 2 |
| P1 | Restaurant new-order alert/acknowledgement | ⬜ Not started | Restaurant portal and acknowledgement workflow are absent. | Phase 2 |
| P1 | Delivery task notifications | ⬜ Not started | Delivery-task/dispatch domain is absent. | Phase 3 |
| P1 | Customer/restaurant/support chat | ⬜ Not started | Messaging, moderation, retention, and abuse controls are absent. | Phase 3 |

### 5.10 Delivery operations

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Real browser GPS driver portal | ✅ Done | Local token-protected portal uses `watchPosition`. | Completed development MVP |
| P0 | Latest fix, accuracy/speed/heading/stale/ETA | ✅ Done | Backend persists and UI displays these values with 30-second freshness. | Completed development MVP |
| P0 | Persist delivered state | ✅ Done | Final `DELIVERED` state is synchronized into Order Service storage. | Completed development MVP |
| P0 | Partner onboarding/approval | ⬜ Not started | Driver account, KYC/verification, approval, and device identity are absent. | Phase 3 |
| P0 | Online/offline availability | ⬜ Not started | Driver availability/capacity model is absent. | Phase 3 |
| P0 | Manual/simple assignment | ⬜ Not started | No delivery task queue or dispatch policy exists. | Phase 3 |
| P0 | Transitions and timestamps | 🟡 Partial | Status progression exists; immutable actor/timestamp event history is incomplete. | Phase 3 |
| P0 | Pickup/delivery verification | ⬜ Not started | PIN/photo/signature/handoff proof is absent. | Phase 3 |
| P0 | Privacy-preserving customer tracking | 🟡 Partial | Customer receives limited current data, but consent, retention, per-driver scope, and deletion policies are absent. | Phases 2–3 |
| P1 | Production consent/retention/per-driver auth | ⬜ Not started | Shared development token is not a production identity or consent system. | Phase 3 |
| P1 | Road route/ETA integration | ⬜ Not started | No contracted routing provider exists. | Phase 2–3 |
| P1 | Automatic dispatch/reassignment | ⬜ Not started | Matching, capacity, timeout, and reassignment engine are absent. | Phase 3 |
| P1 | Earnings/payout ledger | ⬜ Not started | Courier finance and settlement models are absent. | Phase 3 |

### 5.11 Ratings and reviews

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P1 | Delivered customers can review | ⬜ Not started | Review entity/API and delivered-order eligibility checks are absent. | Phase 3 |
| P1 | Food/restaurant/delivery ratings | ⬜ Not started | Itemized orders and review dimensions are absent. | Phase 3 |
| P1 | One review/order with edit window | ⬜ Not started | Review uniqueness, lifecycle, and timestamps are absent. | Phase 3 |
| P1 | Restaurant responses | ⬜ Not started | Restaurant identity/portal and response model are absent. | Phase 3 |
| P1 | Reporting/moderation/anti-abuse | ⬜ Not started | Admin moderation, reports, fraud signals, and audit workflow are absent. | Phase 3 |
| P1 | Approved aggregate ratings | ⬜ Not started | Approved review source data and recalculation jobs are absent. | Phase 3 |

### 5.12 Offers and promotions

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P1 | Coupon validity/limits/minimum/restaurant/user rules | 🟡 Partial | `WELCOME10` demonstrates UI calculation only; durable rule and redemption models are absent. | Phase 3 after pricing engine |
| P1 | Restaurant/platform funding | ⬜ Not started | Promotion funding and settlement attribution are absent. | Phase 3 |
| P1 | Server-side promotion calculation | ⬜ Not started | Discount is currently calculated in the browser fixture flow. | Phase 1 base pricing, Phase 3 rules |
| P1 | Referral credits/wallet ledger | ⬜ Not started | Credit ledger, expiry, and accounting controls are absent. | Phase 3 |
| P1 | Stacking/abuse prevention | ⬜ Not started | Server promotion engine, usage history, and fraud rules are absent. | Phase 3 |

### 5.13 Administration and support

| Priority | Requirement | Status | Why | Planned phase |
|---|---|---|---|---|
| P0 | Protected admin portal | ⬜ Not started | Admin role, authorization policy, and UI are absent. | Phase 1 |
| P0 | Approve/suspend restaurants/users | ⬜ Not started | Approval/suspension states, reasons, audit, and workflows are absent. | Phase 1 |
| P0 | Cross-domain operational search | ⬜ Not started | No admin search API/index or protected detail UI exists. | Phase 1–2 |
| P0 | Complete order/payment timeline | 🟡 Partial | Current states can be queried, but append-only cross-service event history is absent. | Phase 1–2 |
| P0 | Cancel/refund with audit reason | 🟡 Partial | Customer cancellation exists; admin identity, refund action, reason, and audit are absent. | Phase 2 |
| P0 | Configure zones/fees/taxes/platform | 🟡 Partial | Restaurant radius/polygon and fee fields are configurable through APIs; protected admin UI, taxes, and global settings are absent. | Phase 1–2 |
| P0 | Immutable privileged-action audit | ⬜ Not started | Audit event model/store does not exist. | Phase 0 |
| P1 | Support tickets/internal notes | ⬜ Not started | Support entities, permissions, SLA, and UI are absent. | Phase 2 |
| P1 | Content moderation | ⬜ Not started | Moderation queues and policies are absent. | Phase 3 |
| P1 | Operational dashboard/SLA alerts | ⬜ Not started | Metrics pipeline, dashboards, and alert rules are absent. | Phase 2 |
| P1 | Access-controlled CSV exports | ⬜ Not started | Admin RBAC, export jobs, and data-redaction rules are absent. | Phase 3 |

## 6. Frontend applications

### Customer web app — P0

- responsive mobile-first layout
- accessible keyboard navigation, focus handling, labels, and status announcements
- home/discovery, restaurant/menu, cart, checkout, order tracking, order history,
  account, addresses, and support screens
- authenticated identity replaces manual user-ID fields
- errors explain how to recover and never display an infinite spinner
- skeleton, empty, offline, retry, and partial-failure states

### Restaurant portal — P0

- secure restaurant/staff login
- incoming-order queue with audible/visual alert
- accept/reject, preparation time, status updates, and item availability
- menu and restaurant-profile management

### Admin portal — P0

- operational search and detail screens
- approval, suspension, refund, audit, and configuration workflows

### Delivery portal/app — later milestone

- task list, pickup/delivery workflow, navigation handoff, and earnings

## 7. Backend and API requirements

### P0 architecture improvements

- version public routes under `/api/v1`
- [x] preserve downstream HTTP status and map transport timeout/unavailability
  to structured gateway errors; general downstream-header forwarding remains
- consistent response/error schema with a request correlation ID
- authentication and authorization policy per route
- server-derived user/restaurant identity
- request validation with field-level errors
- pagination, filtering, and stable sorting for collection endpoints
- idempotency keys for order, payment, refund, and webhook operations
- rate limits for login, registration, search, and mutation endpoints
- OpenAPI specification as the API contract
- database migrations instead of startup-time ad hoc `ALTER TABLE`
- transactional updates within each service boundary
- event/outbox approach for cross-service consistency
- health endpoints separated into liveness and readiness

### Service evolution

- expand User Service for profiles, roles, addresses, sessions, and verification
- expand Restaurant Service for outlets, serviceability, menus, inventory, and hours
- expand Order Service for carts, line items, pricing snapshots, state history, and
  order/payment consistency
- expand Payment Service for provider adapters, verified webhooks, refunds,
  reconciliation, and audit events
- expand Notification Service for channels, templates, retries, and preferences
- introduce Delivery Service when delivery-partner workflows begin
- consider Search Service only after database search becomes insufficient

## 8. Data requirements

### P0 entities

- users, roles, sessions, verification tokens
- customer profiles and addresses
- restaurant brands, outlets, staff, hours, service zones
- menu categories, items, variants, add-ons, availability
- carts and cart items
- orders, order items, price components, address snapshots, status history
- payments, payment attempts, webhook events, refunds
- notifications and delivery attempts
- audit events

### Database rules

- use PostgreSQL for shared production-grade persistence; keep SQLite for local
  development and focused tests
- store timestamps in UTC and render in the user's timezone
- store money as integer minor units with ISO currency code
- define foreign keys, uniqueness constraints, and indexes from access patterns
- encrypt sensitive data in transit and at rest
- document retention and deletion rules for personal and location data
- automated backups with tested restore procedures

## 9. Non-functional requirements

### Security — P0

- OWASP-aligned input validation and output encoding
- password hashing with Argon2 using reviewed parameters (required; not yet implemented)
- short-lived access tokens and rotated refresh tokens
- secrets from a managed secret store; none in source or frontend code
- least-privilege service and database credentials
- CSRF protection where cookies are used and strict CORS allowlists
- webhook signatures, replay protection, and event deduplication
- dependency and container vulnerability scanning
- audit logs for authentication, role, restaurant, order, payment, and refund actions

### Reliability — P0

- retries only for safe/idempotent operations
- [x] bounded connect/total timeouts on gateway service calls; circuit breaking remains
- no order remains indefinitely in an unexplained intermediate state
- reconciliation jobs for payment/order mismatches
- graceful degradation when notification or recommendation systems fail
- documented backup, restore, rollback, and incident procedures

### Performance targets for MVP

- p95 read API latency under 500 ms at expected pilot load
- p95 mutation latency under 1 second excluding external-provider confirmation
- first meaningful customer page content within 2.5 seconds on typical mobile data
- live order/payment status visible within 5 seconds
- paginated endpoints remain bounded as data grows

### Accessibility and compatibility

- target WCAG 2.2 AA for customer and operator web apps
- support current and previous major versions of Chrome, Edge, Firefox, and Safari
- responsive layouts from 320 px mobile width upward
- reduced-motion and high-contrast support

### Observability

- structured logs with correlation ID, service, route, status, latency, and error code
- metrics for traffic, latency, errors, orders, payments, webhooks, and notification delivery
- distributed tracing across the gateway and services
- alerts for service unavailability, payment mismatches, elevated failures, and stuck orders
- never log passwords, JWTs, payment credentials, or unnecessary personal data

## 10. Testing requirements

### P0

- unit tests for validation, pricing, state transitions, and authorization
- repository integration tests against isolated databases
- API contract tests for every route and error schema
- end-to-end customer order and payment flows
- tests for invalid identity, role access, duplicate requests, and tampered amounts
- payment tests for success, failure, timeout, cancellation, duplicate/out-of-order
  webhooks, and reconciliation
- browser tests for customer, restaurant, and admin critical paths
- migration tests from the previous released schema
- load tests for browse, checkout, and live-status endpoints
- security tests for common authentication and authorization failures

No test may charge real money or use production customer data.

## 11. Delivery roadmap

### Phase 0 — Stabilize the current foundation

- [x] preserve gateway downstream status and map transport timeout/unavailability;
  complete shared error schema and correlation IDs remain
- [x] derive customer identity from JWT and remove manual user ID (gateway order/payment creation and E2E verified)
- protect restaurant, order, payment, and notification routes by role
- [x] make payment callbacks update order status consistently (authenticated callback, idempotent state mapping, focused C++ test, and expanded E2E coverage)
- add database migrations, unit tests, and CI
- create repeatable one-command local startup

### Phase 1 — Ordering MVP

- ✅ Local address book and serviceability: JWT ownership, GPS/manual/map selection, radius/polygon validation, quote, and checkout enforcement.
- 🟡 Development geocoding and map tiles using OpenStreetMap/Nominatim.
- ⬜ Production address privacy/retention controls and contracted map/routing provider.
- menu/category/item/add-on model
- cart, server-side pricing, taxes, fees, and itemized orders
- customer restaurant/menu/cart/checkout/order-history screens
- restaurant order-management and menu-management portal
- admin approval and order-support portal
- production-provider sandbox integration

### Phase 2 — Single-city pilot

- real provider go-live readiness and refunds
- restaurant availability and preparation workflow
- email/SMS/push notifications
- operational monitoring, backups, deployment, and support procedures
- limited delivery dispatch or restaurant pickup model

### Phase 3 — Growth

- [x] real browser GPS delivery tracking and persisted final state
- production delivery-partner onboarding, dispatch, tracking ingestion, verification, and retention
- ratings/reviews, favourites, coupons, recommendations, and scheduled orders
- restaurant analytics and settlements
- stronger search and personalized discovery

### Phase 4 — Scale

- multi-city configuration
- high-availability databases, caching, queues, and autoscaling
- fraud/risk systems, advanced dispatch, experimentation, and data platform

## 12. MVP acceptance criteria

The marketplace MVP is complete when all of the following are demonstrable:

1. A verified customer signs in without entering a user ID manually.
2. The customer selects a valid address and sees only serviceable restaurants.
3. A restaurant exposes an available, itemized menu.
4. The customer builds a cart and sees a server-calculated price breakdown.
5. Checkout creates exactly one order and one idempotent payment attempt.
6. A sandbox provider can produce success, failure, cancellation, and timeout.
7. Payment status updates order status consistently and appears live in the UI.
8. The restaurant accepts the order and progresses it through preparation.
9. The customer sees a complete order timeline and receives notifications.
10. Admin can locate the transaction and view its audit trail.
11. Unauthorized users cannot access or mutate another user's/restaurant's data.
12. Automated tests cover the critical flow and run in CI without real charges.
13. Logs, metrics, backups, and a documented recovery path are available.

## 13. Decisions required before Phase 1

- first launch model: delivery, restaurant pickup, or both
- launch city/country, currency, tax, invoice, and consumer-protection rules
- payment provider: Razorpay, Stripe, or another supported provider
- maps/geocoding/ETA provider
- email, SMS, and push providers
- PostgreSQL hosting and deployment platform
- frontend framework decision: retain dependency-free JavaScript or migrate to a
  structured framework such as React/TypeScript
- service communication strategy: synchronous HTTP only or HTTP plus durable events
- restaurant commercial model, fees, settlements, refunds, and cancellation policy
- privacy retention periods and support escalation process

## 14. Resume checklist for a future Codex task

Use this prompt:

```text
Resume FoodService development toward the marketplace requirements.
Read docs/PRODUCT_REQUIREMENTS.md, docs/ARCHITECTURE.md,
docs/SEQUENCE_DIAGRAMS.md, docs/API.md, and docs/TESTING.md.
Inspect git status, the current branch, recent commits, and open pull requests.
Select the next incomplete roadmap item, state its acceptance criteria, implement
it with tests, and update the documentation. Do not commit database/build files
or use real payment credentials.
```

At the start of each milestone, move selected requirements into an issue or task
list with an owner, target release, dependencies, and testable acceptance criteria.
## 15. Marketplace delivery status summary

- ✅ Signed-in customers can add, list, select, and delete delivery addresses.
- ✅ Each address stores label, recipient, phone, coordinates, address line, and delivery notes.
- ✅ GPS selection, manual entry, OpenStreetMap click selection, and Nominatim search are available; manual/GPS remain usable if map search fails.
- ✅ Restaurant serviceability is enforced on the server using a radius or configured polygon.
- ✅ Checkout is blocked outside the delivery zone and order creation independently rechecks it.
- ✅ The server estimates straight-line distance, delivery fee, and ETA.
- ✅ Base/per-kilometre fees and restaurant preparation minutes are configurable per restaurant.
- 🟡 Surge, rain, and late-night multipliers are server-controlled, but rain and surge currently use development flags rather than real operational feeds.
- ⬜ Road-network distance, live traffic ETA, trusted weather, demand-driven surge, production geocoding contracts, and large-scale geospatial storage.

Production follow-up: replace straight-line distance with a contracted routing provider, obtain rain/surge signals from trusted operations/weather systems, move addresses to a dedicated profile service with encryption/retention controls, and use a geospatial database/index for large-scale polygon queries.

## 16. Complete requirements status audit

This audit covers every requirements area in this document. “When” means the
planned roadmap phase, not a promised calendar date. Work is selected in phase
order after its prerequisites and product/provider decisions are resolved.

### Delivery timing used in this audit

| When | Meaning |
|---|---|
| **Now / completed** | Present in the local development MVP and covered by focused checks or the E2E harness. |
| **Next — Phase 0** | Security, correctness, repeatable builds, migrations, and API foundation; must precede marketplace feature growth. |
| **Then — Phase 1** | Complete customer ordering, authoritative catalogue/cart/pricing, and basic operator portals. |
| **Then — Phase 2** | Single-city operational pilot with providers, preparation workflow, notifications, monitoring, and recovery. |
| **Later — Phase 3** | Delivery operations and growth features after the ordering pilot is stable. |
| **Later — Phase 4** | Multi-city scale, high availability, fraud/risk, and data platform work. |
| **Decision required** | Cannot be finalized responsibly until the named business/provider decision is made. |

### Sections 1–4 — Vision, baseline, roles, and priorities

| Requirement area | Status | Why | When |
|---|---|---|---|
| Product vision and single-city journey | 🟡 Partial | Customer discovery, local ordering, test payment, and GPS tracking exist; restaurant acceptance and admin/support operations do not. | Phases 1–2 |
| Current baseline inventory | ✅ Done | The baseline and known blockers reflect the current repository. | Now / completed |
| Customer role | 🟡 Partial | JWT customer identity and owned orders/addresses exist; verification, secure profile lifecycle, support, and reviews remain. | Phases 0–3 |
| Restaurant owner/staff roles | ⬜ Not started | There is restaurant CRUD but no authenticated owner/staff identity or portal. | Phase 1 |
| Delivery-partner role | 🟡 Partial | A token-protected local GPS page exists; onboarding, per-driver identity, task assignment, and earnings do not. | Phase 3 |
| Administrator/support roles | ⬜ Not started | No RBAC-backed admin/support portal, audit workflow, or case management exists. | Phases 1–2 |
| P0/P1/P2 prioritization | ✅ Done | Priorities and phased sequencing are documented. | Now / completed |

### Section 5.1 — Identity and accounts

| Requirement | Status | Why | When |
|---|---|---|---|
| Register, sign in/out, browser session | 🟡 Partial | Registration, login, JWT storage, and sign-out work, but password storage is not production-safe and the browser session has no refresh/revocation flow. | Phase 0 |
| Access/refresh tokens, expiry, revocation, multiple devices | ⬜ Not started | Only a basic access JWT exists; there is no session store or rotated refresh token. | Phase 0; multi-device completion Phase 1 |
| Email/phone verification and password recovery | ⬜ Not started | Verification tokens, provider delivery, reset endpoints, and screens are absent. | Phase 1 |
| Customer profile and saved addresses | 🟡 Partial | Address book is JWT-owned; name/photo/phone/UPI preferences are partly browser-local and addresses live in the gateway MVP database. | Phase 1 |
| Role-based authorization | ⬜ Not started | Tokens identify a user but do not carry/enforce marketplace roles. | Next — Phase 0 |
| Server-derived customer identity | ✅ Done | `/me`, orders, payments, and addresses derive customer identity from verified JWT data. | Now / completed |
| Security audit events | ⬜ Not started | There is no immutable authentication/account audit store. | Phase 0 |
| Social sign-in, export/deletion, dietary preferences | ⬜ Not started | Provider integration and privacy/profile models are absent. | Phase 3 |

### Section 5.2 — Location and serviceability

| Requirement | Status | Why | When |
|---|---|---|---|
| Add/select structured delivery addresses | ✅ Done | JWT-owned CRUD stores label, recipient, phone, address, coordinates, and notes. | Now / completed |
| GPS, manual fallback, map picker, geocoding | 🟡 Partial | Browser GPS, manual input, Leaflet/OpenStreetMap, and Nominatim work for development; there is no contracted SLA, quota, or provider failover. | Provider decision, then Phase 2 |
| Radius/polygon serviceability and checkout blocking | ✅ Done | Quote and order creation both enforce server-side radius or polygon rules. | Now / completed |
| Distance, delivery fee, and ETA | 🟡 Partial | Backend straight-line estimates work; road routing and live traffic do not. | Provider decision, then Phase 2 |
| Surge, rain, late-night, distance rules | 🟡 Partial | Distance and server-time rules work; rain/surge are development flags rather than trusted weather/demand signals. | Phase 2 |
| Production address privacy/geospatial storage | ⬜ Not started | Encryption, retention/deletion, PostGIS indexes, and a dedicated address/profile service are absent. | Phases 1–2 |

### Section 5.3 — Restaurant discovery

| Requirement | Status | Why | When |
|---|---|---|---|
| Nearby restaurant discovery/import | 🟡 Partial | User-triggered OpenStreetMap discovery works, but public development data is not an authoritative commercial catalogue. | Phase 2 |
| Serviceable-only browsing | 🟡 Partial | Cards apply a radius approximation and checkout enforces the authoritative rule; cards do not batch-evaluate polygons/opening state. | Phase 1 |
| Restaurant cards | 🟡 Partial | Name, rating, address, estimated distance/time, offers, and photos/fallbacks exist; price band and real availability are missing. | Phase 1 |
| Search and filters | 🟡 Partial | Restaurant/address text search, rating, and favourites exist; dish/cuisine/veg/time/cost/offer filtering requires a real catalogue. | Phase 1 |
| Restaurant detail/menu/hours/policies | ⬜ Not started | There is no authoritative detail page, hours model, or server menu. | Phase 1 |
| Closed/busy/paused states | ⬜ Not started | Restaurant capacity and availability state models are absent. | Phase 1 |
| Recommendations, collections, sorting | ⬜ Not started | No behavioral data, ranking service, or catalogue metadata exists. | Phase 3 |
| Favourites/recently viewed | 🟡 Partial | Favourites work only in browser storage; recently viewed and account synchronization are absent. | Phase 3 |

### Section 5.4 — Restaurant onboarding and management

| Requirement | Status | Why | When |
|---|---|---|---|
| Restaurant business CRUD | 🟡 Partial | Backend CRUD and service-zone/pricing fields exist, but routes are not owner-scoped and there is no management UI. | Phase 0 authorization, Phase 1 portal |
| Admin approval and verification documents | ⬜ Not started | No approval state, document storage, moderation, or admin workflow exists. | Phase 1 |
| Hours, holidays, capacity, pause/resume | 🟡 Partial | Preparation time and service zone exist; schedules, capacity, and operational pause states do not. | Phase 1 |
| Image management | 🟡 Partial | Image URLs and local generated test photos display; upload, licensing, moderation, and owner control are absent. | Phase 1 |
| Restaurant staff authorization | ⬜ Not started | No staff accounts, restaurant assignment, or permission checks exist. | Phase 1 |
| Outlets, scheduled menus, settlements/reports | ⬜ Not started | Required outlet, schedule, ledger, and analytics models are absent. | Phases 2–3 |

### Section 5.5 — Menu and catalogue

| Requirement | Status | Why | When |
|---|---|---|---|
| Categories and authoritative menu items | ⬜ Not started | The three displayed foods are frontend fixtures, not restaurant-owned database records. | Next major item in Phase 1 |
| Food type, availability, variants, add-ons | ⬜ Not started | Catalogue schemas and validation rules are absent. | Phase 1 |
| Integer minor-unit pricing | ⬜ Not started | Existing restaurant/order amounts use floating point. | Phase 1 migration |
| Server-authoritative item pricing | ⬜ Not started | The server currently trusts submitted subtotal/discount while recalculating only delivery fee/total. | Phase 1 |
| Combos, nutrition, inventory, import/export | ⬜ Not started | These depend on the base catalogue model. | Phase 3 |

### Section 5.6 — Cart and checkout

| Requirement | Status | Why | When |
|---|---|---|---|
| Add/edit/remove quantities | 🟡 Partial | Quantity controls work in the current browser checkout but are based on fixture items. | Phase 1 |
| One restaurant per cart | 🟡 Partial | The current modal naturally selects one restaurant, but there is no persistent server cart invariant. | Phase 1 |
| Persisted cart and item notes | ⬜ Not started | Cart/cart-item models and APIs are absent. | Phase 1 |
| Select address/payment method | ✅ Done | Saved address selection and payment method/checkout surfaces exist. | Now / completed |
| Complete price breakdown | 🟡 Partial | Subtotal, test discount, delivery fee, and total display; authoritative tax/packaging/item pricing are absent. | Phase 1 |
| Price/availability revalidation and recovery | ⬜ Not started | No authoritative catalogue exists to revalidate. | Phase 1 |
| Explicit confirmation | 🟡 Partial | “Place order” is explicit, but there is no final review screen after server repricing. | Phase 1 |
| Scheduled/group orders, tips, preferences | ⬜ Not started | Supporting models and workflows are absent. | Phase 3 |

### Section 5.7 — Orders

| Requirement | Status | Why | When |
|---|---|---|---|
| Customer-owned order listing | ✅ Done | Gateway filters by verified JWT customer identity. | Now / completed |
| Itemized immutable snapshot | 🟡 Partial | A text item summary and price components persist; immutable order-line records and per-item prices do not. | Phase 1 |
| Payment/order state consistency | ✅ Done | Authenticated idempotent mapping handles processing, success, failure, and cancellation without regressing delivery states. | Now / completed |
| Full restaurant/delivery state machine and timestamps | 🟡 Partial | Payment and delivery states exist; accepted/preparing/ready/rejected/refund history with actor/timestamp is incomplete. | Phases 1–2 |
| Restaurant-scoped orders and acceptance timeout | ⬜ Not started | Restaurant RBAC and operator workflow are absent. | Phase 1 |
| Customer cancellation policy | 🟡 Partial | Cancellation exists, but policy is not state/actor/time configurable. | Phase 1 |
| Public order reference and idempotent order creation | ⬜ Not started | Orders expose internal IDs and lack an order idempotency key. | Phase 0 |
| Reorder, substitutions, proof/PIN, invoice | ⬜ Not started | These depend on itemized orders and restaurant/delivery workflows. | Phase 3 |

### Section 5.8 — Payments and refunds

| Requirement | Status | Why | When |
|---|---|---|---|
| Dedicated payment page and UI recovery | ✅ Done | Separate checkout, progress/retry states, popup fallback, refresh recovery, and polling/SSE status are implemented. | Now / completed |
| Razorpay sandbox order and signature verification | ✅ Done | Provider order creation is backend-only and HMAC verification is server-side; no raw card/UPI secrets are stored. | Now / completed |
| Payment idempotency and valid transitions | ✅ Done | Unique idempotency keys and constrained state changes are implemented and tested locally. | Now / completed |
| Webhook durable event log/replay protection | 🟡 Partial | Secret/signature checks and constrained callbacks exist; raw provider-event deduplication and durable event audit are incomplete. | Phase 0 |
| Payment-to-order synchronization | ✅ Done | Authenticated internal callback maps payment outcomes to order states idempotently. | Now / completed |
| Provider timeout/reconciliation | ⬜ Not started | No scheduled provider query or mismatch repair job exists. | Phase 2 |
| Cash on delivery | 🟡 Partial | It appears as a UI method but lacks a dedicated backend COD lifecycle/policy. | Phase 1 |
| Refunds and reconciliation | ⬜ Not started | Refund entities, provider calls, webhooks, and operator workflows are absent. | Phase 2 |
| Currency plus integer minor units | ⬜ Not started | Current models use floating point and implicit INR. | Phase 1 migration |
| Saved tokens, partial refunds, settlements | ⬜ Not started | Provider tokenization and financial ledgers are absent. | Phase 3 |

### Section 5.9 — Live status and notifications

| Requirement | Status | Why | When |
|---|---|---|---|
| Live payment/order browser status | 🟡 Partial | Reconnecting SSE snapshots plus polling work; there is no authenticated durable event stream with cursor replay. | Phase 2 |
| Durable event history | 🟡 Partial | Current state and notifications persist, but a complete append-only order/payment history does not. | Phase 1 |
| In-app notifications | 🟡 Partial | Notification persistence exists; a complete customer notification center and preferences do not. | Phase 2 |
| Email/SMS/push, retries, dead letters | ⬜ Not started | Channel providers, templates, delivery attempts, retry queues, and DLQ are absent. | Phase 2 |
| Restaurant/delivery alerts and chat | ⬜ Not started | Operator applications and messaging/abuse controls are absent. | Phase 3 |

### Section 5.10 — Delivery operations

| Requirement | Status | Why | When |
|---|---|---|---|
| Real browser driver GPS and freshness | ✅ Done | Driver portal uses `watchPosition`; latest fix, accuracy, speed, heading, age, and stale state persist/display. | Now / completed |
| Payment-gated tracking and delivered persistence | ✅ Done | Tracking is unavailable before verified payment; final delivered state persists. | Now / completed |
| Driver onboarding, identity, availability | ⬜ Not started | The MVP uses a shared development token, not verified per-driver accounts. | Phase 3 |
| Assignment/dispatch/reassignment | ⬜ Not started | No delivery task queue, matching rules, capacity, or reassignment engine exists. | Phase 3 |
| State timestamps and pickup/delivery verification | 🟡 Partial | State labels exist; immutable actor/timestamp history, pickup proof, PIN, and photo/signature verification do not. | Phase 3 |
| Customer privacy and location retention | 🟡 Partial | Customer receives only current tracking data, but consent, scoped driver authorization, retention, and deletion jobs are absent. | Phases 2–3 |
| Routing, earnings, payouts | ⬜ Not started | No route provider or courier financial ledger exists. | Phase 3 |

### Sections 5.11–5.13 — Reviews, promotions, administration, and support

| Requirement area | Status | Why | When |
|---|---|---|---|
| Ratings/reviews | ⬜ Not started | There is no review entity, delivered-order eligibility, moderation, or aggregate recalculation. | Phase 3 |
| Favourites | 🟡 Partial | Browser-local favourites work but are not synchronized or governed by an API. | Phase 3 |
| Coupons/promotions | 🟡 Partial | `WELCOME10` is a browser test rule; validity, funding, limits, stacking, abuse checks, and a server ledger are absent. | Phase 3 after authoritative pricing |
| Admin portal and privileged workflows | ⬜ Not started | No admin RBAC, approval/suspension, refund/configuration, or audit UI exists. | Phase 1 basic; Phase 2 operational |
| Support tickets, moderation, dashboards, exports | ⬜ Not started | Support and operations data models are absent. | Phases 2–3 |

### Section 6 — Frontend applications

| Application/requirement | Status | Why | When |
|---|---|---|---|
| Customer responsive web app | 🟡 Partial | Discovery, auth, addresses/map, fixture menu/cart, payment, orders, profile, and tracking exist; restaurant detail, authoritative cart, support, and full offline/error coverage remain. | Phase 1 |
| Accessibility foundation | 🟡 Partial | Labels, dialogs, focusable controls, status regions, responsive layouts, and reduced clutter exist; no WCAG audit or cross-browser automation has been completed. | Phases 1–2 |
| Restaurant portal | ⬜ Not started | No secure staff UI or operational order/menu workflow exists. | Phase 1 |
| Admin portal | ⬜ Not started | No admin search, approval, refund, audit, or configuration UI exists. | Phase 1 basic; Phase 2 complete |
| Delivery portal/app | 🟡 Partial | Live GPS sharing form exists; task list, navigation handoff, verification, and earnings do not. | Phase 3 |

### Section 7 — Backend and API

| Requirement | Status | Why | When |
|---|---|---|---|
| API Gateway and domain microservices | ✅ Done | User, restaurant, order, payment, notification services and a gateway communicate over HTTP. | Now / completed |
| `/api/v1`, status/error consistency, correlation IDs | 🟡 Partial | Gateway now preserves downstream status and maps connection/transport failures to 502 and timeouts to 504; routes remain unversioned and shared error codes/correlation IDs are absent. | Phase 0 |
| Authentication and server-derived customer identity | 🟡 Partial | Customer identity is server-derived on critical flows; role/restaurant/admin policies are absent. | Phase 0 |
| Validation, pagination, filtering, sorting | 🟡 Partial | Important fields have targeted validation, but there is no shared field-error framework or bounded collections. | Phase 0 |
| Idempotency | 🟡 Partial | Payment creation is idempotent; orders, refunds, and raw webhook events are not fully covered. | Phase 0 |
| Rate limiting and OpenAPI | ⬜ Not started | No limiter or generated/validated API contract exists. | Phase 0 |
| Database migrations | ⬜ Not started | Services still perform startup-time table creation and ad-hoc `ALTER TABLE`. | Phase 0 |
| Transactions and cross-service consistency | 🟡 Partial | SQLite writes are serialized and payment/order callbacks are retry-safe; there is no outbox/event bus or distributed recovery. | Phase 0 foundation; Phase 2 outbox |
| Liveness/readiness separation | ⬜ Not started | Only simple health endpoints exist. | Phase 0 |
| Service evolution list | 🟡 Partial | Each named service has a minimal foundation; profiles/catalogue/cart/refunds/channels/delivery domains remain. | Phases 1–3 |

### Section 8 — Data requirements

| Requirement area | Status | Why | When |
|---|---|---|---|
| Users and customer addresses | 🟡 Partial | Basic users and JWT-owned addresses exist; roles, sessions, verification, secure profiles, and retention do not. | Phases 0–1 |
| Restaurant and service zones | 🟡 Partial | Restaurant/outlet-like record, radius/polygon, and pricing inputs exist; brands, staff, hours, documents, and availability do not. | Phase 1 |
| Catalogue and carts | ⬜ Not started | All authoritative catalogue/cart entities are absent. | Phase 1 |
| Orders and payments | 🟡 Partial | Core records and price/address snapshots exist; line items, full history, attempts/events/refunds, and minor-unit currency do not. | Phases 1–2 |
| Notifications and delivery attempts | 🟡 Partial | Notifications and latest driver fix persist; channel attempts and full delivery-task/history models do not. | Phases 2–3 |
| Audit events | ⬜ Not started | No immutable audit entity exists. | Phase 0 |
| PostgreSQL, constraints, encryption, retention, backups | ⬜ Not started | SQLite is appropriate only for local development; production data governance/operations are absent. | Phase 2 before pilot |
| UTC timestamps and integer money | 🟡 Partial | Some epochs/timestamps exist, but timezone treatment is inconsistent and money remains floating point. | Phase 1 migration |

### Section 9 — Non-functional requirements

| Requirement area | Status | Why | When |
|---|---|---|---|
| Security | 🟡 Partial | JWT checks, output escaping, payment signatures, internal/driver secrets, and no raw-card storage exist; plaintext password handling, missing RBAC, permissive CORS, secret management, rate limits, and audit gaps block production. | Immediate Phase 0 |
| Reliability | 🟡 Partial | Idempotent payment flow, database busy handling, polling fallback, bounded 2-second connect/10-second gateway calls, status propagation, and explicit failures exist; circuit breakers, reconciliation, queues, backups, rollback, and incident procedures do not. | Phases 0–2 |
| Performance targets | 🟡 Partial | Concurrency/load harnesses and bounded payment fetching exist; current claims are not production capacity evidence and no repeatable environment measures every target. | Phase 0 baseline; Phase 2 pilot verification |
| Accessibility/browser compatibility | 🟡 Partial | Responsive and semantic foundations exist; WCAG 2.2 AA audit, reduced-motion/high-contrast coverage, and browser matrix tests remain. | Phases 1–2 |
| Observability | 🟡 Partial | C++ flow logs and local log files exist; structured correlation logs, metrics, traces, dashboards, and alerts are absent. | Phase 2 |

### Section 10 — Testing requirements

| Requirement | Status | Why | When |
|---|---|---|---|
| Focused unit tests | 🟡 Partial | Delivery quote, payment/order transition, and gateway HTTP status/failure mapping tests exist; coverage is not comprehensive. | Phase 0, continuous |
| API/E2E tests | 🟡 Partial | Dependency-free local E2E covers major auth/order/payment/tracking/address failures and successes; it is not a complete contract suite. | Phase 0, continuous |
| Repository/migration tests | ⬜ Not started | Isolated database fixtures and versioned migrations do not exist. | Phase 0 |
| Browser tests | ⬜ Not started | Critical customer/operator paths are not automated in a browser runner. | Phase 1 |
| Load tests | 🟡 Partial | A 1,000-client development harness exists; results depend on services/environment and are not a production capacity guarantee. | Phase 0 baseline; Phase 2 sizing |
| Security tests | 🟡 Partial | JWT, forged signature, unsigned webhook, and tampering cases exist; OWASP/RBAC/rate-limit scanning is incomplete. | Phase 0 |
| CI execution | ✅ Done | GitHub Actions runs source checks, a clean CMake/vcpkg build, CTest, and six-service E2E tests without real payment credentials. | Completed |

### Section 11 — Delivery roadmap status

| Phase | Status | Why / exit requirement | When |
|---|---|---|---|
| Phase 0 — Stabilize | 🟡 In progress | JWT identity, payment/order sync, CI, and bounded gateway status propagation are done; password security, RBAC, migrations, versioning, correlation IDs, and complete API consistency remain. | Current phase |
| Phase 1 — Ordering MVP | 🟡 Started | Address/serviceability is implemented; authoritative catalogue/cart/pricing and restaurant/admin portals remain. | After Phase 0 exit |
| Phase 2 — Single-city pilot | ⬜ Not started | Requires stable ordering plus provider/business decisions, production data platform, operations, refunds, notifications, and monitoring. | After Phase 1 exit |
| Phase 3 — Growth | 🟡 Prototype only | Real browser GPS is an early prototype; production delivery operations and growth domains remain. | After pilot stability |
| Phase 4 — Scale | ⬜ Not started | Multi-city/HA/fraud/data work is premature before pilot evidence. | After product-market and capacity evidence |

### Section 12 — Marketplace MVP acceptance criteria

| # | Acceptance criterion | Status | Why / when |
|---:|---|---|---|
| 1 | Verified customer signs in without manual user ID | 🟡 Partial | No manual ID and JWT identity work; email/phone verification and secure password/session lifecycle remain for Phase 0–1. |
| 2 | Valid address and serviceable restaurants | 🟡 Partial | Structured address and server enforcement work; card browsing needs authoritative polygon/hours evaluation in Phase 1. |
| 3 | Available itemized restaurant menu | ⬜ Not started | Phase 1 catalogue. |
| 4 | Cart with server-calculated breakdown | ⬜ Not started | Phase 1 cart/pricing engine. |
| 5 | Exactly one order/payment attempt | 🟡 Partial | Payment is idempotent; order creation is not yet idempotent. Phase 0. |
| 6 | Sandbox success/failure/cancel/timeout | 🟡 Partial | Success/failure/cancel work; provider timeout/reconciliation remains for Phase 2. |
| 7 | Consistent live payment/order status | ✅ Done | Authenticated state mapping plus UI status refresh are implemented locally. |
| 8 | Restaurant preparation workflow | ⬜ Not started | Phase 1 restaurant portal/state workflow. |
| 9 | Complete timeline and notifications | 🟡 Partial | Current states/notifications exist; complete append-only history and channels remain for Phases 1–2. |
| 10 | Admin transaction audit | ⬜ Not started | Phase 1–2 admin/audit portal. |
| 11 | Cross-user/restaurant authorization | 🟡 Partial | Customer ownership works on core flows; restaurant/admin RBAC remains Phase 0. |
| 12 | Critical CI tests without charges | ✅ Done | Clean GitHub Actions builds and tests the service flow using only test/dummy credentials. |
| 13 | Logs, metrics, backups, recovery | ⬜ Not started | Phase 2 operational readiness. |

The Marketplace MVP is therefore **not complete**. The next blocking work is
Phase 0 security/build/API stabilization, followed by the Phase 1 authoritative
menu, cart, pricing, and restaurant workflow.

### Sections 13–15 — Decisions, resume process, and delivery summary

| Area | Status | Why | When |
|---|---|---|---|
| Launch/business/provider decisions | ⬜ Decision required | Launch model/city, legal/tax rules, maps, messages, hosting, commercial terms, retention, and final provider choices change the architecture and operating cost. | Before committing Phase 2 production integrations |
| Razorpay provider direction | 🟡 Partial decision | Razorpay test mode is integrated; live activation, compliance, refunds, and commercial approval remain. | Phase 2 |
| Resume checklist | ✅ Done | The repository contains a reusable continuation prompt and source-of-truth document list. | Now / completed |
| Delivery address/serviceability summary | ✅ Done for development MVP | Local requirements are implemented and separated from production follow-up. | Production completion in Phase 2 |
