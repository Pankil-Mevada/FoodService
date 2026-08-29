# End-to-end sequence diagrams

These diagrams describe how the browser frontend, API Gateway, C++
microservices, and SQLite databases communicate in the current implementation.

The multi-client worker, code-path, 1,000-order, and test-verification diagrams
are maintained in [High-concurrency order processing](CONCURRENCY_DIAGRAMS.md).

## Authenticated customer identity

```mermaid
sequenceDiagram
    participant UI as Web UI
    participant GW as API Gateway
    participant JWT as JWT verifier
    participant Order as Order Service
    UI->>GW: POST /orders + Bearer JWT<br/>{restaurantId, totalAmount, delivery destination}
    GW->>JWT: Verify token and read userId claim
    alt missing or invalid token
        GW-->>UI: 401 valid bearer token required
    else valid token
        GW->>Order: POST /orders<br/>{userId from JWT, restaurantId, totalAmount, destination}
        Order-->>GW: Order result
        GW-->>UI: Order result
    end
```

The gateway is the public identity trust boundary. It ignores any client-sent
`userId` for order and payment creation and injects the verified JWT claim.

## Components and ports

| Component | Port | Storage |
|---|---:|---|
| Plated frontend | 5173 | Browser local storage for API URL, JWT, selected location, and address |
| API Gateway | 8085 | None |
| User Service | 8080 | `foodservice.db` |
| Restaurant Service | 8081 | `restaurant.db` |
| Order Service | 8082 | `order.db` |
| Payment Service | 8083 | `payment.db` |
| Notification Service | 8084 | `notification.db` |

## 1. Registration and login

```mermaid
sequenceDiagram
    autonumber
    actor Customer
    participant UI as Plated frontend<br/>localhost:5173
    participant GW as API Gateway<br/>:8085
    participant UC as UserClient
    participant US as User Service<br/>:8080
    participant UDB as foodservice.db

    Customer->>UI: Submit registration form
    UI->>GW: POST /register<br/>{name, email, password}
    GW->>UC: registerUser(body)
    UC->>US: POST /register
    US->>US: Validate input
    US->>UDB: INSERT user with plaintext password (known security defect)
    UDB-->>US: User stored
    US-->>UC: {success, message}
    UC-->>GW: Response body
    GW-->>UI: Registration result
    UI-->>Customer: Ask user to sign in

    Customer->>UI: Submit email and password
    UI->>GW: POST /login
    GW->>UC: login(body)
    UC->>US: POST /login
    US->>UDB: Find user by email
    UDB-->>US: User and plaintext password
    US->>US: Compare password and create JWT
    US-->>GW: {success, token}
    GW-->>UI: JWT
    UI->>UI: Store token in localStorage
    UI-->>Customer: Signed-in state
```

Protected user requests include `Authorization: Bearer <JWT>`. The API Gateway
forwards this header to User Service, where authentication middleware validates
it.

Passwords are not currently hashed despite the intended production design.
Only dummy local credentials may be used until the documented security debt is
fixed.

## 2. Load restaurants

```mermaid
sequenceDiagram
    autonumber
    actor Customer
    participant UI as Plated frontend
    participant GW as API Gateway<br/>:8085
    participant RC as RestaurantClient
    participant RS as Restaurant Service<br/>:8081
    participant RDB as restaurant.db

    Customer->>UI: Open or refresh application
    UI->>GW: GET /health
    GW-->>UI: API Gateway is Healthy
    UI->>GW: GET /restaurants
    GW->>RC: getAllRestaurants()
    RC->>RS: GET /restaurants
    RS->>RDB: SELECT restaurants
    RDB-->>RS: Restaurant rows
    RS-->>RC: JSON array
    RC-->>GW: Response body
    GW-->>UI: Restaurant array
    UI-->>Customer: Render restaurant cards
```

## 3. Create order and pending payment

An order creates its payment intent automatically. The frontend reuses that
payment instead of creating a duplicate.

