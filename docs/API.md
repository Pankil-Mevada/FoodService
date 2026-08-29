# HTTP API reference

Default base URLs are API Gateway `http://127.0.0.1:8085`, User `:8080`,
Restaurant `:8081`, Order `:8082`, Payment `:8083`, and Notification `:8084`.
JSON requests require `Content-Type: application/json`.

## Gateway downstream behavior

The Gateway preserves HTTP status codes returned by User, Restaurant, Order,
and Payment services. Its synchronous libcurl calls have a 2-second connection
timeout and a 10-second total timeout for GET, POST, PUT, and DELETE.

- A downstream timeout returns `504` with
  `{"success":false,"message":"A downstream service timed out"}`.
- DNS, connection, and other transport failures return `502` with
  `{"success":false,"message":"A downstream service is unavailable"}`.
- A valid downstream response retains its original status and body.

The transport error text is logged server-side but is not returned to clients.
This is a bounded synchronous design; the request's Gateway worker waits until
the response or timeout while other workers remain available.

| API | Method and path | Request body | Notes |
|---|---|---|---|
| Gateway | `GET /health` | — | Gateway health |
| Auth | `POST /register` | `name`, `email`, `password` | Creates user; 201 or 409 |
| Auth | `POST /login` | `email`, `password` | Returns JWT in `token` |
| Auth | `GET /me` | — | JWT required; returns the signed-in user |
| Users | `GET /users` | — | `Authorization: Bearer <JWT>` |
| Users | `GET /users/{id}` | — | JWT required |
| Users | `PUT /users/{id}` | `name`, `email`, `password` | JWT required |
| Users | `DELETE /users/{id}` | — | JWT required |
| Restaurants | `GET /restaurants` | — | Lists restaurants |
| Restaurants | `GET /restaurants/discover?lat={lat}&lon={lon}` | — | User-triggered OpenStreetMap lookup; deduplicates/imports up to 20 nearby restaurants and returns city/provider metadata |
| Restaurants | `POST /restaurants` | `name`, `address`, `phone`, `rating`, optional `latitude`, `longitude`, `deliveryRadiusKm`, `imageUrl` | Creates restaurant, delivery zone, and optional HTTP(S) photo reference |
| Restaurants | `GET/PUT/DELETE /restaurants/{id}` | same fields for PUT | CRUD |
| Orders | `GET /orders` | — | JWT required; returns only orders owned by the signed-in customer |
| Orders | `POST /orders` | `restaurantId`, `totalAmount`, `deliveryLatitude`, `deliveryLongitude`, `deliveryAddress`; optional `itemSummary`, `subtotal`, `discountAmount`, `deliveryFee` | JWT required; validates delivery zone and price arithmetic, derives customer ID |
| Orders | `GET/PUT/DELETE /orders/{id}` | same IDs/amount for PUT | CRUD |
| Internal order sync | `POST /orders/{id}/payment-status` | `paymentStatus`: `processing`, `succeeded`, `failed`, or `cancelled` | Direct Order Service only; requires `X-Internal-Secret`; maps payment state to order state idempotently |
| Delivery | `GET /orders/{id}/tracking` | — | JWT/ownership and successful payment required; returns the latest real driver GPS fix, freshness, distance-based progress/ETA, and timeline |
| Driver GPS | `POST /driver/orders/{id}/location` | `latitude`, `longitude`; optional accuracy, speed, heading, delivery status and driver/vehicle profile | `X-Driver-Token` required; verified payment required; stores the real browser GPS fix in `delivery.db` |

Tracking returns HTTP `409` until the latest payment has durable status
`succeeded`. Order Service also validates payment before accepting internal
delivery-status transitions, preventing a direct assignment bypass.

