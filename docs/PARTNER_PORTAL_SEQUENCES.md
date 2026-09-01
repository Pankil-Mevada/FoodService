# Restaurant Partner Portal Sequences

## Login and membership resolution

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant US as User Service
 participant RS as Restaurant Service
 participant UDB as foodservice.db
 participant RDB as restaurant.db
 Partner->>UI: Email and password
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

The current implementation has no durable command-idempotency store, admin API
or outbox yet; those remain launch work and are not implied by a browser header.
