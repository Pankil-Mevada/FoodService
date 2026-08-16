# Plated frontend

A dependency-free responsive web client for FoodService. It covers registration/login, restaurant discovery, order creation/cancellation, payment checkout, and live payment status.

## Run

Start the API gateway on `http://localhost:8085`, then serve this directory (opening the file directly is not recommended):

```powershell
cd frontend
python -m http.server 5173
```

Open `http://localhost:5173`. Use **API settings** in the footer if the gateway uses another URL or payment stream route.

## API contract

- `GET /health`
- `POST /register`, `POST /login`
- `GET /restaurants`
- `GET|POST /orders`, `DELETE /orders/{id}`
- `POST /payments` with an `Idempotency-Key` header and JSON `idempotencyKey` (creation responses use `{ success, message, payment }`)
- `GET /payments/order/{orderId}`
- `GET /payments/stream?orderId={orderId}` (server-sent events)

Order creation may create its pending payment automatically. Checkout first looks up the order payment and only creates one when none exists, preventing duplicate charges. The live watcher handles the named `payment-status` SSE event and automatically switches to 2.5-second polling if SSE is unavailable. The API URL and stream path are stored only in browser local storage. JWTs are sent as bearer tokens for normal API calls; the SSE endpoint must not require an Authorization header because native `EventSource` cannot set one.

The order dialog includes a three-item sample menu using generated project assets
in `assets/menu/`. Quantities, the `WELCOME10` test coupon, delivery fee, and
final total can be tested without entering an arbitrary amount. These photographs
illustrate menu items and are not claimed to come from the listed restaurant.

Signed-in users can open **Profile** to upload a browser-local avatar, save a
display name, phone, and test UPI ID. Restaurant hearts are browser-local
favourites. The image is resized before storage. The app never requests a UPI
PIN, and none of these fields are bank-verified or synchronized to the backend.

## Files

- `index.html` — semantic application structure and dialogs
- `styles.css` — responsive design and accessible states
- `app.js` — API client, state, auth, orders, payments, SSE/polling

No build step or package install is required.