The local lifecycle is `ASSIGNED -> PICKED_UP -> ON_THE_WAY -> ARRIVING -> DELIVERED`.
Tracking begins on the first tracking request, refreshes every five seconds, and
reaches `DELIVERED` after 180 seconds. Driver contacts and vehicle plates are
test values and do not identify real people or vehicles.
| Payments | `GET /payments` | — | Direct service; lists payment state |
| Payments | `POST /payments` | `orderId`, `amount`, `paymentMethod` | Gateway requires JWT and derives customer ID; test mode |
| Payments | `GET /payments/order/{orderId}` | — | Latest payment for an order |
| Payments | `GET/PUT/DELETE /payments/{id}` | payment fields for PUT | Operational CRUD |
| Payments | `GET /payments/stream?orderId={id}` | — | SSE-compatible current-state event with retry hint |
| Payments | `POST /payments/webhooks/provider` | `transactionId`, `status`, optional `providerPaymentId` | Requires `X-Webhook-Secret`; synchronizes the corresponding order or returns 502 for safe retry |
| Razorpay | `POST /payments/razorpay/order` | `transactionId` | JWT required; creates/reuses a Test Mode order server-side |
| Razorpay | `POST /payments/razorpay/verify` | `transactionId`, `razorpay_order_id`, `razorpay_payment_id`, `razorpay_signature` | JWT required; HMAC verification required before success |
| Notifications | `GET /notifications` | — | Direct service; lists notifications |
| Notifications | `POST /notifications` | `userId`, `type`, `message` | Normally called by payment service |
| Notifications | `GET/PUT/DELETE /notifications/{id}` | notification fields for PUT | Operational CRUD |
| Every service | `GET /health` | — | Liveness check |

Nearby discovery uses the configurable development provider endpoint in the
gateway and sends only the selected latitude/longitude. The bundled public
OpenStreetMap endpoint is for low-volume local testing, must retain attribution,
and must be replaced by a contracted or self-hosted provider for production.

`POST /payments` accepts an `Idempotency-Key` header or `idempotencyKey` JSON
field. Reusing a key must return the existing payment rather than charge twice.
Payment states are `pending`, `processing`, `succeeded`, `failed`, and
`cancelled`. The gateway proxies these payment routes as well as the direct
Payment Service.

Payment Service calls Order Service with `X-Internal-Secret` after an accepted
provider transition. Order mapping is `processing -> PAYMENT_PENDING`,
`succeeded -> CONFIRMED`, `failed -> PAYMENT_FAILED`, and
`cancelled -> CANCELLED`. A repeated status returns success without regressing a
later order state. If the payment is durable but Order Service is unavailable,
the provider endpoint returns HTTP 502 with the durable payment object and a
retry-safe message.

The implementation still returns a mixture of JSON and plain-text domain errors,
although Gateway-generated transport errors are JSON. Clients must check HTTP
status before decoding a domain object. Payment provider routes
and exact status names should be documented here once the provider integration
lands; do not infer success from the order-create response alone.
# Delivery address and quote APIs

All `/addresses` operations require the customer's bearer JWT.

- `GET /addresses` lists only the signed-in customer's addresses.
- `POST /addresses` accepts `label`, `recipient`, `phone`, `addressLine`, `latitude`, `longitude`, `deliveryNotes`, and `isDefault`.
- `PUT /addresses/{id}/select` makes an owned address the default.
- `DELETE /addresses/{id}` deletes an owned address.
- `POST /delivery/quote` accepts `restaurantId`, `latitude`, and `longitude`. It returns `serviceable`, `distanceKm`, `deliveryFee`, `etaMinutes`, `zoneType`, and applied pricing flags. An outside-zone point returns HTTP 422.

`POST /orders` accepts an optional `addressId`. The gateway verifies ownership, loads its coordinates, recalculates the delivery quote, rejects an outside-zone order, and replaces browser-provided delivery fee/total with server-calculated values.

Restaurant configuration supports `deliveryRadiusKm`, `deliveryPolygon` (JSON `[[latitude,longitude], ...]`), `baseDeliveryFee`, `perKmFee`, and `preparationMinutes`. A valid polygon takes precedence over radius.
