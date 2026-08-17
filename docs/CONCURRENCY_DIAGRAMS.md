# High-concurrency order processing diagrams

These diagrams describe the current implementation and the verified local
1,000-client burst test. “Parallel” means clients connect together; requests
are processed by bounded Crow worker pools and may wait while downstream work
completes. It does not mean SQLite performs 1,000 writes simultaneously.

## Runtime architecture for many clients

```mermaid
flowchart LR
    Clients["1,000 browser/test clients<br/>released together"]
    OS["Operating-system TCP accept queue"]
    GW["API Gateway :8085<br/>Crow App, 128 workers<br/>JWT + delivery-zone validation"]
    RS["Restaurant Service :8081<br/>64 workers"]
    OSVC["Order Service :8082<br/>128 workers"]
    PS["Payment Service :8083<br/>128 workers"]
    NS["Notification Service :8084<br/>64 workers"]
    RDB[("restaurant.db<br/>SQLite WAL")]
    ODB[("order.db<br/>SQLite WAL<br/>single serialized writer")]
    PDB[("payment.db<br/>SQLite WAL<br/>unique idempotency key")]
    NDB[("notification.db<br/>SQLite WAL")]
    Logs[".run/*.log<br/>C++ flow and error logs"]

    Clients --> OS --> GW
    GW -->|"GET restaurant for serviceability"| RS
    GW -->|"POST validated order"| OSVC
    RS --> RDB
    OSVC -->|"confirm restaurant"| RS
    OSVC -->|"INSERT + status update"| ODB
    OSVC -->|"POST pending payment<br/>Idempotency-Key: order-{id}"| PS
    PS -->|"indexed lookup + INSERT"| PDB
    PS -->|"payment-pending message"| NS
    NS --> NDB
    GW -.-> Logs
    OSVC -.-> Logs
    PS -.-> Logs
    NS -.-> Logs
```

The worker counts are deliberately higher than the 16 CPU cores because the
current handlers block while making localhost HTTP and SQLite calls. Workers
do not create extra copies of an order: one accepted request retains its own
stack and JSON body while shared repositories use SQLite's serialized
connection mode. The Order Repository additionally locks `INSERT` and
`sqlite3_last_insert_rowid()` together so another worker cannot replace the
new order ID between those calls.

## Code-level request path

```mermaid
flowchart TD
    Route["ApiGateway main.cpp<br/>CROW_ROUTE POST /orders"]
    JWT["authenticatedUserId(req)<br/>JwtManager::verifyToken/getUserId"]
    Zone["RestaurantClient::getRestaurantById<br/>distanceKm + deliveryRadiusKm"]
    GC["OrderClient::createOrder"]
    OC["OrderController::createOrder"]
    Domain["OrderService::createOrder"]
    Exists["OrderService RestaurantClient::restaurantExists"]
    OR["OrderRepository::saveOrder<br/>recursive mutex around INSERT + row ID"]
    PC["OrderService PaymentClient::createPayment<br/>10 s bounded timeout"]
    PCTL["PaymentController::createPayment<br/>validate + idempotency key"]
    PD["PaymentService::createPayment"]
    PR["PaymentRepository<br/>indexed key lookup / INSERT / transaction lookup"]
    NC["NotificationClient::createNotification<br/>3 s bounded timeout"]
    Status["OrderRepository::updateOrderStatus<br/>PAYMENT_PENDING or PAYMENT_FAILED"]
    Reply["HTTP response returned through<br/>OrderClient and Gateway"]

    Route --> JWT
    JWT -->|"invalid"| Unauthorized["401; no downstream work"]
    JWT -->|"valid user ID"| Zone
    Zone -->|"outside radius"| Rejected["422; no order inserted"]
    Zone -->|"serviceable"| GC --> OC --> Domain --> Exists
    Exists -->|"not found / timeout"| Failed["failure response"]
    Exists -->|"found"| OR --> PC --> PCTL --> PD --> PR
    PR --> NC
    PR -->|"payment created"| Status
    Status --> Reply
```

Relevant implementation files:

- `services/ApiGateway/src/main.cpp`
- `services/ApiGateway/src/client/OrderClient.cpp`
- `services/OrderService/src/OrderController.cpp`
- `services/OrderService/src/OrderService.cpp`
- `services/OrderService/src/OrderRepository.cpp`
- `services/OrderService/src/client/PaymentClient.cpp`
- `services/PaymentService/src/PaymentController.cpp`
- `services/PaymentService/src/PaymentService.cpp`
- `services/PaymentService/src/PaymentRepository.cpp`
- `common/src/Database.cpp`

