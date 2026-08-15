# Latest E2E test result

Run on 2026-08-15 against the locally running WSL services.

```text
[PASS] health:gateway - API Gateway is Healthy!
[PASS] health:users - User Service is Healthy!
[PASS] health:restaurants - Restaurant Service is Healthy!
[PASS] health:orders - Order Service is Healthy!
[PASS] health:payments - Payment Service is Healthy!
[PASS] health:notifications - Notification Service is Healthy!
[PASS] auth:register - test account created
[PASS] auth:login - JWT received
[PASS] auth:reject-missing-token - request rejected without JWT
[PASS] users:list - persisted user found
[PASS] restaurants:create-and-list - persisted restaurant found
[PASS] orders:payment-eventual-consistency - order created a pending payment
[PASS] payments:idempotency - repeated key returned the same payment
[PASS] payments:realtime-snapshot - order lookup and SSE returned pending
[PASS] payments:reject-unsigned-webhook - unsigned mutation rejected
[PASS] payments:test-webhook-transition - pending -> processing -> succeeded
[PASS] notifications:payment-event - payment notifications observed

17 passed, 0 failed, 0 skipped
```

The suite uses temporary randomized records and performs best-effort cleanup. Payments are test-mode records; no live card transaction is performed.
