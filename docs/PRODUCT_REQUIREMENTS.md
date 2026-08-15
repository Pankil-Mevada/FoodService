# FoodService product requirements

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
- notification persistence
- responsive Plated customer frontend
- automated end-to-end test harness and dummy-payment script

### Gaps that block a real marketplace

- no menu, category, item, variation, or add-on data model
- no cart or itemized order lines
- frontend asks for a numeric user ID instead of using the logged-in identity
- no roles or authorization rules for customer, restaurant, delivery, or admin
- public restaurant/order mutation routes are insufficiently protected
- payment completion does not update the corresponding order status
- API Gateway does not consistently preserve downstream HTTP status codes
- SSE is reconnect-based status sampling rather than a durable event stream
- no delivery address, serviceability, tax, fee, coupon, or pricing engine
- no restaurant, delivery-partner, or administrator web portal
- no production payment-provider integration
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

- customer can add and select a delivery address
- address contains label, recipient, phone, coordinates, and delivery notes
- determine whether a restaurant serves the selected address
- prevent checkout when an address is outside the delivery zone
- estimate delivery time and distance

#### P1

- map-based address picker and geocoding
- [x] live location permission with manual-address fallback and user-triggered nearby discovery (development MVP)
- configurable restaurant delivery radius or polygon
- surge, rain, late-night, and distance-based delivery rules

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

#### P0

- integrate one provider in sandbox mode first, such as Razorpay or Stripe
- create provider payment intents only on the backend
- never store raw card or UPI credentials
- provider-signature verification using the official SDK
- idempotent payment creation and webhook handling
- webhook event deduplication and durable event log
- successful/failed payment updates both `payment` and `order` consistently
- payment states map clearly to order states
- support test success, failure, cancellation, timeout, and duplicate webhook cases
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

- delivery-partner onboarding and approval
- online/offline availability
- assign a delivery task manually or using a simple dispatch rule
- delivery state transitions and timestamps
- pickup and delivery verification
- customer sees coarse live status without exposing unnecessary personal data

#### P1

- location updates with consent and retention limits
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
- password hashing with Argon2 using reviewed parameters
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
- make payment webhooks update order status consistently
- add database migrations, unit tests, and CI
- create repeatable one-command local startup

### Phase 1 — Ordering MVP

- [x] addresses and serviceability (MVP: browser coordinates, persisted order destination, radius validation)
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

- delivery-partner workflow and tracking
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
