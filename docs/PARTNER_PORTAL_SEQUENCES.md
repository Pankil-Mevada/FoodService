# Restaurant Partner Portal Sequences

## Registration, login and membership resolution

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant US as User Service
 participant RS as Restaurant Service
 participant UDB as foodservice.db
 participant RDB as restaurant.db
 alt New partner identity
  Partner->>UI: Name, email and strong password
  UI->>GW: POST /register
  GW->>US: Create central identity
  US->>UDB: Store user and Argon2id password hash
  US-->>UI: Account created
 end
 Partner->>UI: Email and password on partner.html
 UI->>GW: POST /login
 GW->>US: Forward credentials
 US->>UDB: Verify Argon2id password hash
 US-->>UI: One-hour JWT
 UI->>GW: GET /partner/restaurants + Bearer JWT
 GW->>RS: Forward JWT and correlation ID
 RS->>RS: Verify token again
 RS->>RDB: Query ACTIVE membership by JWT user ID
 RS-->>UI: Authorized JSON array, including []
~~~

## Create private restaurant

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant RS as Restaurant Service
 participant DB as restaurant.db
 Partner->>UI: Save new restaurant
 UI->>GW: POST /partner/restaurants + JWT
 GW->>RS: Forward JWT and correlation ID
 RS->>RS: Validate identity and fields
 RS->>DB: BEGIN IMMEDIATE
 RS->>DB: Insert DRAFT restaurant
 RS->>DB: Insert ACTIVE OWNER membership
 RS->>DB: Insert RESTAURANT_CREATED audit
 RS->>DB: COMMIT
 RS-->>UI: 201, restaurantId, DRAFT, version 1
~~~

## Draft update

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant RS as Restaurant Service
 participant DB as restaurant.db
 Partner->>UI: Save draft
 UI->>GW: PUT /partner/restaurants/{id} + JWT + version
 GW->>RS: Forward JWT and correlation ID
 RS->>RS: Verify JWT again
 RS->>DB: Check ACTIVE membership, role, state and version
 alt authorized and current
  RS->>DB: Transaction update plus audit
  RS-->>UI: 200 plus new version
 else wrong owner or role
  RS-->>UI: 404 or 403
 else stale version
  RS-->>UI: 409 conflict
 end
~~~

## Approval boundary

~~~mermaid
sequenceDiagram
 actor Partner
 actor Reviewer
 participant RS as Restaurant Service
 Partner->>RS: Submit DRAFT with menu
 RS->>RS: Validate membership, role, version and menu
 RS-->>Partner: PENDING_REVIEW
 Reviewer-->>RS: Future separate admin approval API
 Note over Partner,RS: Partner routes cannot set APPROVED or clear suspension
~~~

The restaurant-profile implementation still has no durable command-idempotency
store, admin API, or outbox; those remain launch work.

## Paid order queue

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant RS as Restaurant Service
 participant OS as Order Service
 participant ODB as order.db
 Partner->>UI: Open Orders for Restaurant A
 UI->>GW: GET /partner/restaurants/A/orders + JWT
 GW->>GW: Verify JWT and derive user ID
 GW->>RS: GET /partner/restaurants/A + JWT
 RS->>RS: Verify ACTIVE membership and return role
 GW->>GW: Require ManageOrders permission
 GW->>OS: Internal GET + restaurant A + derived actor + secret
 OS->>ODB: Select Restaurant A paid orders only
 OS-->>UI: Stable newest-first JSON array, including []
 Note over OS,ODB: PAYMENT_PENDING/FAILED and other restaurants are excluded
~~~

## Kitchen status transition

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant RS as Restaurant Service
 participant OS as Order Service
 participant ODB as order.db
 Partner->>UI: Accept / prepare / ready / handoff
 UI->>GW: POST status + expectedVersion + Idempotency-Key
 GW->>RS: Recheck ACTIVE membership and ManageOrders role
 GW->>OS: Internal command + derived actor + secret
 OS->>ODB: BEGIN IMMEDIATE
 OS->>ODB: Check paid order, restaurant scope, command key and version
 OS->>OS: Enforce one-step state machine
 alt valid new command
  OS->>ODB: Upsert workflow + insert command + audit event
  OS->>ODB: COMMIT
  OS-->>UI: 200, next version
 else exact command replay
  OS-->>UI: 200, idempotentReplay=true
 else stale, skipped, foreign, or handoff before driver
  OS-->>UI: 404/409, no state change
 end
~~~

Restaurant order transitions now have durable server-side idempotency. Profile,
menu, submit, admin, and cross-service event propagation still need complete
idempotency/outbox coverage before launch.
