# JWT Authentication in FoodService

JWT means **JSON Web Token**. It is a signed identity token that FoodService
returns after successful login. The browser sends it with protected requests so
the backend can identify the customer without sending the password again.

## Simple flow

```mermaid
sequenceDiagram
    participant UI as Browser
    participant GW as API Gateway
    participant US as User Service
    participant API as Protected API
    UI->>GW: POST /login with email and password
    GW->>US: Forward login
    US->>US: Verify password and sign JWT
    US-->>UI: JWT containing userId
    UI->>GW: POST /orders with Authorization: Bearer JWT
    GW->>GW: Verify signature, issuer, and expiry
    GW->>API: Forward request with trusted userId
    API-->>UI: Customer-authorized response
```

## JWT pronunciation and structure

JWT is commonly pronounced **“jot.”** A token contains three dot-separated
Base64URL sections:

```text
header.payload.signature
```

Example decoded content:

```json
{
  "header": { "alg": "HS256", "typ": "JWT" },
  "payload": {
    "iss": "FoodService",
    "sub": "customer@example.com",
    "userId": "7",
    "iat": 1787000000,
    "exp": 1787086400
  }
}
```

The header describes the algorithm. The payload contains claims. The signature
proves that the header and payload have not been modified after FoodService signed
them.

A JWT is normally **signed, not encrypted**. Anyone holding it can decode the
payload, so never place passwords, payment secrets, UPI PINs, or private keys in
a JWT.

## 1. Login verifies the password

The login request enters through the API Gateway and is forwarded to User
Service. User Service finds the account, verifies the submitted password against
the stored password representation, and only then generates a token.

Relevant files:

- `services/ApiGateway/src/main.cpp` — public `/login` route
- `services/ApiGateway/src/client/UserClient.cpp` — forwards login
- `services/UserService/src/UserController.cpp` — parses the request
- `services/UserService/src/UserService.cpp` — verifies login and generates JWT
- `common/src/PasswordHasher.cpp` — password hashing/verification

The password is needed for login but is not sent with every later API request.

## 2. User Service generates the JWT

`services/UserService/src/UserService.cpp` calls `JwtManager::generateToken()`
after successful credential verification.

The implementation is in `common/src/JwtManager.cpp`:

```cpp
auto token =
    jwt::create()
        .set_issuer("FoodService")
        .set_subject(email)
        .set_payload_claim(
            "userId",
            jwt::claim(std::to_string(userId)))
        .set_issued_at(system_clock::now())
        .set_expires_at(system_clock::now() + hours{24})
        .sign(jwt::algorithm::hs256{m_secret});
```

FoodService adds these claims:

| Claim | Meaning |
|---|---|
| `iss` | Token issuer: `FoodService` |
| `sub` | Subject: customer email |
| `userId` | Internal authenticated customer ID |
| `iat` | Time at which the token was issued |
| `exp` | Expiration time, currently 24 hours later |

HS256 uses one shared secret to both sign and verify the token.

## 3. The frontend stores the token

After login, `frontend/app.js` saves the returned token:

```javascript
state.token = data.token;
localStorage.setItem('plated_token', data.token);
```

On page reload, it restores the token:

```javascript
const state = {
  token: localStorage.getItem('plated_token') || ''
};
```

This is convenient for local testing, but browser local storage is readable by
JavaScript. A successful cross-site-scripting attack could steal it. A production
web application should evaluate secure `HttpOnly`, `Secure`, and `SameSite`
cookies, a strong Content Security Policy, short token lifetime, and appropriate
CSRF protection.

## 4. The frontend sends a Bearer token

The common frontend request function attaches the token:

```javascript
// frontend/app.js
if (state.token) {
  headers.Authorization = `Bearer ${state.token}`;
}
```

The resulting HTTP request resembles:

```http
POST /orders HTTP/1.1
Host: localhost:8085
Authorization: Bearer eyJhbGciOiJIUzI1NiJ9...
Content-Type: application/json
```

“Bearer” means possession grants access. The token must therefore be protected
like a temporary credential and sent only through HTTPS in production.

## 5. The API Gateway verifies the token

`services/ApiGateway/src/main.cpp::authenticatedUserId()` performs the common
verification:

```cpp
std::optional<int> authenticatedUserId(const crow::request& req)
{
    const std::string header =
        req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";

    if (header.rfind(prefix, 0) != 0)
        return std::nullopt;

    const std::string token = header.substr(prefix.size());
    JwtManager jwt;

    if (!jwt.verifyToken(token))
        return std::nullopt;

    try {
        return jwt.getUserId(token);
    } catch (...) {
        return std::nullopt;
    }
}
```