```mermaid
sequenceDiagram
    autonumber
    actor Customer
    participant UI as Plated frontend
    participant GW as API Gateway<br/>:8085
    participant OC as Gateway OrderClient
    participant OS as Order Service<br/>:8082
    participant RS as Restaurant Service<br/>:8081
    participant ODB as order.db
    participant PS as Payment Service<br/>:8083
    participant PDB as payment.db
    participant NS as Notification Service<br/>:8084
    participant NDB as notification.db

    Customer->>UI: Click Order here
    Customer->>UI: Select menu items/quantities and optional WELCOME10
    UI->>UI: Calculate subtotal, discount, delivery fee, and total
    UI->>GW: POST /orders + JWT<br/>{restaurantId, item summary, price breakdown, destination}
    GW->>GW: Verify JWT; inject userId; validate delivery radius
    GW->>OC: createOrder(body)
    OC->>OS: POST /orders
    OS->>RS: GET /restaurants/{restaurantId}
    RS-->>OS: Restaurant exists
    OS->>ODB: INSERT order with PENDING
    ODB-->>OS: New order ID
    OS->>PS: POST /payments<br/>Idempotency-Key: order-{id}
    PS->>PDB: Find payment by idempotency key
    PDB-->>PS: No existing payment
    PS->>PDB: INSERT test payment with pending status
    PDB-->>PS: Payment stored
    PS->>NS: POST /notifications<br/>Payment pending
    NS->>NDB: INSERT notification
    NDB-->>NS: Notification stored
    NS-->>PS: Created
    PS-->>OS: HTTP 201 payment created
    OS->>ODB: UPDATE order status = PAYMENT_PENDING
    OS-->>OC: Order created response
    OC-->>GW: Response body
    GW-->>UI: {success, message}
    UI->>GW: GET /orders + JWT
    GW->>GW: Filter rows to JWT customer ID
    GW-->>UI: Signed-in customer's order array
    UI->>GW: GET /payments/order/{orderId}
    GW-->>UI: Pending payment
    UI-->>Customer: Render order with pending status
```

The frontend performs payment lookups with a bounded pool of eight requests,
so even a large customer history cannot monopolize the browser connection queue
and block a new order submission.

If the restaurant does not exist, the order is rejected. If payment creation
fails—for example, an amount above `1000000`—Order Service records
`PAYMENT_FAILED` and no payable transaction exists.

## 4. Dummy provider payment and live UI status

### Razorpay Test Mode checkout

```mermaid
sequenceDiagram
    participant Customer
    participant UI as Plated frontend
    participant GW as API Gateway
    participant OS as Order Service
    participant PS as Payment Service
    participant RZ as Razorpay Test API / Checkout
    participant DB as payment.db
    Customer->>UI: Click Pay securely
    UI->>GW: POST /payments/razorpay/order (JWT, transactionId)
    GW->>PS: Create sandbox provider order
    PS->>RZ: POST /v1/orders (server credentials)
    RZ-->>PS: order_id, amount, INR
    PS->>DB: Save provider and order_id
    PS-->>UI: Public Test Key ID and checkout order
    UI->>RZ: Open hosted Standard Checkout
    Customer->>RZ: Complete mock payment
    RZ-->>UI: payment_id, order_id, signature
    UI->>GW: POST /payments/razorpay/verify (JWT)
    GW->>PS: Forward verification payload
    PS->>PS: HMAC-SHA256(order_id|payment_id, Key Secret)
    PS->>DB: Mark succeeded only if signature matches
    PS-->>UI: Verified payment state
    UI-->>Customer: Main order shows Paid / Preparing
    Customer->>UI: Click Track driver
    UI->>GW: GET /orders/{id}/tracking
    GW->>PS: GET /payments/order/{id}
    PS-->>GW: status=succeeded
    GW->>OS: POST /orders/{id}/status ASSIGNED
    OS->>PS: Confirm latest payment succeeded
    PS-->>OS: succeeded
    OS-->>GW: Driver assignment accepted
    GW-->>UI: Driver, coordinates, ETA, timeline
```

Test Mode moves no real funds. The browser receives only the public Test Key
ID; the Key Secret remains inside Payment Service.

If either payment check is not `succeeded`, tracking returns HTTP 409 and Order
Service rejects the delivery transition. The tracking timer is not created.

`scripts/test-dummy-payment.ps1` simulates the external provider. It looks up
the payment transaction and sends an authenticated webhook.

```mermaid
sequenceDiagram
    autonumber
    actor Customer
    participant UI as Plated frontend
    participant GW as API Gateway<br/>:8085
    participant PC as Gateway PaymentClient
    participant PS as Payment Service<br/>:8083
    participant PDB as payment.db
    participant Script as Dummy payment script

    Customer->>UI: Click Pay now
    UI->>GW: GET /payments/order/{orderId}
    GW->>PC: getPaymentForOrder(orderId)
    PC->>PS: GET /payments/order/{orderId}
    PS->>PDB: Find payment for order
    PDB-->>PS: Payment with pending status
    PS-->>UI: Pending payment
    UI->>GW: GET /payments/stream?orderId={id}
    GW->>PS: GET SSE snapshot
    PS->>PDB: Read current status
    PS-->>UI: event: payment-status<br/>data: pending<br/>retry: 2000
    Note over UI,PS: Connection closes and EventSource reconnects.<br/>Polling is the fallback.

    Script->>GW: GET /payments/order/{orderId}
    GW-->>Script: transactionId and current status
    Script->>GW: POST /payments/webhooks/provider<br/>X-Webhook-Secret<br/>{transactionId, status: succeeded}
    GW->>PC: providerWebhook(body, secret)
    PC->>PS: Forward webhook
    PS->>PS: Verify secret and allowed transition
    PS->>PDB: UPDATE status = succeeded
    PDB-->>PS: Payment updated
    PS-->>Script: Updated payment
    UI->>GW: SSE reconnect or polling request
    GW->>PS: Read payment status
    PS->>PDB: SELECT payment
    PDB-->>PS: succeeded
    PS-->>UI: payment-status: succeeded
    UI->>UI: Update order card and close dialog
    UI-->>Customer: Display Payment succeeded
```

