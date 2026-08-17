# Payment and delivery test cases

Updated 2026-08-17. Razorpay cases use Test Mode only. Never use real card,
UPI PIN, OTP, or banking credentials.

## Required lifecycle

`PAYMENT_PENDING -> PAYMENT_IN_PROGRESS -> succeeded -> ASSIGNED -> PICKED_UP -> ON_THE_WAY -> ARRIVING -> DELIVERED`

`PAYMENT_IN_PROGRESS` is a browser presentation state shared between the main
page and separate checkout page. Durable payment truth is stored in
`payment.db`. Delivery assignment is permitted only when that durable status is
`succeeded`.

## Acceptance and negative test matrix

| ID | Scenario | Expected frontend | Expected backend |
|---|---|---|---|
| PD-01 | Razorpay mock success | Verified; Paid/Preparing; Track enabled | Signature passes; payment `succeeded` |
| PD-02 | Track before payment | Track hidden | HTTP 409; no timer/driver |
| PD-03 | Direct assignment before payment | No driver | Order Service rejects transition |
| PD-04 | Razorpay mock failure | Error and Retry | Never marked succeeded |
| PD-05 | Checkout dismissed | Payment retry needed | Remains pending |
| PD-06 | Popup blocked | Checkout uses current tab | No premature mutation |
| PD-07 | Forged signature/order ID | Verification failed | HTTP 401/409; unchanged |
| PD-08 | Reopen checkout | Existing payment recovered | Provider order reused |
| PD-09 | Duplicate success callback | Still paid | Idempotent; no duplicate payment |
| PD-10 | Missing Test credentials | Configuration error | Provider call rejected |
| PD-11 | Provider/network unavailable | Reachable error and Retry | Remains pending |
| PD-12 | Gateway unavailable | Exact API URL error | No mutation |
| PD-13 | Payment Service unavailable | Checkout unavailable | No create/verify |
| PD-14 | Missing/expired JWT | Sign-in requested | Protected route rejected |
| PD-15 | Cancelled order | Payment unavailable | No driver |
| PD-16 | Refresh checkout | State safely recovered | Existing transaction reused |
| PD-17 | Two-tab synchronization | Main shows in progress/result | DB remains authoritative |
| PD-18 | Amount integrity | Same total shown | Trusted amount converted to paise |
| PD-19 | Real-data safety | Test warning visible | Launcher requires `rzp_test_` |
| PD-20 | Log privacy | N/A | No secrets/card/UPI PIN logged |

PD-20 also checks that success and failure decisions remain understandable via
the documented `[order-flow]`, `[payment-flow]`, `[razorpay]`, and
`[delivery-gate]` prefixes without exposing sensitive fields.

## Manual run

1. Run `scripts/start-all.ps1`; require seven `ONLINE` lines.
2. Watch `Get-Content .run/gateway.log -Wait` and
   `Get-Content .run/payments.log -Wait` in separate terminals.
3. Create an order; confirm **Track driver** is absent.
4. Click **Pay now**; confirm `payment.html` opens separately.
5. Exercise Failure/dismissal, retry, and then Success.
6. Return to the main page; confirm **Track driver** is enabled.

`tests/e2e_test.py` automates unpaid tracking rejection, forged confirmation
rejection, dummy verified completion, and paid delivery tracking without an
external charge.
