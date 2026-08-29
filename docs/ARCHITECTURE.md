# Architecture and design

## Correlated synchronous request flow

The API Gateway middleware chooses one safe `X-Correlation-ID` per incoming
request and stores it only for the lifetime of the Crow worker thread. The HTTP
client adds it to calls made to downstream services. The gateway returns the
same ID to the browser and logs request start/finish records. It clears the
thread-local value after responding so a reused worker cannot leak an old ID.

```text
Browser -- X-Correlation-ID --> API Gateway middleware
                                  |-- same ID --> downstream service
                                  |<-- response --|
Browser <-- same ID ------------ API Gateway + correlated logs
```

## Timed delivery state machine

The gateway stores both the latest GPS timestamp and a separate status-change
timestamp. Frequent GPS fixes therefore do not restart the transition timer.
The ingestion route accepts only the next lifecycle state after the configured
30-second development delay and then asks Order Service to persist it. This is
an API invariant; hiding controls in the browser is only an additional UX rule.
Delivered orders cannot be cancelled through the gateway.

## Cross-service request logs

All six C++ processes use the shared `RequestLoggingMiddleware`. Successful,
client-failure, and server-failure responses are classified as INFO, WARNING,
and ERROR respectively, with request duration and correlation ID. DEBUG arrival
records are opt-in through `FOODSERVICE_LOG_LEVEL=DEBUG`. See
[Runtime logging](LOGGING.md) for safe fields and live log commands.

## Payment-gated delivery invariant

The browser opens `frontend/payment.html` as a separate checkout surface. It
shares non-sensitive progress through same-origin local storage, while
`payment.db` remains authoritative. Payment Service validates Razorpay's HMAC
signature before storing `succeeded`. API Gateway and Order Service both
require that state before starting delivery or accepting `ASSIGNED` and later
transitions. No driver identity, coordinates, or timer is created for an unpaid
order.

Updated 2026-08-17 for real delivery-partner browser GPS ingestion.

For request-by-request frontend and backend interactions, see
[End-to-end sequence diagrams](SEQUENCE_DIAGRAMS.md).
For the worker pools, SQLite serialization, 1,000-client sequence, and load-test
verification flow, see [High-concurrency order processing](CONCURRENCY_DIAGRAMS.md).

FoodService is a C++20/Crow microservice application. Each domain service owns
its HTTP controller, business service, repository, and SQLite persistence. The
API Gateway is the browser-facing façade.

Gateway-to-service transport is currently synchronous: the Crow worker handling
a request waits for libcurl to finish. Every method uses a 2-second connection
timeout and 10-second total timeout, so a dependency cannot hold that worker
indefinitely. `HttpResult` carries the downstream status/body/failure category;
the gateway preserves valid downstream HTTP status, maps a timeout to `504`, and
maps connection/other transport failures to `502`. Other Crow workers can still
serve requests, but this is bounded blocking rather than asynchronous I/O.

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
and durable real driver GPS ingestion.

The public Gateway order-list route requires a valid JWT and filters the Order
Service response to the JWT customer ID. The browser also resolves payment
status with at most eight concurrent lookups. This prevents another customer's
orders from appearing and prevents a large history from exhausting the browser
connection queue while checkout is waiting for its response.

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
authenticated test/provider callbacks; it never charges a card. After Payment
Service durably applies an allowed provider transition, it calls Order Service's
authenticated internal `/orders/{id}/payment-status` endpoint. Order Service owns
the mapping: `processing -> PAYMENT_PENDING`, `succeeded -> CONFIRMED`,
`failed -> PAYMENT_FAILED`, and `cancelled -> CANCELLED`. Duplicate callbacks are
idempotent, and payment callbacks never regress confirmed/delivery states.

`ORDER_SYNC_SECRET` authenticates the local service callback and
`ORDER_SERVICE_URL` configures its destination. If payment persistence succeeds
but Order Service cannot be updated, Payment Service returns HTTP 502 and asks the
provider/test caller to retry the same event safely. This is an explicit failure,
but it is not yet durable delivery: a production design still needs provider
event IDs, an outbox/message broker, retry storage, and reconciliation. Clients
display pending state and use the SSE snapshot or order-payment polling until
terminal.

Do not trust an amount, currency, user, or success status supplied by a browser.
Resolve the authoritative order server-side, validate webhook signatures over
the raw body, keep secrets in environment variables, use HTTPS outside local
development, and never store raw card data.

## Known boundaries

### Real driver browser GPS

The driver opens `frontend/driver.html`, enters the assigned order and driver
token, grants precise browser location permission, and starts `watchPosition`.
Each real device fix is posted to API Gateway and persisted in `delivery.db`.
The customer tracking route verifies JWT ownership and successful payment,
returns the stored coordinates, reports fixes older than 30 seconds as stale,
and calculates progress/ETA from geographic distance and reported speed. It
never invents a position when no driver fix exists.

The local driver token is a limited MVP trust boundary, not production driver
identity. Production still requires a Delivery Service with driver accounts,
per-assignment authorization, dispatch, encrypted transport, consent and
retention controls, background mobile location support, routing, audit events,
and automatic token revocation.

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
- Payment-to-order updates use an authenticated synchronous callback with safe
  retry behavior; there is still no durable outbox or distributed transaction.
- Notification updates remain best-effort/eventually consistent.
- The gateway exposes user, restaurant, order, payment, webhook, discovery, and
  tracking routes. Notification CRUD remains a direct-service operation.
# Delivery serviceability design

The browser provides GPS/manual/map inputs, but it is not the authority for delivery eligibility or pricing. API Gateway stores JWT-owned address rows in its local delivery database for this MVP. It requests the restaurant configuration, applies polygon-or-radius serviceability, computes distance/ETA/fees, and re-runs the same decision during order creation. This prevents a modified browser request from bypassing the delivery zone or lowering the fee.

Rule order: polygon when at least three configured points exist, otherwise radius; base plus per-kilometre fee; then surge (1.25), rain (1.15), and late-night (1.20) multipliers. ETA is preparation time plus distance travel time and operational buffers.

This is a development architecture. Production should use a profile/address service, encrypted personal data, PostGIS/geospatial indexes, road-network routing, a trusted weather feed, an operations-driven surge service, timezone-aware restaurant schedules, observability, and cache/failover policies.