Allowed provider transitions:

```mermaid
stateDiagram-v2
    [*] --> pending
    pending --> processing
    pending --> succeeded
    pending --> failed
    pending --> cancelled
    processing --> succeeded
    processing --> failed
    processing --> cancelled
    succeeded --> [*]
    failed --> [*]
    cancelled --> [*]
```

## 5. Payment-to-order consistency boundary

After an authenticated provider callback is accepted, Payment Service persists
the payment transition and synchronously calls the authenticated internal Order
Service payment-status endpoint.

```mermaid
sequenceDiagram
    participant Provider as Dummy/real provider
    participant PS as Payment Service
    participant PDB as payment.db
    participant OS as Order Service
    participant ODB as order.db

    Provider->>PS: Webhook: succeeded
    PS->>PDB: Idempotently set Payment = succeeded
    PDB-->>PS: Durable payment state
    PS->>OS: POST /orders/{id}/payment-status<br/>X-Internal-Secret<br/>{paymentStatus: succeeded}
    OS->>OS: Validate secret and transition policy
    OS->>ODB: PAYMENT_PENDING -> CONFIRMED
    ODB-->>OS: Order updated
    OS-->>PS: HTTP 200 synchronized
    PS-->>Provider: HTTP 200 payment + order consistent

    alt Order Service unavailable
        PS-->>Provider: HTTP 502 payment stored; retry safely
        Note over PS,OS: Duplicate callback re-attempts order synchronization
    end
```

The same endpoint maps `processing -> PAYMENT_PENDING`,
`failed -> PAYMENT_FAILED`, and `cancelled -> CANCELLED`. Duplicate events are
accepted without regressing `CONFIRMED` or delivery states. This local synchronous
callback exposes failures but cannot guarantee delivery after a process crash.
Production still requires a transactional outbox, durable broker, idempotent
consumer/event IDs, retries, and payment/order reconciliation.

## 6. Fetch nearby restaurants from the customer's location

```mermaid
sequenceDiagram
    participant Customer
    participant UI as Web UI
    participant GW as API Gateway
    participant OSM as OpenStreetMap Overpass
    participant RS as Restaurant Service
    participant RDB as restaurant.db

    Customer->>UI: Click Use GPS & find nearby
    UI->>Customer: Request browser location permission
    Customer-->>UI: Allow GPS coordinates
    alt GPS unavailable or demo requested
        Customer->>UI: Click Bengaluru demo
        UI->>UI: Use configured Bengaluru test coordinates
    end
    UI->>GW: GET /restaurants/discover?lat&lon
    GW->>GW: Identify Bengaluru/Ahmedabad coordinate region
    GW->>OSM: One capped nearby restaurant query
    OSM-->>GW: Public restaurant POIs + optional image/Wikimedia tag
    GW->>RS: Import up to 20 restaurants with coordinates + imageUrl
    RS->>RDB: Deduplicate and persist restaurants/radius
    RDB-->>RS: Saved restaurant rows
    GW-->>UI: City, provider, discovered count
    UI->>GW: GET /restaurants
    GW->>RS: Load saved and newly imported restaurants
    RS-->>UI: Restaurants with coordinates and delivery radius
    UI->>UI: Save customer coordinates and calculate Haversine distance
    UI-->>Customer: Show nearby/orderable and outside-area cards
    Customer->>UI: Enter address and place order
    UI->>GW: POST /orders + JWT + destination
    GW->>RS: GET /restaurants/{id}
    RS-->>GW: Coordinates and delivery radius
    GW->>GW: Haversine serviceability check
    alt outside restaurant delivery radius
        GW-->>UI: 400 restaurant does not deliver to this location
    else serviceable
        GW-->>UI: Continue authenticated order workflow
    end
```

Nearby discovery is user-triggered and displays OpenStreetMap attribution. The
public endpoint is a development dependency only; production must use a
contracted or self-hosted provider with caching, monitoring, and privacy review.

## 7. Real driver GPS tracking

