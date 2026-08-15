# Codebase guide

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
