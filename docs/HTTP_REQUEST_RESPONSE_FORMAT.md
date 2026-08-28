# HTTP request and response format

This guide explains the structure of normal HTTP communication and maps each
part to FoodService and Crow C++ code.

## 1. Request overview

An HTTP client sends a request to a server. A FoodService client can be the
browser frontend, a mobile application, PowerShell, curl, Postman, or another
microservice.

```text
METHOD /path HTTP/version
Header-Name: header value
Header-Name: header value

Optional request body
```

The four sections are:

1. request line;
2. request headers;
3. one empty line;
4. optional request body.

The empty line is meaningful: it separates the headers from the body.

## 2. Complete FoodService request

```http
POST /orders HTTP/1.1
Host: localhost:8085
Content-Type: application/json
Accept: application/json
Authorization: Bearer eyJhbGciOiJIUzI1NiJ9...
Idempotency-Key: order-123-create
Content-Length: 203

{
  "restaurantId": 5,
  "subtotal": 299,
  "discountAmount": 30,
  "deliveryLatitude": 12.9716,
  "deliveryLongitude": 77.5946,
  "deliveryAddress": "MG Road, Bengaluru"
}
```

Conceptually:

```text
POST             -> requested operation
/orders          -> target resource/route
HTTP/1.1         -> protocol version
headers          -> metadata, identity and representation
empty line       -> end of headers
JSON object      -> data supplied to the route
```

## 3. Request line

```http
POST /orders HTTP/1.1
```

The request line contains three tokens separated by spaces:

| Part | Example | Meaning |
|---|---|---|
| Method | `POST` | Operation requested by the client. |
| Request target | `/orders` | Resource path, optionally including a query string. |
| Version | `HTTP/1.1` | HTTP protocol version used for the message. |

### Common methods

| Method | Usual meaning | FoodService example |
|---|---|---|
| `GET` | Read a resource without changing it. | `GET /restaurants` |
| `POST` | Create a resource or trigger an operation. | `POST /orders` |
| `PUT` | Replace or update a known resource. | `PUT /orders/25` |
| `DELETE` | Remove a resource. | `DELETE /addresses/4` |
| `OPTIONS` | Ask which cross-origin operations are allowed. | `OPTIONS /orders` |

The convention describes intent; the server route ultimately determines actual
behavior.

## 4. Path parameters

A path identifies the route:

```http
GET /orders/25 HTTP/1.1
```

Here, `/orders` identifies the resource type and `25` is an order ID. Crow
declares this route as:

```cpp
CROW_ROUTE(app, "/orders/<int>")
.methods(crow::HTTPMethod::GET)
([](int id)
{
    // id is 25 for /orders/25
});
```

Use path parameters for the identity of a specific resource.

## 5. Query parameters

Query parameters follow `?`; multiple parameters are separated with `&`:

```http
GET /restaurants/discover?lat=12.9716&lon=77.5946 HTTP/1.1
```

```text
/restaurants/discover  -> path
?                       -> beginning of query string
lat=12.9716             -> first name/value pair
&                       -> separator
lon=77.5946             -> second name/value pair
```

The API Gateway reads them with:

```cpp
const char* lat = req.url_params.get("lat");
const char* lon = req.url_params.get("lon");
```

Query parameters are useful for search, filters, pagination, sorting, and
optional inputs. Do not put passwords, JWTs, payment credentials, or secrets in
a URL: URLs are commonly retained in browser history, proxies, and logs.

## 6. Request headers

Headers carry metadata as name/value pairs:

```http
Header-Name: header value
```

Header names are case-insensitive. Header values have header-specific syntax.

### Host

```http
Host: localhost:8085
```

`localhost` is the host name and `8085` is the API Gateway port. In production
the value might be `api.foodservice.example` and DNS would resolve that name to
an IP address.

### Content-Type

```http
Content-Type: application/json
```

`Content-Type` describes the request body representation. Common examples:

| Value | Body format |
|---|---|
| `application/json` | JSON object or array |
| `text/plain` | Plain text |
| `text/html` | HTML document |
| `multipart/form-data` | Form containing files or multiple fields |
| `application/x-www-form-urlencoded` | Traditional encoded form fields |

FoodService normally sends JSON to backend routes.

### Accept

```http
Accept: application/json
```

