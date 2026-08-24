# Payment workflow

PaymentService runs in safe test mode: it creates a durable `pending` intent but never contacts a processor or charges a card. Configure it with environment variables:

- `PAYMENT_SERVICE_PORT` (default `8083`)
- `PAYMENT_DATABASE_PATH` (default `payment.db`)
- `PAYMENT_WEBHOOK_SECRET` (default `test-webhook-secret`; production deployments must set a strong secret)
- `ORDER_SERVICE_URL` (default `http://localhost:8082`)
- `ORDER_SYNC_SECRET` (default `local-order-sync-secret`; Payment and Order services must share a strong non-default value outside local development)
- API Gateway: `PAYMENT_SERVICE_URL` (default `http://localhost:8083`)

OrderService also uses `PAYMENT_SERVICE_URL`, creates payments with an order-derived idempotency key, and records `PAYMENT_PENDING` until a provider event resolves the payment.

## API (also exposed by ApiGateway on port 8085)

- `POST /payments`: JSON `{orderId,userId,amount,paymentMethod,idempotencyKey?}`. Prefer the `Idempotency-Key` header. Repeating a key returns the original payment.
- `GET /payments/{id}`: payment status.
- `GET /payments/order/{orderId}`: latest payment for an order.
- `GET /payments/stream?orderId={id}`: SSE snapshot (`payment-status` event and 2-second retry). The connection closes after each event so `EventSource` reconnects; polling the order endpoint is an equivalent fallback.
- `POST /payments/webhooks/provider`: authenticated test/provider callback. Send `X-Webhook-Secret` and JSON `{transactionId,status,providerPaymentId?}`. Allowed states are `processing`, `succeeded`, `failed`, and `cancelled`; terminal states cannot transition.

After a provider transition is stored, Payment Service authenticates to Order
Service's internal `/orders/{id}/payment-status` route. Processing, success,
failure, and cancellation map to `PAYMENT_PENDING`, `CONFIRMED`,
`PAYMENT_FAILED`, and `CANCELLED`. If synchronization fails, the provider route
returns HTTP 502; retrying the identical event is safe and re-attempts the order
update. This is a local synchronous consistency step, not a replacement for the
production outbox, durable event delivery, and reconciliation roadmap.

Payment mutation and deletion are deliberately not routed. Status changes only through the authenticated webhook transition rules.

For a production processor, replace the shared-secret callback check with that provider's official signature verification SDK and replace intent creation with its HTTPS API. Keep raw card data outside this service; accept only provider tokens/payment-method identifiers.
