# FoodService System-Design Questions, Answers, and Code

This workbook is designed for system-design and backend interviews. Every answer
uses a real FoodService code excerpt, explains its runtime behavior, and states
the trade-off or production improvement an interviewer may ask about.

Snippets are intentionally shortened. Use the linked source file for complete
error handling and surrounding logic.

## 1. How did you divide FoodService into microservices?

### Interview answer

I divided the system by business capability: users own identity, restaurants own
the catalogue, orders own the purchase lifecycle, payments own provider state,
and notifications own messages. The browser calls an API Gateway rather than
calling every service directly.

### Code

Each service is a separate Crow process. Payment Service, for example, selects a
runtime port and starts its own worker pool:

```cpp
// services/PaymentService/src/main.cpp
const char* portValue = std::getenv("PAYMENT_SERVICE_PORT");
const unsigned short port = portValue
    ? static_cast<unsigned short>(std::stoi(portValue))
    : 8083;

app.port(port)
   .concurrency(128)
   .run();
```

Notification Service independently runs on port 8084, Order Service on 8082,
Restaurant Service on 8081, User Service on 8080, and the Gateway on 8085.

### Explanation

Each executable has independent routes and a service-owned SQLite database. This
creates process and data boundaries. A payment crash does not directly corrupt
the restaurant catalogue process.

### Follow-up: why not a modular monolith?

For a small team, a modular monolith would be simpler to deploy and transact
across. Microservices are justified when independent scaling, security, failure
isolation, or team ownership outweigh distributed-system complexity.

## 2. What does the API Gateway do?

### Interview answer

The Gateway provides one public API, hides internal topology, validates JWTs,
injects trusted identity, forwards requests, and composes tracking data.

### Code

```cpp
// services/ApiGateway/src/main.cpp
CROW_ROUTE(app, "/payments").methods(crow::HTTPMethod::POST)
([&paymentClient](const crow::request& req) {
    const auto userId = authenticatedUserId(req);
    if (!userId) return unauthorized();

    const auto input = crow::json::load(req.body);
    if (!input || !input.has("orderId") ||
        !input.has("amount") || !input.has("paymentMethod"))
        return crow::response(400, "Missing payment fields");

    crow::json::wvalue body;
    body["userId"] = *userId;
    body["orderId"] = input["orderId"].i();
    body["amount"] = input["amount"].d();
    body["paymentMethod"] = input["paymentMethod"].s();

    return crow::response(paymentClient.createPayment(
        body.dump(), req.get_header_value("Idempotency-Key")));
});
```

### Explanation

The Gateway does not copy `userId` from the browser. It replaces it with the ID
from a verified token, then calls Payment Service through
`services/ApiGateway/src/client/PaymentClient.cpp`.

### Trade-off

The Gateway is another network hop and possible bottleneck. Keep domain rules in
services, operate multiple stateless Gateway replicas, and put them behind a load
balancer.

## 3. How do authentication and authorization differ here?

### Interview answer

Authentication proves who the caller is by verifying the JWT. Authorization
checks whether that identity may access a specific order or operation.

### Code: token creation

```cpp
// common/src/JwtManager.cpp
auto token = jwt::create()
    .set_issuer("FoodService")
    .set_subject(email)
    .set_payload_claim("userId", jwt::claim(std::to_string(userId)))
    .set_issued_at(system_clock::now())
    .set_expires_at(system_clock::now() + hours{24})
    .sign(jwt::algorithm::hs256{m_secret});
```

### Code: resource authorization

```cpp
// services/ApiGateway/src/main.cpp
const auto userId = authenticatedUserId(req);
if (!userId) return unauthorized();

const auto order = crow::json::load(client.getOrderById(id));
if (!order || !order.has("id"))
    return jsonError(404, "Order not found");
if (order["userId"].i() != *userId)
    return jsonError(403, "This order belongs to another customer");
```

### Explanation

