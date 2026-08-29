# Latest E2E test result

Run on 2026-08-29 against freshly rebuilt WSL services using isolated temporary
databases and test-only secrets.

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
[PASS] auth:current-user - JWT resolved to persisted user
[PASS] orders:reject-anonymous - anonymous creation returned 401
[PASS] restaurants:create-and-list - persisted restaurant found
[PASS] addresses:book-and-delivery-quote - JWT-owned address and delivery quote verified
[PASS] orders:reject-outside-delivery-zone - out-of-zone address returned 422
[PASS] orders:payment-eventual-consistency - order created a pending payment
[PASS] orders:reject-tracking-before-payment - HTTP 409; no driver assigned before verified payment
[PASS] payments:reject-forged-razorpay-signature - forged provider confirmation rejected
[PASS] payments:complete-order-payment - verified payment succeeded; delivery may start
[PASS] payments:failure-cancellation-order-sync - failure/cancellation synchronized; invalid internal secret rejected
[PASS] orders:live-delivery-tracking - no fabricated fix -> authenticated real GPS -> delivered at customer coordinates
[PASS] payments:idempotency - repeated key returned the same payment
[PASS] payments:realtime-snapshot - order lookup and SSE returned pending
[PASS] payments:reject-unsigned-webhook - unsigned mutation rejected
[PASS] payments:test-webhook-transition - pending -> processing -> succeeded
[PASS] notifications:payment-event - payment notifications observed

26 passed, 0 failed, 0 skipped
```

The suite uses temporary randomized records and performs best-effort cleanup. Payments are test-mode records; no live card transaction is performed.