## Sequence for a 1,000-client burst

```mermaid
sequenceDiagram
    autonumber
    participant LT as load_test.py
    participant TCP as TCP/Crow accept handling
    participant GW as Gateway 128 workers
    participant RS as Restaurant 64 workers
    participant OS as Order 128 workers
    participant ODB as order.db
    participant PS as Payment 128 workers
    participant PDB as payment.db
    participant NS as Notification 64 workers

    LT->>LT: Register one isolated test user<br/>and one isolated test restaurant
    LT->>LT: Create 1,000 asyncio tasks
    LT->>LT: Set one Event; release all tasks together
    par Requests 1..1000
        LT->>TCP: Open TCP + POST /orders + shared JWT<br/>unique LOAD marker/address
        TCP->>GW: Assign accepted connection to an available worker
        GW->>GW: Verify JWT and validate payload
        GW->>RS: Read restaurant coordinates/radius
        RS-->>GW: Restaurant record
        GW->>OS: POST order with JWT-derived userId
        OS->>RS: Confirm restaurant exists
        RS-->>OS: HTTP 200
        OS->>ODB: Serialized INSERT; obtain correct orderId
        ODB-->>OS: orderId
        OS->>PS: POST /payments + order idempotency key
        PS->>PDB: Indexed idempotency lookup; INSERT pending payment
        PDB-->>PS: Unique payment record
        PS->>NS: Create payment-pending notification
        NS-->>PS: HTTP 201
        PS-->>OS: HTTP 201
        OS->>ODB: Set PAYMENT_PENDING
        OS-->>GW: success=true
        GW-->>LT: HTTP success
    end
    Note over GW,PS: When all workers are busy, accepted requests wait.<br/>Bounded downstream timeouts prevent indefinite worker starvation.
```

## Verification and test-log flow

```mermaid
flowchart TD
    Start["python3 tests/load_test.py --orders 1000"]
    Setup["Create unique LOAD marker,<br/>test JWT, and test restaurant"]
    Burst["Release 1,000 asynchronous TCP clients"]
    Capture["Capture status, response body,<br/>and latency for every request"]
    ReadOrders["GET Order Service /orders<br/>filter deliveryAddress by marker"]
    ReadPayments["GET Payment Service /payments<br/>match persisted order IDs"]
    Checks{"All acceptance checks true?"}
    Pass["Exit 0 and passed=true"]
    Fail["Exit 1 and record grouped errors"]
    JSON["tests/results/load-1000-latest.json<br/>counts, throughput, p50/p95/p99"]
    Runtime[".run/load-1000-console.log"]
    ServiceLogs[".run/orders.log<br/>.run/payments.log<br/>.run/notifications.log"]
    E2E["python3 tests/e2e_test.py<br/>24 functional checks"]

    Start --> Setup --> Burst --> Capture --> ReadOrders --> ReadPayments --> Checks
    Checks -->|"1,000 HTTP + 1,000 orders +<br/>1,000 payments + 1,000 unique links;<br/>zero duplicates/errors"| Pass
    Checks -->|"any mismatch"| Fail
    Pass --> JSON
    Fail --> JSON
    Start -.->|"tee"| Runtime
    Burst -.->|"C++ stdout/stderr"| ServiceLogs
    Pass --> E2E
```

The latest measured run passed all four 1,000-count checks, produced no
duplicates or errors, completed in 16.99 seconds, and was followed by 24/24
passing functional tests. See [the load-testing guide](LOAD_TESTING.md) for the
commands and limitations.

## Current scaling boundary

```mermaid
flowchart LR
    Current["Current local MVP<br/>one process per service"]
    Burst["Verified: 1,000-client burst<br/>58.86 accepted orders/s"]
    Limit["Limits<br/>blocking service calls<br/>single SQLite writer<br/>no durable queue<br/>no horizontal replicas"]
    Production["Production direction<br/>load balancer + replicas<br/>PostgreSQL<br/>message broker/outbox<br/>metrics and tracing<br/>rate limits + soak tests"]
    Current --> Burst --> Limit --> Production
```

The measured result is evidence for this machine and test payload, not a claim
that the application can sustain 1,000 new orders every second or meet a
production availability target.

For interactive browsing, `GET /orders` is JWT-scoped at the Gateway and the
frontend limits payment-status enrichment to eight requests at a time. Load
verification intentionally reads the internal Order and Payment services
directly after the burst so it can validate all marker-tagged persistence rows.