A valid token is insufficient by itself. The second check prevents an
authenticated customer from changing the URL to another order ID.

### Production improvement

The repository currently uses a hard-coded development JWT secret. Production
must inject and rotate a strong secret or use asymmetric keys with a managed
identity provider. Authorization should be consistently applied to all public
resource reads and mutations.

## 4. How is the code separated into layers?

### Interview answer

Routes handle transport, controllers validate HTTP input, services enforce
business rules, and repositories run SQL. This avoids mixing JSON, payment rules,
and database code in one function.

### Code

```cpp
// services/PaymentService/src/main.cpp: transport layer
CROW_ROUTE(app, "/payments")
.methods(crow::HTTPMethod::POST)
([&controller](const crow::request& req) {
    return controller.createPayment(req);
});

// PaymentController.cpp: validation and mapping
std::string key = req.get_header_value("Idempotency-Key");
auto payment = m_service.createPayment(
    orderId, userId, amount, method, key);

// PaymentService.cpp: business behavior
auto existing = m_repository.getPaymentByIdempotencyKey(idempotencyKey);
if (existing) return existing;

// PaymentRepository.cpp: persistence
return findPayment(m_database,
    "SELECT ... FROM payments WHERE idempotency_key=?;", key);
```

### Explanation

Each layer has one reason to change. A future REST-to-gRPC migration mostly
affects transport/controller code; a database migration mostly affects the
repository.

### Trade-off

Layers add files and mapping code. For very small features, excessive layering
can become ceremony, but payment and order logic benefit from explicit boundaries.

## 5. How do you prevent duplicate payments?

### Interview answer

I use an idempotency key tied to the business operation. A retry first returns
the existing payment, and a unique database constraint provides the final
concurrency-safe guarantee.

### Code: stable key from the order

```cpp
// services/OrderService/src/client/PaymentClient.cpp
const std::string idempotencyHeader =
    "Idempotency-Key: order-" + std::to_string(orderId);
headers = curl_slist_append(headers, idempotencyHeader.c_str());
```

### Code: reuse existing result

```cpp
// services/PaymentService/src/PaymentService.cpp
if (!idempotencyKey.empty()) {
    auto existing =
        m_repository.getPaymentByIdempotencyKey(idempotencyKey);
    if (existing) return existing;
}
```

### Code: database-level guarantee

```sql
-- created by common/src/Database.cpp
CREATE UNIQUE INDEX IF NOT EXISTS idx_payments_idempotency_key
ON payments(idempotency_key)
WHERE idempotency_key IS NOT NULL;
```

### Explanation

The initial lookup improves retry performance. The unique index matters because
two simultaneous requests could both see “not found” before either inserts.

### Follow-up: what should a complete implementation store?

Store the caller/key, request fingerprint, operation state, HTTP response, and
expiration. Reusing one key with a different payload should be rejected.

## 6. How does the application handle many clients concurrently?

### Interview answer

Crow uses bounded worker pools. Gateway, Order, and Payment services configure 128
workers, while lighter services use fewer or Crow's multithreaded default.

### Code

```cpp
// services/ApiGateway/src/main.cpp
app.loglevel(crow::LogLevel::Warning)
   .port(8085)
   .concurrency(128)
   .run();
```

### Explanation

Different connections can run on different workers. This provides concurrency,
not unlimited throughput. A worker blocked in SQLite, libcurl, or Razorpay remains
occupied until that call completes.

### Follow-up: why not 10,000 threads?

Threads consume stack memory and scheduler time. Production capacity should use
bounded pools, async I/O where useful, connection pools, downstream deadlines,
backpressure, and horizontal replicas. Worker count must be load-tested.

## 7. How is SQLite protected from concurrent access?

### Interview answer

The shared database wrapper enables WAL and a busy timeout. Compound connection
operations are protected by a recursive mutex using RAII locks.

### Code: connection configuration

