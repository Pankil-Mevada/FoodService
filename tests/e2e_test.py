#!/usr/bin/env python3
"""Dependency-free FoodService API smoke/E2E harness.

The suite uses only localhost HTTP calls and test-mode payment data. It never
submits card numbers, provider secrets, or requests a live charge.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
import urllib.error
import urllib.request
import uuid
from dataclasses import dataclass
from typing import Any, Callable


@dataclass
class Result:
    name: str
    outcome: str
    detail: str = ""


class Api:
    def __init__(self, timeout: float = 5.0):
        self.timeout = timeout

    def request(self, method: str, url: str, body: Any = None,
                token: str | None = None,
                extra_headers: dict[str, str] | None = None) -> tuple[int, Any]:
        headers = {"Accept": "application/json"}
        data = None
        if body is not None:
            data = json.dumps(body).encode()
            headers["Content-Type"] = "application/json"
        if token:
            headers["Authorization"] = f"Bearer {token}"
        if extra_headers:
            headers.update(extra_headers)
        request = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                raw = response.read().decode()
                return response.status, self._decode(raw)
        except urllib.error.HTTPError as error:
            raw = error.read().decode()
            return error.code, self._decode(raw)

    @staticmethod
    def _decode(raw: str) -> Any:
        try:
            return json.loads(raw)
        except (json.JSONDecodeError, TypeError):
            return raw


class Suite:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.api = Api(args.timeout)
        self.results: list[Result] = []
        self.created: dict[str, int] = {}
        self.token: str | None = None
        self.idempotent_payment_id: int | None = None
        self.test_transaction_id: str | None = None
        suffix = uuid.uuid4().hex[:10]
        self.email = f"foodservice-e2e-{suffix}@example.test"

    def run(self, name: str, operation: Callable[[], str]) -> None:
        try:
            self.results.append(Result(name, "PASS", operation()))
        except SkipTest as error:
            self.results.append(Result(name, "SKIP", str(error)))
        except Exception as error:  # keep running to report all feature gaps
            self.results.append(Result(name, "FAIL", str(error)))

    @staticmethod
    def expect(status: int, allowed: tuple[int, ...], payload: Any) -> None:
        if status not in allowed:
            raise AssertionError(f"expected HTTP {allowed}, got {status}: {payload}")

    def health(self, name: str, base: str) -> str:
        status, payload = self.api.request("GET", f"{base}/health")
        self.expect(status, (200,), payload)
        return str(payload)

    def register(self) -> str:
        status, payload = self.api.request("POST", f"{self.args.gateway}/register", {
            "name": "FoodService E2E", "email": self.email, "password": "TestOnly123!"
        })
        # The current API gateway preserves the downstream body but normalizes
        # the proxied status to 200.
        self.expect(status, (200, 201), payload)
        if not isinstance(payload, dict) or payload.get("success") is not True:
            raise AssertionError(f"registration was not accepted: {payload}")
        return self.email

    def login(self) -> str:
        status, payload = self.api.request("POST", f"{self.args.gateway}/login", {
            "email": self.email, "password": "TestOnly123!"
        })
        self.expect(status, (200,), payload)
        if not isinstance(payload, dict) or not payload.get("token"):
            raise AssertionError(f"login did not return token: {payload}")
        self.token = payload["token"]
        return "JWT received"

    def users(self) -> str:
        status, payload = self.api.request("GET", f"{self.args.gateway}/users", token=self.token)
        self.expect(status, (200,), payload)
        if not isinstance(payload, list):
            raise AssertionError(f"expected users array: {payload}")
        match = next((user for user in payload if user.get("email") == self.email), None)
        if not match:
            raise AssertionError("registered user absent from list")
        self.created["user"] = int(match["id"])
        return f"user id {match['id']}"

    def current_user(self) -> str:
        status, payload = self.api.request("GET", f"{self.args.gateway}/me", token=self.token)
        self.expect(status, (200,), payload)
        if not isinstance(payload, dict) or payload.get("email") != self.email:
            raise AssertionError(f"JWT resolved to the wrong user: {payload}")
        if int(payload.get("id", 0)) != self.created.get("user"):
            raise AssertionError(f"JWT user ID did not match registered user: {payload}")
        return f"JWT resolves to user {payload.get('id')}"

    def reject_anonymous_order(self) -> str:
        status, payload = self.api.request("POST", f"{self.args.gateway}/orders", {
            "restaurantId": 1, "totalAmount": 1.0
        })
        if status != 401:
            raise AssertionError(f"anonymous order was accepted: HTTP {status}, {payload}")
        return "HTTP 401"

    def restaurant(self) -> str:
        name = f"E2E Kitchen {uuid.uuid4().hex[:8]}"
        status, payload = self.api.request("POST", f"{self.args.gateway}/restaurants", {
            "name": name, "address": "Test Mode Street", "phone": "+10000000000", "rating": 4.5,
            "latitude": 23.0225, "longitude": 72.5714, "deliveryRadiusKm": 8.0
        })
        self.expect(status, (200, 201), payload)
        if not isinstance(payload, dict) or payload.get("success") is not True:
            raise AssertionError(f"restaurant creation was not accepted: {payload}")
        status, payload = self.api.request("GET", f"{self.args.gateway}/restaurants")
        self.expect(status, (200,), payload)
        match = next((item for item in payload if item.get("name") == name), None)
        if not match:
            raise AssertionError("created restaurant absent from list")
        self.created["restaurant"] = int(match["id"])
        return f"restaurant id {match['id']}"

    def order_and_payment(self) -> str:
        if "user" not in self.created or "restaurant" not in self.created:
            raise SkipTest("requires successful user and restaurant setup")
        before_status, before = self.api.request("GET", f"{self.args.payments}/payments")
        self.expect(before_status, (200,), before)
        status, payload = self.api.request("POST", f"{self.args.gateway}/orders", {
            "userId": self.created["user"] + 999999, "restaurantId": self.created["restaurant"],
            "totalAmount": 12.34, "deliveryLatitude": 23.0240,
            "deliveryLongitude": 72.5730, "deliveryAddress": "E2E Test Address"
        }, token=self.token)
        self.expect(status, (200, 201), payload)
        if not isinstance(payload, dict) or payload.get("success") is not True:
            raise AssertionError(f"order creation was not accepted: {payload}")
        status, orders = self.api.request("GET", f"{self.args.gateway}/orders")
        self.expect(status, (200,), orders)
        candidates = [item for item in orders if item.get("userId") == self.created["user"]]
        if not candidates:
            raise AssertionError("created order absent from list")
        order = max(candidates, key=lambda item: item["id"])
        self.created["order"] = int(order["id"])

        deadline = time.monotonic() + self.args.eventual_timeout
        payment = None
        while time.monotonic() < deadline:
            p_status, payments = self.api.request("GET", f"{self.args.payments}/payments")
            self.expect(p_status, (200,), payments)
            payment = next((item for item in payments if item.get("orderId") == order["id"]), None)
            if payment:
                break
            time.sleep(self.args.poll_interval)
        if not payment:
            raise AssertionError("no payment record created for order")
        self.created["payment"] = int(payment["id"])
        return f"order {order['id']} -> payment {payment['id']} ({payment.get('status')})"

    def reject_outside_delivery_zone(self) -> str:
        if "restaurant" not in self.created:
            raise SkipTest("requires successful restaurant setup")
        status, payload = self.api.request("POST", f"{self.args.gateway}/orders", {
            "restaurantId": self.created["restaurant"], "totalAmount": 9.99,
            "deliveryLatitude": 28.6139, "deliveryLongitude": 77.2090,
            "deliveryAddress": "Outside test delivery zone"
        }, token=self.token)
        if status != 422 or "outside" not in str(payload).lower():
            raise AssertionError(f"out-of-zone order was not rejected: HTTP {status}, {payload}")
        return "HTTP 422 outside delivery area"

    def delivery_tracking(self) -> str:
        if "order" not in self.created:
            raise SkipTest("requires successful order setup")
        status, payload = self.api.request(
            "GET", f"{self.args.gateway}/orders/{self.created['order']}/tracking",
            token=self.token)
        self.expect(status, (200,), payload)
        required = ("driverId", "driverName", "driverContact", "driverRating",
                    "vehicleType", "vehiclePlate", "driverLatitude", "driverLongitude",
                    "etaMinutes", "remainingSeconds", "progressPercent", "timeline",
                    "customerLatitude", "customerLongitude")
        if not isinstance(payload, dict) or any(key not in payload for key in required):
            raise AssertionError(f"incomplete tracking response: {payload}")
        if payload.get("simulated") is not True:
            raise AssertionError(f"test tracking must be explicitly marked simulated: {payload}")
        if payload.get("status") != "ASSIGNED" or payload.get("etaMinutes") != 3:
            raise AssertionError(f"tracking did not start assigned with three-minute ETA: {payload}")
        status, advanced = self.api.request(
            "POST", f"{self.args.orders}/orders/{self.created['order']}/status",
            {"status": "DELIVERED"})
        self.expect(status, (200,), advanced)
        status, delivered = self.api.request(
            "GET", f"{self.args.gateway}/orders/{self.created['order']}/tracking",
            token=self.token)
        self.expect(status, (200,), delivered)
        if delivered.get("status") != "DELIVERED" or delivered.get("progressPercent") != 100:
            raise AssertionError(f"delivered status was not persisted: {delivered}")
        return f"driver {payload['driverId']} assigned -> delivered, ETA 3 -> 0 min"

    def notifications(self) -> str:
        status, payload = self.api.request("GET", f"{self.args.notifications}/notifications")
        self.expect(status, (200,), payload)
        if "user" not in self.created:
            raise SkipTest("requires successful user setup")
        matches = [item for item in (payload or []) if item.get("userId") == self.created["user"]]
        if not matches:
            raise AssertionError("no notification produced for test user")
        return f"{len(matches)} notification(s) observed"

    def negative_auth(self) -> str:
        status, payload = self.api.request("GET", f"{self.args.gateway}/users")
        proxy_rejected = status == 200 and "authorization" in str(payload).lower()
        if status not in (401, 403) and not proxy_rejected:
            raise AssertionError(f"protected users route accepted no token: HTTP {status}, {payload}")
        return f"request rejected without JWT (gateway HTTP {status})"

    def payment_idempotency(self) -> str:
        if "order" not in self.created or "user" not in self.created:
            raise SkipTest("requires successful order and user setup")
        key = f"foodservice-e2e-{uuid.uuid4().hex}"
        body = {"orderId": self.created["order"], "userId": self.created["user"],
                "amount": 12.34, "paymentMethod": "TEST"}
        headers = {"Idempotency-Key": key}
        first_status, first = self.api.request("POST", f"{self.args.payments}/payments",
                                               body, extra_headers=headers)
        second_status, second = self.api.request("POST", f"{self.args.payments}/payments",
                                                 body, extra_headers=headers)
        self.expect(first_status, (200, 201), first)
        self.expect(second_status, (200, 201), second)
        if not isinstance(first, dict) or not isinstance(second, dict):
            raise AssertionError(f"expected payment objects: {first}, {second}")
        first_payment = first.get("payment")
        second_payment = second.get("payment")
        if not isinstance(first_payment, dict) or not isinstance(second_payment, dict):
            raise AssertionError(f"responses did not contain payment objects: {first}, {second}")
        if first_payment.get("id") != second_payment.get("id"):
            raise AssertionError(f"idempotency key created two payments: {first}, {second}")
        self.idempotent_payment_id = int(first_payment["id"])
        self.test_transaction_id = str(first_payment["transactionId"])
        return f"same payment id {first_payment['id']} returned twice"

    def realtime_payment_snapshot(self) -> str:
        if "order" not in self.created:
            raise SkipTest("requires successful order setup")
        status, payment = self.api.request(
            "GET", f"{self.args.payments}/payments/order/{self.created['order']}")
        self.expect(status, (200,), payment)
        if not isinstance(payment, dict) or payment.get("orderId") != self.created["order"]:
            raise AssertionError(f"unexpected order payment: {payment}")
        status, stream = self.api.request(
            "GET", f"{self.args.payments}/payments/stream?orderId={self.created['order']}")
        self.expect(status, (200,), stream)
        if not isinstance(stream, str) or "data:" not in stream:
            raise AssertionError(f"expected SSE data frame: {stream}")
        return f"order lookup and SSE snapshot returned {payment.get('status')}"

    def reject_unsigned_webhook(self) -> str:
        status, payload = self.api.request(
            "POST", f"{self.args.payments}/payments/webhooks/provider",
            {"transactionId": "not-a-real-transaction", "status": "succeeded"})
        if status not in (400, 401, 403, 404):
            raise AssertionError(f"unsigned/unknown webhook was accepted: HTTP {status}, {payload}")
        return f"HTTP {status}; no mutation accepted"

    def test_webhook_transition(self) -> str:
        if not self.test_transaction_id:
            raise SkipTest("requires successful idempotent test payment")
        if not self.args.webhook_secret:
            raise SkipTest("set --webhook-secret for authenticated local callback test")
        headers = {"X-Webhook-Secret": self.args.webhook_secret}
        status, payload = self.api.request(
            "POST", f"{self.args.payments}/payments/webhooks/provider",
            {"transactionId": self.test_transaction_id, "status": "processing",
             "providerPaymentId": f"test_{uuid.uuid4().hex[:12]}"},
            extra_headers=headers)
        self.expect(status, (200,), payload)
        status, payload = self.api.request(
            "POST", f"{self.args.payments}/payments/webhooks/provider",
            {"transactionId": self.test_transaction_id, "status": "succeeded"},
            extra_headers=headers)
        self.expect(status, (200,), payload)
        if not isinstance(payload, dict) or payload.get("status") != "succeeded":
            raise AssertionError(f"payment did not reach succeeded: {payload}")
        return "pending -> processing -> succeeded"

    def cleanup(self) -> None:
        if self.args.keep_data:
            return
        if self.idempotent_payment_id is not None:
            self.api.request("DELETE", f"{self.args.payments}/payments/{self.idempotent_payment_id}")
        targets = [
            ("payment", self.args.payments, "payments", None),
            ("order", self.args.gateway, "orders", None),
            ("restaurant", self.args.gateway, "restaurants", None),
            ("user", self.args.gateway, "users", self.token),
        ]
        for key, base, resource, token in targets:
            if key in self.created:
                self.api.request("DELETE", f"{base}/{resource}/{self.created[key]}", token=token)

    def execute(self) -> int:
        services = (("gateway", self.args.gateway), ("users", self.args.users),
                    ("restaurants", self.args.restaurants), ("orders", self.args.orders),
                    ("payments", self.args.payments), ("notifications", self.args.notifications))
        for name, base in services:
            self.run(f"health:{name}", lambda n=name, b=base: self.health(n, b))
        self.run("auth:register", self.register)
        self.run("auth:login", self.login)
        self.run("auth:reject-missing-token", self.negative_auth)
        self.run("users:list", self.users)
        self.run("auth:current-user", self.current_user)
        self.run("orders:reject-anonymous", self.reject_anonymous_order)
        self.run("restaurants:create-and-list", self.restaurant)
        self.run("orders:reject-outside-delivery-zone", self.reject_outside_delivery_zone)
        self.run("orders:payment-eventual-consistency", self.order_and_payment)
        self.run("orders:live-delivery-tracking", self.delivery_tracking)
        self.run("payments:idempotency", self.payment_idempotency)
        self.run("payments:realtime-snapshot", self.realtime_payment_snapshot)
        self.run("payments:reject-unsigned-webhook", self.reject_unsigned_webhook)
        self.run("payments:test-webhook-transition", self.test_webhook_transition)
        self.run("notifications:payment-event", self.notifications)
        self.cleanup()

        for result in self.results:
            detail = f" - {result.detail}" if result.detail else ""
            print(f"[{result.outcome:4}] {result.name}{detail}")
        failed = sum(item.outcome == "FAIL" for item in self.results)
        skipped = sum(item.outcome == "SKIP" for item in self.results)
        print(f"\n{len(self.results) - failed - skipped} passed, {failed} failed, {skipped} skipped")
        return 1 if failed else 0


class SkipTest(Exception):
    pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="FoodService local E2E test harness")
    parser.add_argument("--gateway", default="http://127.0.0.1:8085")
    parser.add_argument("--users", default="http://127.0.0.1:8080")
    parser.add_argument("--restaurants", default="http://127.0.0.1:8081")
    parser.add_argument("--orders", default="http://127.0.0.1:8082")
    parser.add_argument("--payments", default="http://127.0.0.1:8083")
    parser.add_argument("--notifications", default="http://127.0.0.1:8084")
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--eventual-timeout", type=float, default=15.0)
    parser.add_argument("--poll-interval", type=float, default=0.5)
    parser.add_argument("--keep-data", action="store_true", help="do not delete test records")
    parser.add_argument("--webhook-secret", default="test-webhook-secret",
                        help="local test webhook secret; pass an empty value to skip transition test")
    return parser.parse_args()


if __name__ == "__main__":
    sys.exit(Suite(parse_args()).execute())
