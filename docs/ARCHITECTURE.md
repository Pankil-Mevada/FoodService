# Architecture and design

Updated 2026-08-15 for the current local MVP and three-minute delivery simulator.

For request-by-request frontend and backend interactions, see
[End-to-end sequence diagrams](SEQUENCE_DIAGRAMS.md).

FoodService is a C++20/Crow microservice application. Each domain service owns
its HTTP controller, business service, repository, and SQLite persistence. The
API Gateway is the browser-facing façade.

```text
Browser/UI -> API Gateway :8085
                 |-> OpenStreetMap Overpass (user-triggered discovery)
                 |-> User Service :8080 -> SQLite
                 |-> Restaurant Service :8081 -> SQLite
                 `-> Order Service :8082 -> SQLite
                                      |-> Restaurant Service
                                      `-> Payment Service :8083 -> SQLite
                                               `-> Notification Service :8084 -> SQLite
```

The browser stores its JWT, API settings, selected coordinates, and address in
local storage. Restaurant Service persists discovered coordinates and delivery
radius. Order Service persists the destination and final delivery state. API
Gateway handles JWT identity injection, serviceability, provider orchestration,
and the local delivery simulation.

The current profile enhancement also stores a resized avatar, display name,
phone, test UPI ID, and favourites in browser local storage. It never captures a
UPI PIN. These fields are presentation/test conveniences, not synchronized or
bank-verified account data; production requires authenticated profile storage,
object storage for photos, consent/retention controls, and payment-provider tokenization.

## Request layers

1. Crow route parses the request and calls a controller.
2. Controller validates JSON and translates the result to HTTP.
3. Service implements business rules and cross-service orchestration.
4. Repository executes SQL through the shared database wrapper.
5. `common/` supplies configuration, logging, validation, an unfinished password-hashing utility,
   JWT, middleware, HTTP responses, and database facilities.

## Payment consistency model

An external payment provider cannot be treated as a single synchronous database
call. The implemented safe test flow creates a local pending intent and accepts
authenticated test/provider callbacks; it never charges a card. A production
adapter would create a provider intent, verify official webhook signatures,
update the local payment idempotently, update the order, and emit a notification.
Provider event IDs should be stored or otherwise deduplicated. Clients display
pending state and use the SSE snapshot or order-payment polling until terminal.

Do not trust an amount, currency, user, or success status supplied by a browser.
Resolve the authoritative order server-side, validate webhook signatures over
the raw body, keep secrets in environment variables, use HTTPS outside local
development, and never store raw card data.

## Known boundaries

### Local delivery simulation

API Gateway owns the in-memory three-minute simulation clock. It verifies JWT
order ownership, selects a stable test driver profile from the order ID,
interpolates coordinates between restaurant and destination, and calls Order
Service's internal status endpoint. Order Service persists lifecycle state in
`order.db`; once `DELIVERED` is stored, gateway restarts cannot revert it.

Before production, a Delivery Service must own driver authentication,
assignment, consented location ingestion, durable events, dispatch rules, and
retention. Current driver/contact/vehicle values and the map are simulated.

### Nearby restaurant discovery

Discovery runs only after the customer chooses GPS or Bengaluru demo. API
Gateway sends the selected coordinates to configurable Overpass, caps results
at 20, deduplicates by name, imports coordinates into Restaurant Service, and
returns city/provider metadata. The UI calculates display distance; API Gateway
independently enforces delivery radius at checkout. Saved restaurants remain
visible when the provider is unavailable.

Restaurant records also persist an optional `imageUrl`. Discovery accepts only
HTTP(S) `image` values from OpenStreetMap or builds a Wikimedia Commons redirect
from `wikimedia_commons`. The frontend displays the sourced photograph with lazy
loading and shows “Photo unavailable” when no attributable image exists; it does
not present stock artwork as a photograph of that restaurant.

For local checkout testing, the frontend ships three generated illustrative food
photos and a fixed menu. It labels those images as sample menu photos. The chosen
item summary and price breakdown are persisted with the order, while Order
Service verifies the arithmetic. This is not an authoritative menu/pricing
architecture: production requires restaurant-owned menu records and server-side
price, promotion, tax, and availability calculation.

The public endpoint is for low-volume development. Production requires a
contracted or self-hosted provider, caching, rate-limit handling, privacy
review, and proper geocoding/routing.

### Current security debt

User Service currently persists and compares passwords as plaintext and writes
password values to logs. `PasswordHasher` is incomplete and unused. Only dummy
credentials may be used until Argon2 hashing, migration, and log removal land.
JWT also uses a hard-coded development secret. Authorization remains incomplete
beyond customer identity and tracking ownership checks.

- Services use fixed localhost ports and direct HTTP discovery.
- SQLite databases are service-local files; there is no distributed transaction.
- Order/payment/notification updates therefore use eventual consistency.
- The gateway exposes user, restaurant, order, payment, webhook, discovery, and
  tracking routes. Notification CRUD remains a direct-service operation.