```cpp
// common/src/Database.cpp
sqlite3_busy_timeout(db, 30000);
execute("PRAGMA journal_mode=WAL;");
execute("PRAGMA synchronous=NORMAL;");
execute("PRAGMA foreign_keys=ON;");
```

### Code: critical section

```cpp
// services/OrderService/src/OrderRepository.cpp
std::optional<int> OrderRepository::saveOrder(const Order& order) {
    // Keep INSERT and sqlite3_last_insert_rowid atomic.
    std::lock_guard<std::recursive_mutex> lock(m_database.mutex());
    // prepare, bind, step, then read inserted ID...
}
```

### Explanation

Without the lock, another worker could insert on the same connection between this
request's INSERT and `sqlite3_last_insert_rowid()`. RAII releases the mutex even
when the function returns early.

### Trade-off

WAL permits better reader/writer overlap, but SQLite still has a single-writer
limit. PostgreSQL/MySQL with pooled connections is the production path for heavy
write concurrency.

## 8. How do you model payment status safely?

### Interview answer

Payment status is a finite state machine. The service permits pending to move to
processing or a terminal state, processing to a terminal state, and rejects
transitions out of terminal states.

### Code

```cpp
// services/PaymentService/src/PaymentService.cpp
const auto& old = current->getStatus();
const bool allowed = old == status ||
    (old == "pending" &&
        (status == "processing" || status == "succeeded" ||
         status == "failed" || status == "cancelled")) ||
    (old == "processing" &&
        (status == "succeeded" || status == "failed" ||
         status == "cancelled"));

if (!allowed) return std::nullopt;
if (old != status &&
    !m_repository.updateStatus(transactionId, status, providerPaymentId))
    return std::nullopt;
```

### Explanation

This prevents a late duplicate webhook from changing `succeeded` back to
`processing` or `failed`. Repeating the same status is safe.

### Production improvement

Use optimistic concurrency/version columns, store every provider event under a
unique event ID, and maintain an append-only audit trail for reconciliation.

## 9. Why is Razorpay success verified on the backend?

### Interview answer

The browser is untrusted. Payment Service recalculates the HMAC using the secret
that exists only on the server and compares it in constant-time style.

### Code

```cpp
// services/PaymentService/src/RazorpayClient.cpp
const std::string data = orderId + "|" + paymentId;
const std::string secret = env("RAZORPAY_KEY_SECRET");

HMAC(EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
     reinterpret_cast<const unsigned char*>(data.data()), data.size(),
     digest, &length);

const bool verified = safeEqual(expected.str(), signature);
return verified;
```

The controller additionally checks that the provider order belongs to the local
transaction before accepting the signature:

```cpp
// services/PaymentService/src/PaymentController.cpp
if (!payment || payment->getProvider() != "razorpay" ||
    payment->getProviderPaymentId() != orderId)
    return crow::response(409, "Razorpay order does not match this payment");

if (!m_razorpay.verifyPaymentSignature(orderId, paymentId, signature))
    return crow::response(401, "Invalid Razorpay payment signature");
```

### Production improvement

Also consume signed webhooks, deduplicate provider event IDs, reject stale
events, query the provider during reconciliation, rotate secrets, and never log
credentials or full payment details.

## 10. How do real-time payment updates work?

### Interview answer

Payment Service returns an SSE-formatted snapshot and tells EventSource to retry
after two seconds. The frontend reconnects or falls back to polling.

### Code

```cpp
// services/PaymentService/src/PaymentController.cpp
response.set_header("Content-Type", "text/event-stream");
response.set_header("Cache-Control", "no-cache");
response.set_header("Connection", "keep-alive");
response.body =
    "retry: 2000\n"
    "event: payment-status\n"
    "data: " + paymentJson(*payment).dump() + "\n\n";
```

### Explanation

This repository closes after a snapshot, so it is reconnect-based sampling—not a
continuously held push stream. Polling provides graceful degradation when native
EventSource cannot be used.

