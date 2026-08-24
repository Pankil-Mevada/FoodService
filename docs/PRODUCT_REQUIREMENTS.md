# FoodService product requirements

Updated 2026-08-24. Statuses describe the exact locally implemented and
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
- latest local E2E evidence: 21 passed, 0 failed, 0 skipped

### Gaps that block a real marketplace

- no menu, category, item, variation, or add-on data model
- current menu is a frontend test fixture; prices are not yet sourced from an authoritative server-side catalogue
- no cart or itemized order lines
- passwords are stored/compared/logged as plaintext; hashing utility is incomplete
- no roles or authorization rules for customer, restaurant, delivery, or admin
- public restaurant/order mutation routes are insufficiently protected
- API Gateway does not consistently preserve downstream HTTP status codes
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

#### P0

- register, sign in, sign out, and maintain an authenticated browser session
- access and refresh tokens with expiration and revocation
- email or phone verification
- forgot-password and reset-password flow
- customer profile with name, email, phone, and saved addresses
- role-based access control enforced at the API, not only in the UI
- frontend derives the customer ID from `/me` or token claims
- audit security-sensitive account events

#### P1

- social sign-in
- account deletion and personal-data export
- saved dietary preferences and allergens
- multiple devices and session management

### 5.2 Location and serviceability

#### P0

Current local MVP coverage:

- ✅ Customer can add, list, select, and delete a JWT-owned delivery address.
- ✅ Address contains label, recipient, phone, coordinates, address line, and delivery notes.
- ✅ Determine whether a restaurant serves the selected address using a radius or configured polygon.
- ✅ Prevent quote, checkout, and order creation when the address is outside the delivery zone.
- ✅ Estimate straight-line distance, delivery fee, and delivery time on the backend.
- ✅ Display straight-line progress and distance/speed-based ETA from real driver GPS.

Production requirements still open:

- 🟡 Move customer addresses from the gateway MVP database to an encrypted profile/address service with retention controls.
- 🟡 Replace straight-line distance and formula ETA with road-network routing and live traffic.
- ⬜ Add address verification, apartment/building metadata, and delivery-instruction moderation.

#### P1

- 🟡 Map-based address picker and Nominatim geocoding work locally; a contracted production provider and failure policy remain.
- ✅ Live location permission with manual-address fallback and user-triggered nearby discovery.
- ✅ Configurable restaurant delivery radius or polygon.
- 🟡 Distance, simulated surge/rain, and server-clock late-night rules work; trusted weather/demand signals and restaurant-timezone schedules remain.

### 5.3 Restaurant discovery

#### P0

- browse only active and currently serviceable restaurants
- restaurant card shows name, cuisine, rating, ETA, price band, and availability
- search by restaurant, dish, or cuisine
- filter by cuisine, vegetarian, rating, delivery time, price, and offers
- restaurant detail page with operating hours, address, policies, and menu
- clear closed, busy, unavailable, and temporarily paused states

#### P1

- personalized recommendations
- collections such as trending, healthy, budget, and late-night
- sorting by relevance, rating, time, cost, and popularity
- favourites and recently viewed restaurants

### 5.4 Restaurant onboarding and management

#### P0

- admin approval before a restaurant becomes publicly visible
- restaurant owner can create and update business information
- business hours, holidays, service zones, preparation time, and order capacity
- upload logo, cover image, food images, and verification documents
- pause/resume ordering and mark individual items unavailable
- restaurant users can only modify their assigned restaurant

#### P1

- multiple outlets under one restaurant brand
- staff accounts and permissions
- scheduled menus and time-based item availability
- restaurant settlement and performance reports

### 5.5 Menu and catalogue

#### P0

- menu categories with explicit ordering
- items with name, description, image, base price, tax category, and food type
- vegetarian/non-vegetarian/vegan indicators
- item availability and stock status
- item variations such as size and portion
- add-on groups such as toppings and extras with min/max selection rules
- prices stored as integer minor units, never floating point
- server calculates all prices; client totals are never authoritative

#### P1

- combos, meal bundles, scheduled pricing, and inventory quantities
- allergen and nutritional information
- catalogue import/export

### 5.6 Cart and checkout

#### P0

- add, edit, and remove item quantities
- enforce one restaurant per cart
- persist cart for signed-in customers
- item-level preparation notes
- select delivery address and payment method
- display item subtotal, tax, packaging fee, delivery fee, discount, and final total
- validate current prices and availability before placing the order
- show clear recovery choices when price or availability changes
- require explicit final confirmation before order creation

#### P1

- scheduled orders
- group ordering
- tips for delivery partners
- reusable checkout preferences

### 5.7 Orders

#### P0

- itemized immutable order snapshot, including item names and prices at purchase
- customer sees only their own orders
- restaurant sees only orders for its restaurant
- order state machine:

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

- explicit allowed transitions with actor and timestamp
- order detail includes items, totals, address snapshot, payment, and timeline
- restaurant accepts or rejects within a configurable time
- customer cancellation policy based on current state
- unique public order reference separate from internal database ID
- idempotent order submission

#### P1

- reorder a previous order
- substitutions and restaurant/customer clarification
- proof of delivery and delivery PIN
- downloadable invoice

### 5.8 Payments and refunds

- [x] dedicated responsive payment page
- [x] main-board in-progress, failure/retry, and verified states
- [x] backend-verified payment required before driver assignment/tracking
- [x] safe recovery from refresh, provider dismissal, popup blocking, and network errors

#### P0

