# Restaurant Partner Portal Sequences

## Draft update

~~~mermaid
sequenceDiagram
 actor Partner
 participant UI as Partner portal
 participant GW as API Gateway
 participant RS as Restaurant service
 participant DB as Restaurant DB
 Partner->>UI: Save draft
 UI->>GW: PUT partner restaurant with JWT, Idempotency-Key and If-Match
 GW->>GW: Authenticate, limit and validate
 GW->>RS: Signed identity context
 RS->>DB: Check active membership and version
 alt authorized and current
  RS->>DB: Transaction update plus audit/outbox
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
 participant RS as Restaurant service
 Partner->>RS: Submit DRAFT
 RS->>RS: Validate profile, menu and compliance metadata
 RS-->>Partner: PENDING_REVIEW
 Reviewer->>RS: Approve with separate admin credential
 RS->>RS: Verify admin permission and dual control
 RS-->>Reviewer: APPROVED plus audit event
 Note over Partner,RS: Partner routes cannot self-approve or clear suspension
~~~

## Paid order

~~~mermaid
sequenceDiagram
 participant Pay as Payment service
 participant O as Order service
 participant Q as Event bus
 participant P as Partner portal
 Pay->>O: payment verified, deduplicated
 O->>O: PENDING_PAYMENT to PAID
 O->>Q: order.paid.v1
 Q-->>P: restaurant-scoped notification
 P->>O: Accept with membership and idempotency
 O->>O: PAID to ACCEPTED
 P->>O: PREPARING then READY_FOR_PICKUP
 Note over P,O: Invalid, duplicate, unpaid, cancelled and cross-restaurant transitions fail
~~~
