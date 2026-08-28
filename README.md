# 🍔 FoodService - C++ Microservices Backend

[![FoodService CI](https://github.com/Pankil-Mevada/FoodService/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/Pankil-Mevada/FoodService/actions/workflows/ci.yml)

FoodService is a backend application built using **Modern C++20**, **Crow**, **SQLite**, and **JWT Authentication** following a **Microservices Architecture**.

For an interview-focused explanation of the microservices, CRUD paths,
multithreading, scaling trade-offs, and model questions, read the
[FoodService Interview Guide](docs/INTERVIEW_GUIDE.md).
For the exact source file responsible for each architecture concept, see the
[System Design Code Map](docs/SYSTEM_DESIGN_CODE_MAP.md).
Practice interview answers using actual repository snippets in
[System-Design Questions, Answers, and Code](docs/SYSTEM_DESIGN_QA_WITH_CODE.md).
For local ports, localhost, WSL/mobile access, and production exposure, see
[Ports and Networking in FoodService](docs/PORTS_AND_NETWORKING.md).
For login tokens, Bearer headers, trusted identity, and route authorization, see
[JWT Authentication in FoodService](docs/JWT_AUTHENTICATION.md).
For the branch strategy, local checks, automated GitHub pipeline, protected-branch
setup, and failure troubleshooting, follow the
[Git toolchain and automated pipeline guide](docs/GIT_PIPELINE.md).

The project demonstrates how multiple independent services communicate while following layered architecture principles such as Controller, Service, Repository, and Database layers.

---

# Architecture

```
                    +------------------+
                    |      Client      |
                    +--------+---------+
                             |
                             |
                     HTTP REST APIs
                             |
      -------------------------------------------------
      |        |           |          |              |
      |        |           |          |              |
+-----------+ +-----------+ +-----------+ +-----------+ +----------------+
|UserService| |Restaurant | |Order      | |Payment    | |Notification    |
|           | |Service    | |Service    | |Service    | |Service         |
+-----------+ +-----------+ +-----------+ +-----------+ +----------------+
      |             |             |             |               |
      -----------------------------------------------------------
                             |
                      Common Library
                             |
                    SQLite Database
```

---

# Features

- REST APIs using Crow
- Modern C++20
- SQLite Database
- Repository Pattern
- Service Layer
- JWT Authentication
- Modular Microservices
- CMake Build System
- JSON APIs
- Common Shared Library

---

# Project Structure

```
FoodService
│
├── common/
│   ├── include/
│   └── src/
│
├── services/
│   ├── UserService/
│   ├── RestaurantService/
│   ├── OrderService/
│   ├── PaymentService/
│   └── NotificationService/
│
├── third_party/
│   └── Crow/
│
├── CMakeLists.txt
├── vcpkg.json
└── README.md
```

---

# Microservices

## User Service

Responsible for

- User Registration
- Login
- JWT Generation
- JWT Verification
- User CRUD

Runs on

```
http://localhost:8080
```

---

## Restaurant Service

Responsible for

- Add Restaurant
- Update Restaurant
- Delete Restaurant
- Get Restaurant

---

## Order Service

Responsible for

- Place Order
- Cancel Order
- Order History

---

## Payment Service

Responsible for

- Create Payment
- Payment Status
- Payment History

---

## Notification Service

Responsible for

- Send Notifications
- Email Notification (Future)
- SMS Notification (Future)

---

# Technologies

| Technology | Purpose |
|------------|----------|
| C++20 | Programming Language |
| Crow | REST Framework |
| SQLite3 | Database |
| JWT-CPP | JWT Authentication |
| OpenSSL | JWT Signing |
| Argon2 | Password Hashing |
| Asio | Networking |
| CMake | Build System |
| vcpkg | Dependency Management |

---

# Build

Clone repository

```bash
git clone <repository_url>

cd FoodService
```

Clone vcpkg

```bash
git clone https://github.com/microsoft/vcpkg.git /workspaces/vcpkg

cd /workspaces/vcpkg

./bootstrap-vcpkg.sh
```

Configure project

```bash
cmake -S . \
-B build \
-DCMAKE_TOOLCHAIN_FILE=/workspaces/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Build

```bash
cmake --build build
```

---

# Dependency Management (vcpkg.json)

The project uses **vcpkg Manifest Mode**.

Dependencies are declared in

```
vcpkg.json
```

Current dependencies

```json
{
  "dependencies": [
    "sqlite3",
    "openssl",
    {
      "name": "jwt-cpp",
      "features": [
        "picojson"
      ]
    },
    "asio",
    "argon2"
  ]
}
```

When CMake configures the project, **vcpkg automatically installs all required libraries**.

This means developers **do not need to manually install each package** one by one.

---

# JWT Authentication

Login API

```
POST /login
```

Example

```json
{
    "email":"admin@test.com",
    "password":"admin123"
}
```

Response

```json
{
    "success": true,
    "message":"Login successful",
    "token":"<JWT_TOKEN>"
}
```

Protected APIs require

```
Authorization: Bearer <JWT_TOKEN>
```

Without token

```
401 Unauthorized
```

---

# API Examples

Register User

```
POST /register
```

Get Users

```
GET /users
```

Create Restaurant

```
POST /restaurants
```

Create Order

```
POST /orders
```

Create Payment

```
POST /payments
```

---

# Design Pattern

The project follows layered architecture.

```
Controller
      |
      v
Service
      |
      v
Repository
      |
      v
Database
```

Responsibilities

- Controller handles HTTP requests
- Service contains business logic
- Repository performs database operations
- Database manages SQLite

---

# Security

Current

- JWT Authentication
- Authorization Middleware

Planned

- Argon2 Password Hashing
- Role Based Access Control
- HTTPS
- Refresh Tokens

---

# Future Enhancements

- Docker
- Docker Compose
- API Gateway
- Redis Cache
- Kafka
- GoogleTest
- CI/CD
- Swagger/OpenAPI
- Service Discovery
- Kubernetes

---

# Learning Objectives

This project demonstrates

- Modern C++
- REST API Development
- JWT Authentication
- Microservices
- SQLite
- CMake
- Repository Pattern
- Clean Architecture
- Dependency Management using vcpkg

---

# Author

**Pankil**

Modern C++ Developer
