# Architecture and design

For request-by-request frontend and backend interactions, see
[End-to-end sequence diagrams](SEQUENCE_DIAGRAMS.md).

FoodService is a C++20/Crow microservice application. Each domain service owns
its HTTP controller, business service, repository, and SQLite persistence. The
API Gateway is the browser-facing façade.

```text
Browser/UI -> API Gateway :8085
                 |-> User Service :8080 -> SQLite
                 |-> Restaurant Service :8081 -> SQLite
                 `-> Order Service :8082 -> SQLite
                                      |-> Restaurant Service
                                      `-> Payment Service :8083 -> SQLite
                                               `-> Notification Service :8084 -> SQLite
```

## Request layers

1. Crow route parses the request and calls a controller.
2. Controller validates JSON and translates the result to HTTP.
3. Service implements business rules and cross-service orchestration.
4. Repository executes SQL through the shared database wrapper.
5. `common/` supplies configuration, logging, validation, password hashing,
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

- Services use fixed localhost ports and direct HTTP discovery.
- SQLite databases are service-local files; there is no distributed transaction.
- Order/payment/notification updates therefore use eventual consistency.
- The gateway currently exposes users, restaurants, and orders; operational
  payment/notification APIs may remain direct-service endpoints.
