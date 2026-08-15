# HTTP API reference

Default base URLs are API Gateway `http://127.0.0.1:8085`, User `:8080`,
Restaurant `:8081`, Order `:8082`, Payment `:8083`, and Notification `:8084`.
JSON requests require `Content-Type: application/json`.

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
| Restaurants | `POST /restaurants` | `name`, `address`, `phone`, `rating` | Creates restaurant |
| Restaurants | `GET/PUT/DELETE /restaurants/{id}` | same fields for PUT | CRUD |
| Orders | `GET /orders` | — | Lists orders |
| Orders | `POST /orders` | `restaurantId`, `totalAmount` | JWT required; derives customer ID from the token |
| Orders | `GET/PUT/DELETE /orders/{id}` | same IDs/amount for PUT | CRUD |
| Payments | `GET /payments` | — | Direct service; lists payment state |
| Payments | `POST /payments` | `orderId`, `amount`, `paymentMethod` | Gateway requires JWT and derives customer ID; test mode |
| Payments | `GET /payments/order/{orderId}` | — | Latest payment for an order |
| Payments | `GET/PUT/DELETE /payments/{id}` | payment fields for PUT | Operational CRUD |
| Payments | `GET /payments/stream?orderId={id}` | — | SSE-compatible current-state event with retry hint |
| Payments | `POST /payments/webhooks/provider` | `transactionId`, `status`, optional `providerPaymentId` | Requires `X-Webhook-Secret` |
| Notifications | `GET /notifications` | — | Direct service; lists notifications |
| Notifications | `POST /notifications` | `userId`, `type`, `message` | Normally called by payment service |
| Notifications | `GET/PUT/DELETE /notifications/{id}` | notification fields for PUT | Operational CRUD |
| Every service | `GET /health` | — | Liveness check |

`POST /payments` accepts an `Idempotency-Key` header or `idempotencyKey` JSON
field. Reusing a key must return the existing payment rather than charge twice.
Payment states are `pending`, `processing`, `succeeded`, `failed`, and
`cancelled`. The gateway proxies these payment routes as well as the direct
Payment Service.

The implementation returns a mixture of JSON and plain-text errors. Clients
must check HTTP status before decoding a domain object. Payment provider routes
and exact status names should be documented here once the provider integration
lands; do not infer success from the order-create response alone.