- [x] integrate Razorpay Standard Checkout in sandbox mode with dummy fallback
- [x] create Razorpay orders only on the backend
- never store raw card or UPI credentials
- [x] verify Razorpay checkout signatures server-side with HMAC-SHA256/OpenSSL
- idempotent payment creation and webhook handling
- webhook event deduplication and durable event log
- [x] successful/failed/cancelled payment callbacks update both `payment` and `order` through the authenticated local synchronization endpoint
- [x] payment states map to `PAYMENT_PENDING`, `CONFIRMED`, `PAYMENT_FAILED`, and `CANCELLED` without regressing later delivery states
- [x] support local test success, failure, cancellation, and duplicate callback cases; provider timeout/reconciliation remains open
- cash-on-delivery is represented as a separate method without fake online payment
- refund creation, status tracking, and webhook reconciliation
- all amounts use currency plus integer minor units

#### P1

- saved provider payment tokens
- partial and multiple refunds
- restaurant settlement ledger
- reconciliation report and mismatch alerts

### 5.9 Live status and notifications

#### P0

- durable order/payment event history
- live browser updates through authenticated SSE or WebSocket
- reconnect from the last received event without losing updates
- in-app notifications for order and payment transitions
- email/SMS/push provider abstraction
- retry failed deliveries with backoff and dead-letter handling
- customer notification preferences

#### P1

- restaurant new-order alert with acknowledgement
- delivery-partner task notifications
- customer/restaurant/support chat with abuse controls

### 5.10 Delivery operations

#### P0 for delivery launch; optional for restaurant-pickup MVP

Current local MVP coverage:

- [x] authenticated local driver portal using real browser `watchPosition` GPS
- [x] durable latest driver fix, accuracy/speed/heading, 30-second stale detection, and distance-based ETA
- [x] persist final `DELIVERED` state in `order.db`

Production requirements still open:

- delivery-partner onboarding and approval
- online/offline availability
- assign a delivery task manually or using a simple dispatch rule
- delivery state transitions and timestamps
- pickup and delivery verification
- customer sees coarse live status without exposing unnecessary personal data

#### P1

- production location consent, retention/deletion jobs, and per-driver authorization
- route/ETA integration
- automatic dispatch and reassignment
- earnings and payout ledger

### 5.11 Ratings and reviews

#### P1

- only customers with delivered orders can review
- separate food, restaurant, and delivery ratings
- one review per order with an edit window
- restaurant responses
- report, moderation, and anti-abuse workflow
- aggregate ratings recalculated from approved reviews

### 5.12 Offers and promotions

#### P1

- coupons with validity, usage limits, minimum order, restaurant, and user rules
- restaurant-funded and platform-funded discounts
- deterministic server-side promotion calculation
- referral credits and promotional wallet ledger
- prevent stacking or abuse according to configured rules

### 5.13 Administration and support

#### P0

- protected admin portal
- approve/suspend restaurants and users
- search customers, restaurants, orders, payments, and notifications
- view a complete order/payment event timeline
- cancel orders and initiate test/refund operations with audit reason
- configure service zones, fees, taxes, and platform settings
- immutable audit log for privileged actions

#### P1

- support tickets and internal notes
- content moderation
- operational dashboard for order volume, failures, and SLA breaches
- CSV exports with access controls

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
- preserve downstream HTTP status, headers, and structured errors at the gateway
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
- timeouts and circuit breaking on service calls
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

- fix gateway status propagation and standardize errors
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
| `/api/v1`, status/error consistency, correlation IDs | ⬜ Not started | Routes are unversioned and proxy/error semantics are inconsistent. | Next — Phase 0 |
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
| Reliability | 🟡 Partial | Idempotent payment flow, database busy handling, polling fallback, and explicit UI failures exist; circuit breakers, reconciliation, queues, backups, rollback, and incident procedures do not. | Phases 0–2 |
| Performance targets | 🟡 Partial | Concurrency/load harnesses and bounded payment fetching exist; current claims are not production capacity evidence and no repeatable environment measures every target. | Phase 0 baseline; Phase 2 pilot verification |
| Accessibility/browser compatibility | 🟡 Partial | Responsive and semantic foundations exist; WCAG 2.2 AA audit, reduced-motion/high-contrast coverage, and browser matrix tests remain. | Phases 1–2 |
| Observability | 🟡 Partial | C++ flow logs and local log files exist; structured correlation logs, metrics, traces, dashboards, and alerts are absent. | Phase 2 |

### Section 10 — Testing requirements

| Requirement | Status | Why | When |
|---|---|---|---|
| Focused unit tests | 🟡 Partial | Delivery quote and payment/order transition tests exist; coverage is not comprehensive. | Phase 0, continuous |
| API/E2E tests | 🟡 Partial | Dependency-free local E2E covers major auth/order/payment/tracking/address failures and successes; it is not a complete contract suite. | Phase 0, continuous |
| Repository/migration tests | ⬜ Not started | Isolated database fixtures and versioned migrations do not exist. | Phase 0 |
| Browser tests | ⬜ Not started | Critical customer/operator paths are not automated in a browser runner. | Phase 1 |
| Load tests | 🟡 Partial | A 1,000-client development harness exists; results depend on services/environment and are not a production capacity guarantee. | Phase 0 baseline; Phase 2 sizing |
| Security tests | 🟡 Partial | JWT, forged signature, unsigned webhook, and tampering cases exist; OWASP/RBAC/rate-limit scanning is incomplete. | Phase 0 |
| CI execution | ⬜ Not started | Tests are not enforced by a clean GitHub Actions build. | Next — Phase 0 |

### Section 11 — Delivery roadmap status

| Phase | Status | Why / exit requirement | When |
|---|---|---|---|
| Phase 0 — Stabilize | 🟡 In progress | JWT identity and payment/order sync are done; password security, RBAC, migrations, CI, API consistency, and repeatable clean build remain. | Current phase |
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
| 12 | Critical CI tests without charges | 🟡 Partial | Local tests avoid real charges; clean CI enforcement remains Phase 0. |
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
