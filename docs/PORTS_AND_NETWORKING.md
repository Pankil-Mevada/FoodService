# Ports and Networking in FoodService

This short guide explains what ports are, how FoodService assigns them, how
`localhost` behaves, and how development ports differ from production exposure.

## FoodService port map

| Component | Port | Purpose | Source |
|---|---:|---|---|
| Frontend server | 5173 | Serves HTML, CSS, JavaScript, and images | `scripts/start-all.sh` |
| User Service | 8080 | Accounts, login, and JWTs | `services/UserService/src/main.cpp` |
| Restaurant Service | 8081 | Restaurant catalogue CRUD | `services/RestaurantService/src/main.cpp` |
| Order Service | 8082 | Order CRUD and status | `services/OrderService/src/main.cpp` |
| Payment Service | 8083 | Payments, Razorpay, webhook, and SSE | `services/PaymentService/src/main.cpp` |
| Notification Service | 8084 | Notification CRUD | `services/NotificationService/src/main.cpp` |
| API Gateway | 8085 | Public development API used by the frontend | `services/ApiGateway/src/main.cpp` |

`5173` is the frontend port. `8085` is the API Gateway port.

```text
Browser loads UI from http://localhost:5173
Browser calls API at http://localhost:8085
Gateway calls internal services on ports 8080–8084
Payment Service calls Razorpay through HTTPS on port 443
```

## What is a port?

An IP address identifies a computer or network interface. A port identifies a
program listening on that computer.

```text
192.168.1.10       = laptop address
192.168.1.10:5173  = frontend program on that laptop
192.168.1.10:8085  = API Gateway program on that laptop
```

The operating system uses the destination port to deliver an incoming TCP
connection to the correct server process. Two programs cannot normally listen on
the same IP address, protocol, and port simultaneously.

## Standard port ranges

TCP and UDP port numbers are 16-bit values from `0` through `65535`.

| Range | Common name | Typical use |
|---:|---|---|
| 0–1023 | System / well-known | Standard public protocols, such as HTTP 80 and HTTPS 443; binding may require elevated privileges |
| 1024–49151 | User / registered | Applications and development servers; FoodService uses 5173 and 8080–8085 here |
| 49152–65535 | Dynamic / private | Temporary client-side source ports and short-lived private use |

These ranges describe registration and convention; they do not automatically
make a port secure. Firewall rules, process binding, authentication, and network
placement determine who can reach it.

## Server ports versus temporary client ports

A server listens on a stable, known port:

```text
API Gateway listens on 8085
```

When the browser connects, the operating system normally assigns the browser a
temporary source port:

```text
127.0.0.1:53421  ->  127.0.0.1:8085
temporary client     stable server
```

The temporary source port can change for every connection. This does not mean
the Gateway port changed.

## How ports are defined in code

Crow binds the API Gateway to port 8085:

```cpp
// services/ApiGateway/src/main.cpp
app.port(8085)
   .concurrency(128)
   .run();
```

Payment Service supports environment-based configuration with an 8083 default:

```cpp
// services/PaymentService/src/main.cpp
const char* value = std::getenv("PAYMENT_SERVICE_PORT");
const unsigned short port = value
    ? static_cast<unsigned short>(std::stoi(value))
    : 8083;
app.port(port).concurrency(128).run();
```

The startup script serves frontend files on port 5173:

```bash
# scripts/start-all.sh
python3 -m http.server 5173 --directory frontend
```

The frontend's default API address points to the Gateway:

```javascript
// frontend/app.js
apiUrl: localStorage.getItem('plated_api_url') ||
        'http://localhost:8085'
```

## Does the port change between laptops?

The configured server port normally remains the same. Each laptop has its own
network stack, so both can use port 8085:

```text
Laptop A: 192.168.1.10:8085
Laptop B: 192.168.1.20:8085
```

A port changes only when configuration changes or the preferred port is already
occupied. An “address already in use” error means another process is listening
on that address and port.

## What does localhost mean?

`localhost` normally resolves to loopback address `127.0.0.1` or `::1` and means
“this same device.” Traffic remains on the device instead of travelling across
the public internet.

```text
Laptop browser + http://localhost:8085 -> Gateway on laptop
Mobile browser + http://localhost:8085 -> looks for Gateway on mobile
```

Therefore, a mobile phone must use the laptop's LAN address, for example:

```text
http://192.168.1.10:5173
```

The frontend must also call:

```text
http://192.168.1.10:8085
```

Both devices normally need the same Wi-Fi, the servers must listen on an
interface reachable from the LAN, and Windows Firewall must permit the ports.
Never open development service ports directly to the public internet.

## How WSL affects localhost

FoodService backend processes run through WSL, while the browser runs on Windows.
Modern WSL commonly forwards localhost ports between the environments. The
startup health checks confirm that ports 8080–8085 and 5173 are reachable.

For phone access, WSL/Windows forwarding and firewall configuration may require
additional setup because the phone connects to the Windows LAN address rather
than Windows loopback.

## Development port usage

During local development, high unprivileged ports are convenient because:

- each service needs a unique address;
- developers can run services without administrator/root privileges;
- direct service health checks and logs are easy;
- ports such as 5173 and 8080–8085 are familiar development conventions.

Only the frontend and Gateway normally need to be used by the browser. Ports
8080–8084 are service internals, even though they are locally reachable for tests.

If a port must change, update both the listening service and every caller. Prefer
environment configuration instead of scattering hard-coded URLs.

## Production port usage

Production users should normally access standard public ports:

| Public port | Protocol | Use |
|---:|---|---|
| 80 | HTTP | Usually redirects to HTTPS |
| 443 | HTTPS | Encrypted website and API traffic |

```text
Customer
   | HTTPS :443 over internet
   v
DNS -> CDN/WAF/load balancer
   | private network
   v
API Gateway replicas -> internal services
```

The load balancer terminates TLS on port 443 and routes traffic privately to the
Gateway. User, Restaurant, Order, Payment, and Notification ports should be
blocked from the public internet by private networking and firewall/security
group rules.

Production recommendations:

- use DNS names instead of fixed laptop IP addresses;
- expose only 80/443 at the public edge;
- redirect HTTP to HTTPS;
- keep internal services on private networks;
- use service discovery rather than hard-coded hostnames;
- apply authentication, rate limits, WAF rules, and request logging at the edge;
- use health checks and load balancing across replicas;
- store ports and service URLs in deployment configuration.

## Where the internet is used

In the local stack, calls between `localhost` ports use the laptop's loopback
network and do not traverse the public internet. The public internet is used when:

- the browser loads Razorpay Checkout;
- Payment Service calls `https://api.razorpay.com` on HTTPS port 443;
- a production customer connects to the deployed public domain;
- services call an external restaurant/location provider.

HTTP is an application-layer protocol. It is normally transported by TCP, which
runs over IP:

```text
JSON -> HTTP/HTTPS -> TCP -> IP -> loopback, LAN, or internet
```

## Quick troubleshooting

| Symptom | Likely cause |
|---|---|
| `localhost:5173` does not open | frontend server is stopped |
| UI shows “Cannot reach API Gateway” | port 8085 is stopped, blocked, or configured incorrectly |
| “Address already in use” | another process owns the selected port |
| Laptop works but mobile does not | mobile used localhost, firewall blocked access, or server is loopback-only |
| Gateway works but payment fails | Payment Service 8083 or Razorpay internet access is unavailable |
| One internal CRUD route fails | owning service port 8080–8084 is unavailable |

Use `.run/*.log` to inspect each service and the startup health checks to confirm
which listening endpoint is unavailable.