`common/src/JwtManager.cpp` verifies the algorithm, shared-secret signature,
issuer, and registered time claims supported by the JWT library:

```cpp
auto verifier =
    jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{m_secret})
        .with_issuer("FoodService");

verifier.verify(decoded);
```

A missing, expired, malformed, incorrectly issued, or incorrectly signed token is
rejected.

## 6. Authentication versus authorization

These are different checks:

- **Authentication:** Is this a valid token, and which customer owns it?
- **Authorization:** May that customer access this particular resource?

For driver tracking, the Gateway first authenticates the caller and then verifies
order ownership:

```cpp
const auto userId = authenticatedUserId(req);
if (!userId)
    return unauthorized();

const auto order = crow::json::load(client.getOrderById(id));
if (!order || !order.has("id"))
    return jsonError(404, "Order not found");

if (order["userId"].i() != *userId)
    return jsonError(403,
        "This order belongs to another customer");
```

A valid token for user 7 does not authorize access to user 99's order.

## 7. Trusted identity injection

The browser is untrusted. It could submit a false `userId` in JSON. The Gateway
therefore derives identity from the verified JWT and overwrites client identity:

```cpp
const auto userId = authenticatedUserId(req);
if (!userId) return unauthorized();

crow::json::wvalue body;
body["userId"] = *userId;
```

Example:

```text
Browser JSON claims userId 99
Verified JWT contains userId 7
Gateway sends userId 7 to Order/Payment Service
```

This prevents the client from placing an order or payment under another user's
identity.

## Where JWT is used in FoodService

The Gateway requires or uses JWT identity for flows including:

| Operation | JWT purpose |
|---|---|
| `GET /me` | Return the signed-in profile |
| `POST /orders` | Derive the order owner |
| `GET /orders` | Return only the customer's orders |
| `GET /orders/{id}/tracking` | Verify order ownership |
| `POST /payments` | Derive payment customer identity |
| Razorpay order creation | Require an authenticated customer |
| Razorpay verification | Require an authenticated customer |

See `services/ApiGateway/src/main.cpp` for the exact route checks.

## Common JWT failure responses

| Situation | Expected result |
|---|---|
| No `Authorization` header | `401 Unauthorized` |
| Header does not start with `Bearer ` | `401 Unauthorized` |
| Token signature was changed | `401 Unauthorized` |
| Token expired | `401 Unauthorized` |
| Valid user accesses another customer's order | `403 Forbidden` |
| Valid token and authorized resource | Request continues |

`401` means the caller is not successfully authenticated. `403` means the caller
is authenticated but is not permitted to access that resource.

## Logout behavior

The current frontend signs out by deleting its local token:

```javascript
state.token = '';
localStorage.removeItem('plated_token');
```

This prevents that browser from sending the token again. It does not immediately
revoke a copied token on the server. The token remains cryptographically valid
until expiration.

Production systems may use short-lived access tokens, refresh-token rotation,
server-side session records, key rotation, or a revocation list depending on the
security and scale requirements.

## Current limitation: hard-coded development secret

The current constructor contains a development-only secret:

```cpp
JwtManager::JwtManager()
    : m_secret("FoodServiceSecretKey")
{
}
```

This must not be used for a public production deployment. Anyone with repository
access could create apparently valid tokens.

Production requirements:

- load keys from a secrets manager or protected runtime configuration;
- use a strong randomly generated secret or asymmetric signing keys;
- rotate keys and identify them with a key ID;
- use HTTPS everywhere;
- shorten access-token lifetime;
- protect or rotate refresh tokens;
- never log complete JWTs;
- validate issuer, audience, algorithm, expiration, and required claims;
- consistently enforce resource authorization after authentication.

## Interview-ready answer

> FoodService creates an HS256 JWT after User Service verifies the customer's
> password. The token contains issuer, email subject, user ID, issue time, and a
> 24-hour expiration. The browser currently stores it in local storage and sends
> it as an Authorization Bearer header. The API Gateway verifies the token and
> derives the trusted user ID, then performs resource authorization—for example,
> confirming that the requested order belongs to that customer. The current
> hard-coded secret and local-storage strategy are development choices; production
> would use managed rotating keys, HTTPS, shorter-lived tokens, secure browser
> storage, and complete authorization coverage.

## Related documents

- [System Design Code Map](SYSTEM_DESIGN_CODE_MAP.md)
- [System-Design Questions with Code](SYSTEM_DESIGN_QA_WITH_CODE.md)
- [API reference](API.md)
- [Sequence diagrams](SEQUENCE_DIAGRAMS.md)
- [Testing](TESTING.md)
