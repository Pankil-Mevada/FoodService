# Codebase guide

## Runtime flow observability

The primary diagnostic path is `.run/gateway.log` -> `.run/payments.log` ->
`.run/orders.log`. Structured messages originate in:

- `services/ApiGateway/src/main.cpp` for routing and tracking gates.
- `services/PaymentService/src/PaymentController.cpp` for provider endpoints.
- `services/PaymentService/src/PaymentService.cpp` for durable transitions.
- `services/PaymentService/src/RazorpayClient.cpp` for safe provider outcomes.
- `services/OrderService/src/OrderService.cpp` and
  `src/client/PaymentClient.cpp` for paid-before-driver enforcement.

Use the transaction ID and order ID to correlate messages. Never add logging of
credentials, Authorization headers, provider signatures, payment instrument
details, OTPs, UPI PINs, passwords, or full third-party response bodies.

Product scope, priorities, acceptance criteria, and the delivery roadmap are in
[`PRODUCT_REQUIREMENTS.md`](PRODUCT_REQUIREMENTS.md).

- `common/include`, `common/src`: shared infrastructure and security helpers.
- `services/ApiGateway`: public façade and HTTP clients for domain services.
- `services/UserService`: registration, login, JWT issuance, and user CRUD.
- `services/RestaurantService`: restaurant catalogue CRUD.
- `services/OrderService`: order persistence and restaurant/payment orchestration.
- `services/PaymentService`: payment lifecycle and notification client.
- `services/NotificationService`: notification persistence and CRUD.
- `frontend`: browser application maintained separately.
- `tests`: dependency-free black-box acceptance harness.
- `docs`: architecture, API, setup, and testing documentation.
- `third_party/Crow`: vendored Crow framework; avoid editing for app features.

Each service follows the same reading path: start at `src/main.cpp` for routes,
then controller, service, repository, and model. Cross-service calls live under
`src/client` with corresponding headers under `include/client`.

When adding a feature, keep HTTP parsing in controllers, domain decisions in
services, and SQL in repositories. Update route documentation and extend the
E2E harness with observable behavior. Payment changes should include an explicit
state machine, idempotency behavior, failure recovery, and secret handling.