```mermaid
sequenceDiagram
    participant Driver
    participant DriverUI as Driver browser portal
    participant Customer
    participant UI as Web UI
    participant GW as API Gateway
    participant JWT as JWT verifier
    participant PS as Payment Service
    participant OS as Order Service
    participant DDB as delivery.db
    participant ODB as order.db

    Driver->>DriverUI: Enter paid order and driver token; allow GPS
    DriverUI->>DriverUI: navigator.geolocation.watchPosition
    loop Every five seconds
        DriverUI->>GW: POST driver location + token + actual GPS fix
        GW->>PS: Verify payment succeeded
        PS-->>GW: succeeded
        GW->>DDB: UPSERT coordinates, accuracy, speed, status, timestamp
        GW->>OS: Persist submitted delivery status
        GW-->>DriverUI: HTTP 202 GPS accepted
    end
    Customer->>UI: Track driver
    loop Every five seconds while tracking is open
        UI->>GW: GET /orders/{id}/tracking + JWT
        GW->>JWT: Verify token and derive customer ID
        GW->>OS: GET /orders/{id}
        OS->>ODB: Load order and destination coordinates
        ODB-->>OS: Order record
        OS-->>GW: Order details
        GW->>DDB: Read latest actual driver fix
        alt no driver fix exists
            GW-->>UI: 409 waiting for driver GPS
        else stored fix exists
            GW->>GW: Calculate age, distance progress and ETA
            GW-->>UI: Actual coordinates, accuracy, freshness, ETA, timeline
            UI->>UI: Move marker using measured progress
        end
    end
    DriverUI->>GW: Final customer coordinates + DELIVERED
    GW->>ODB: Persist DELIVERED
    GW-->>UI: 100% progress, ETA 0
```

Coordinates now come from real browser GPS movement and remain durable across
Gateway restarts. The local shared driver token is not production identity or
dispatch. Production still requires driver accounts, assignment-scoped tokens,
background mobile tracking, HTTPS, retention/deletion enforcement, and a
selected maps/routing provider for road-aware ETA.

## Bounded synchronous Gateway call

```mermaid
sequenceDiagram
  participant UI as Browser
  participant GW as API Gateway worker
  participant HC as HttpClient/libcurl
  participant SVC as Domain service
  UI->>GW: HTTP request
  GW->>HC: synchronous downstream call
  HC->>SVC: HTTP (connect timeout 2s, total 10s)
  Note over GW,HC: This one Gateway worker waits
  alt service responds
    SVC-->>HC: status + body
    HC-->>GW: HttpResult
    GW-->>UI: preserve downstream status/body
  else total timeout
    HC-->>GW: Timeout failure
    GW-->>UI: 504 JSON error
  else DNS/connect/transport failure
    HC-->>GW: Transport failure
    GW-->>UI: 502 JSON error
  end
  Note over GW: Other Crow workers remain available
```

This prevents an unavailable dependency from holding a Gateway worker forever.
It does not yet provide asynchronous I/O, retries, circuit breaking, or a
request correlation ID.

## Source-code map

- Browser orchestration: `frontend/app.js`
- API Gateway routes: `services/ApiGateway/src/main.cpp`
- Gateway HTTP clients: `services/ApiGateway/src/client/`
- Order workflow: `services/OrderService/src/OrderService.cpp`
- Order-to-payment request: `services/OrderService/src/client/PaymentClient.cpp`
- Payment validation and webhooks: `services/PaymentService/src/PaymentController.cpp`
- Payment transition rules: `services/PaymentService/src/PaymentService.cpp`
- Payment persistence: `services/PaymentService/src/PaymentRepository.cpp`
- Database schema creation: `common/src/Database.cpp`
- Automated end-to-end flow: `tests/e2e_test.py`
- Parallel load and persistence verification: `tests/load_test.py`
- Manual payment simulator: `scripts/test-dummy-payment.ps1`
# Address selection and protected checkout

```mermaid
sequenceDiagram
  actor Customer
  participant UI as Web UI
  participant Map as OpenStreetMap/Nominatim
  participant GW as API Gateway
  participant DB as Delivery DB
  participant RS as Restaurant Service
  participant OS as Order Service
  Customer->>UI: GPS, manual entry, map search/click
  UI->>Map: Search/tiles (only after user action)
  UI->>GW: POST /addresses + JWT
  GW->>DB: Store address under JWT user ID
  UI->>GW: POST /delivery/quote
  GW->>RS: Load radius/polygon and fee rules
  GW-->>UI: serviceable, distance, fee, ETA, rules
  alt outside delivery zone
    UI-->>Customer: Checkout blocked
  else serviceable
    UI->>GW: POST /orders + addressId
    GW->>DB: Verify address belongs to JWT user
    GW->>RS: Reload restaurant rules
    GW->>GW: Recalculate quote (do not trust UI)
    GW->>OS: Create order with server fee/total
    OS-->>UI: Order created
  end
```