`Accept` tells the server which response representation the client can process.
It is different from `Content-Type`: `Content-Type` describes what is being
sent; `Accept` describes what the client wants back.

### Authorization

```http
Authorization: Bearer eyJhbGciOiJIUzI1NiJ9...
```

`Bearer` is the authentication scheme and the remaining value is the JWT. The
gateway retrieves it with:

```cpp
const std::string header = req.get_header_value("Authorization");
```

Login and registration do not require this header. Protected customer routes
do require it.

### Idempotency-Key

```http
Idempotency-Key: order-123-payment-attempt-1
```

This identifies one logical operation. If a client retries the same payment
request after a timeout, the backend can recognize the key and avoid creating a
second payment.

### Content-Length

```http
Content-Length: 203
```

This gives the body size in bytes. Browser and HTTP libraries normally compute
it automatically. Developers should not manually guess its value.

### FoodService-specific driver header

```http
X-Driver-Token: local-driver-test-token
```

The delivery-partner GPS endpoint uses this header to authenticate a location
publisher. Customer routes use the JWT instead.

## 7. Request body

The request body follows the empty line after the headers:

```http
Content-Type: application/json

{
  "restaurantId": 5,
  "subtotal": 299
}
```

Crow exposes the raw bytes as `req.body`:

```cpp
const auto input = crow::json::load(req.body);
if (!input || !input.has("restaurantId"))
{
    return crow::response(400, "restaurantId is required");
}

const int restaurantId = input["restaurantId"].i();
const double subtotal = input["subtotal"].d();
```

`GET` requests normally do not have a body. Use the path or query string for
normal read parameters.

## 8. Examples from FoodService

### Login

```http
POST /login HTTP/1.1
Host: localhost:8085
Content-Type: application/json
Accept: application/json

{
  "email": "customer@example.com",
  "password": "test-password"
}
```

There is no JWT yet. A successful login response returns one.

### Read tracking data

```http
GET /orders/2020/tracking HTTP/1.1
Host: localhost:8085
Accept: application/json
Authorization: Bearer eyJ...

```

There is no request body. The order ID is part of the path and customer
identity comes from the JWT.

### Publish driver GPS

```http
POST /driver/orders/2020/location HTTP/1.1
Host: localhost:8085
Content-Type: application/json
X-Driver-Token: local-driver-test-token

{
  "latitude": 12.9716,
  "longitude": 77.5946,
  "accuracy": 15,
  "speed": 4.2,
  "status": "ON_THE_WAY",
  "driverName": "Pankil",
  "vehiclePlate": "KA1236"
}
```

### Service-to-service order call

After validating the frontend request, the API Gateway creates another HTTP
request to Order Service:

```http
POST /orders HTTP/1.1
Host: localhost:8082
Content-Type: application/json

{
  "userId": 7,
  "restaurantId": 5,
  "totalAmount": 308,
  "deliveryAddress": "MG Road, Bengaluru"
}
```

This request travels through the laptop's network stack between two independent
C++ processes. `HttpClient.cpp` builds it using libcurl.

## 9. HTTP response format

The server returns a response with a similar structure:

```http
HTTP/1.1 201 Created
Content-Type: application/json
Content-Length: 67

{
  "success": true,
  "id": 2021,
  "message": "Order created"
}
```

The four sections are:

1. status line;
2. response headers;
3. one empty line;
4. optional response body.

### Status line

```http
HTTP/1.1 201 Created
```

| Part | Meaning |
|---|---|
| `HTTP/1.1` | Protocol version |
| `201` | Machine-readable status code |
| `Created` | Human-readable reason phrase |

### Common status codes

| Code | Meaning | FoodService use |
|---|---|---|
| `200 OK` | Operation succeeded. | Successful reads or updates |
| `201 Created` | Resource was created. | New address or order |
| `202 Accepted` | Request was accepted for processing. | Driver location accepted |
| `204 No Content` | Success with no response body. | Possible delete response |
| `400 Bad Request` | Request syntax or required fields are invalid. | Missing JSON field |
| `401 Unauthorized` | Valid authentication was not supplied. | Missing/invalid JWT |
| `403 Forbidden` | Identity is known but access is not allowed. | Another customer's order |
| `404 Not Found` | Resource does not exist. | Unknown order/restaurant |
| `409 Conflict` | Request conflicts with current state. | Tracking before payment |
| `422 Unprocessable Content` | Syntax is valid but business data is invalid. | Invalid coordinates or delivery zone |
| `500 Internal Server Error` | Unexpected server failure. | Database/internal error |
| `502 Bad Gateway` | Gateway received an invalid upstream response. | Broken service response |
| `503 Service Unavailable` | Service or external dependency is unavailable. | Nearby provider unavailable |