### Production improvement

Publish payment events through a durable broker, run horizontally scalable SSE
or WebSocket fan-out servers, authorize subscriptions, and resume from event IDs.

## 11. How is live driver location implemented?

### Interview answer

The driver's browser watches real device GPS and sends authenticated location
updates. The Gateway stores the latest fix and the customer UI polls tracking,
checks freshness, and moves the bike marker.

### Code: real browser GPS

```javascript
// frontend/driver.js
watcher = navigator.geolocation.watchPosition(
  send,
  error => show(`GPS error: ${error.message}`),
  { enableHighAccuracy: true, maximumAge: 2000, timeout: 15000 }
);
```

### Code: protected upsert

```cpp
// services/ApiGateway/src/main.cpp
bool saveDriverLocation(Database& database, int orderId,
                        const DriverLocation& value) {
    std::lock_guard<std::recursive_mutex> lock(database.mutex());
    const char* sql =
        "INSERT INTO driver_locations(...) VALUES(...) "
        "ON CONFLICT(order_id) DO UPDATE SET "
        "latitude=excluded.latitude, longitude=excluded.longitude, "
        "updated_epoch=excluded.updated_epoch;";
    // prepare and bind the complete values...
}
```

### Code: move marker from server progress

```javascript
// frontend/app.js
const t = await request(config.paths.tracking(id));
$('#driver-pin').style.left =
  `${15 + Number(t.progressPercent) * .7}%`;
state.trackingTimer = setTimeout(refresh, 5000);
```

### Explanation

The gateway route requires `X-Driver-Token` for ingestion and JWT ownership for
customer reads. A timestamp lets the UI distinguish live from stale data.

### Production improvement

Use one short-lived token per driver/order, sequence numbers, rate limits,
geospatial storage, stream ingestion, retention controls, and push fan-out.

## 12. Where is backpressure used?

### Interview answer

The frontend bounds payment-history fan-out to eight concurrent requests instead
of launching one request for every order at once.

### Code

```javascript
// frontend/app.js
async function loadPaymentsBounded(orders, concurrency = 8) {
  let next = 0;
  const worker = async () => {
    while (next < orders.length) {
      const order = orders[next++];
      await loadPayment(order.id);
    }
  };
  await Promise.all(
    Array.from({ length: Math.min(concurrency, orders.length) }, worker)
  );
}
```

### Explanation

This limits outstanding work and avoids a browser-side request storm for a large
order history.

### Production improvement

The server still needs rate limiting, bounded queues, connection-pool limits,
request deadlines, load shedding, circuit breakers, and `Retry-After` responses.

## 13. How did you test 1,000 simultaneous orders?

### Interview answer

The load test creates 1,000 asyncio tasks, releases them together through an
event, measures latency and throughput, and verifies both persisted orders and
one unique payment record per order.

### Code: synchronized burst

```python
# tests/load_test.py
start = asyncio.Event()
tasks = [
    asyncio.create_task(
        post_order(args.host, args.port, token, item, start)
    )
    for item in payloads
]
wall_start = time.perf_counter()
start.set()
results = await asyncio.gather(*tasks)
```

### Code: correctness, not only HTTP 200

```python
summary["passed"] = all((
    http_success == args.orders,
    len(persisted) == args.orders,
    len(matching_payments) == args.orders,
    len(set(payment_order_ids)) == args.orders,
))
```

### Explanation

The test checks durable side effects and duplicate payments, not just response
codes. It records p50, p95, p99, maximum latency, throughput, and errors in
`tests/results/load-1000-latest.json`.

### Important interview qualification

This proves one local configuration completed one defined burst. It does not
prove that real Razorpay checkout, GPS, notifications, and databases can process
1,000 complete production journeys at once. Report hardware, configuration,
resource saturation, and repeatability with every capacity claim.

## 14. How do you handle a payment succeeding while Order Service is down?