Status code and JSON body should agree, but clients should use the status code
as the primary protocol-level outcome.

## 10. CORS preflight request

The browser considers `localhost:5173` and `localhost:8085` different origins
because their ports differ. Before some cross-origin requests it sends:

```http
OPTIONS /orders HTTP/1.1
Host: localhost:8085
Origin: http://localhost:5173
Access-Control-Request-Method: POST
Access-Control-Request-Headers: authorization,content-type

```

The gateway's CORS middleware returns headers indicating what it permits. If
the preflight succeeds, the browser sends the real `POST /orders` request.

```mermaid
sequenceDiagram
    participant UI as Browser :5173
    participant GW as API Gateway :8085
    UI->>GW: OPTIONS /orders (preflight)
    GW-->>UI: Allowed origin, method, and headers
    UI->>GW: POST /orders + JWT + JSON
    GW-->>UI: HTTP status + JSON
```

CORS is a browser policy, not authentication. curl, Postman, mobile clients, and
other servers are not secured by CORS. JWT, authorization checks, and driver or
webhook verification are still required.

## 11. HTTP versus HTTPS

HTTP describes the request and response semantics shown above. HTTPS carries
the same HTTP messages inside a TLS-encrypted connection.

```text
HTTP  = request/response is not encrypted in transit
HTTPS = HTTP protected by TLS encryption and server identity certificates
```

Local development commonly uses `http://localhost`. A production login,
payment, customer, or driver-location API must use HTTPS.

## 12. How the message travels

For a browser call to the local gateway:

```text
JavaScript fetch()
    -> browser constructs HTTP request
    -> operating-system networking
    -> TCP connection to localhost:8085
    -> Crow parses request line, headers, and body
    -> matching CROW_ROUTE handler runs
    -> handler constructs crow::response
    -> Crow serializes HTTP response
    -> browser receives status, headers, and body
    -> JavaScript reads response.status and response.json()
```

For HTTP/1.1, TCP provides an ordered reliable byte stream. HTTP defines how
those bytes represent requests and responses.

## 13. Browser JavaScript example

```javascript
const response = await fetch("http://localhost:8085/orders", {
  method: "POST",
  headers: {
    "Content-Type": "application/json",
    "Authorization": `Bearer ${token}`,
    "Idempotency-Key": "order-123-create"
  },
  body: JSON.stringify(order)
});

if (!response.ok) {
  const error = await response.json();
  throw new Error(error.message);
}

const createdOrder = await response.json();
```

The browser library creates the raw request line, calculates content length,
opens the connection, and parses the response. Application code supplies the
method, URL, important headers, and body.

## 14. PowerShell and curl examples

PowerShell:

```powershell
$headers = @{ Authorization = "Bearer $token" }
Invoke-RestMethod `
  -Method Get `
  -Uri "http://localhost:8085/orders/2020/tracking" `
  -Headers $headers
```

curl:

```bash
curl --include \
  --request GET \
  --header "Authorization: Bearer $TOKEN" \
  http://localhost:8085/orders/2020/tracking
```

`--include` displays response headers as well as the body. Add `--verbose` to
inspect connection and request details, but be careful: verbose logs can expose
JWTs and other credentials.

## 15. Debugging in browser Developer Tools

Open **Developer Tools -> Network**, select a request, and inspect:

1. **General:** request URL, method, and response status.
2. **Request Headers:** origin, content type, authorization presence, and custom
   headers.
3. **Payload:** JSON body actually sent by the frontend.
4. **Response Headers:** content type and CORS information.
5. **Response/Preview:** JSON or text returned by the server.
6. **Timing:** connection, waiting, and download durations.

Do not share screenshots containing complete JWTs, real payment keys, passwords,
cookies, or driver tokens.

## 16. Quick memory model

For requests:

```text
What action?       -> method
On which resource? -> path and query
Who/what format?   -> headers
With which data?   -> body
```

For responses:

```text
What happened?     -> status code
What format?       -> response headers
What result?       -> response body
```