### Honest current answer

The current services use synchronous HTTP and separate SQLite databases. There is
no atomic distributed transaction or durable broker. Therefore this failure can
temporarily leave payment and order state inconsistent.

### Production design answer

Use a transactional outbox in Payment Service:

```sql
BEGIN;
UPDATE payments
SET status = 'succeeded'
WHERE transaction_id = :transaction_id;

INSERT INTO outbox(event_id, event_type, aggregate_id, payload)
VALUES(:event_id, 'PaymentSucceeded', :transaction_id, :payload);
COMMIT;
```

An outbox worker publishes the event to Kafka/RabbitMQ. Order Service consumes it
idempotently:

```sql
BEGIN;
INSERT INTO consumed_events(event_id)
VALUES(:event_id)
ON CONFLICT DO NOTHING;

UPDATE orders
SET status = 'PAID'
WHERE id = :order_id
  AND status = 'PAYMENT_PENDING';
COMMIT;
```

### Explanation

The first local transaction makes payment state and the intention to publish
atomic. At-least-once delivery is safe because the consumer deduplicates event
IDs and performs a conditional transition. A reconciliation job compares local
state with Razorpay for long-running discrepancies.

This SQL is a **proposed production design**, not code currently implemented in
the repository.

## 15. How would you scale FoodService beyond one machine?

### Interview answer

I would keep services stateless, run replicas behind load balancers, move SQLite
to managed PostgreSQL, introduce Redis selectively, and use a durable event broker
for payment/order/notification propagation. I would add observability and scale
from SLO and saturation signals.

### Phased answer

1. Instrument p95/p99 latency, errors, CPU, memory, worker saturation, SQL time,
   and downstream time.
2. Move service databases to PostgreSQL with pooling and migrations.
3. Run multiple Gateway and service replicas with health checks.
4. Add Redis for catalogue cache, rate limits, and short-lived data—not payment
   truth.
5. Add outbox plus broker for durable asynchronous workflows.
6. Add deadlines, exponential backoff with jitter, circuit breakers, bulkheads,
   and dead-letter handling.
7. Add OpenTelemetry traces and correlation IDs.
8. Load-test each complete user journey and failure mode.

### What not to say

Do not claim that `.concurrency(128)` automatically supports millions of users.
It only configures local workers. Throughput depends on work per request, database
contention, external latency, hardware, replicas, and acceptable tail latency.

## 16. Rapid-fire follow-up questions

### Why use a unique constraint when the service already checks the key?

Because the check and insert are separate operations. Only the database can
reliably arbitrate simultaneous inserts across threads or replicas.

### Why is a mutex not the same as a database transaction?

A mutex coordinates threads inside one process. A transaction gives database
atomicity and isolation. A process mutex does not coordinate other replicas.

### Why is polling still useful when SSE exists?

It is a compatibility and recovery path. Connections can be interrupted by
proxies, browser restrictions, deployments, or mobile networks.

### Why store only the newest GPS point?

The customer UI needs the latest operational position. Full history has different
storage, privacy, retention, and analytics requirements and should be handled
asynchronously.

### Where is the biggest current scalability bottleneck?

Service-local SQLite writes and blocking synchronous dependencies are more likely
bottlenecks than CPU-only business logic. Measurements must confirm this.

### Is this fully event-driven architecture?

No. It currently uses synchronous HTTP plus reconnecting status reads. The outbox
and broker design is documented as a production evolution, not implemented fact.

## Recommended reading order

1. [System Design Code Map](SYSTEM_DESIGN_CODE_MAP.md)
2. This question-and-code workbook
3. [Sequence diagrams](SEQUENCE_DIAGRAMS.md)
4. [Concurrency diagrams](CONCURRENCY_DIAGRAMS.md)
5. [Load testing](LOAD_TESTING.md)
6. [Payment and delivery test cases](PAYMENT_DELIVERY_TEST_CASES.md)
