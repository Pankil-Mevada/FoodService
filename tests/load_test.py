#!/usr/bin/env python3
"""Create and verify a burst of concurrent local FoodService test orders.

No provider checkout is opened and no money can move. Each accepted order must
be present in SQLite and have exactly one pending local payment record.
"""
from __future__ import annotations

import argparse
import asyncio
import json
import math
import statistics
import time
import urllib.error
import urllib.request
import uuid
from pathlib import Path


def request(method: str, url: str, body=None, token: str | None = None, timeout=30):
    data = json.dumps(body).encode() if body is not None else None
    headers = {"Accept": "application/json"}
    if data is not None:
        headers["Content-Type"] = "application/json"
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            raw = response.read().decode()
            return response.status, json.loads(raw) if raw else None
    except urllib.error.HTTPError as error:
        raw = error.read().decode()
        try:
            raw = json.loads(raw)
        except json.JSONDecodeError:
            pass
        return error.code, raw


async def post_order(host: str, port: int, token: str, payload: dict, start: asyncio.Event):
    await start.wait()
    began = time.perf_counter()
    try:
        reader, writer = await asyncio.wait_for(asyncio.open_connection(host, port), 20)
        body = json.dumps(payload, separators=(",", ":")).encode()
        head = (
            f"POST /orders HTTP/1.1\r\nHost: {host}:{port}\r\n"
            f"Authorization: Bearer {token}\r\nContent-Type: application/json\r\n"
            f"Content-Length: {len(body)}\r\nConnection: close\r\n\r\n"
        ).encode()
        writer.write(head + body)
        await writer.drain()
        header = await asyncio.wait_for(reader.readuntil(b"\r\n\r\n"), 120)
        status = int(header.split(b" ", 2)[1])
        length = 0
        for line in header.split(b"\r\n"):
            if line.lower().startswith(b"content-length:"):
                length = int(line.split(b":", 1)[1])
        raw = await asyncio.wait_for(reader.readexactly(length), 120) if length else await reader.read()
        writer.close()
        await writer.wait_closed()
        decoded = json.loads(raw) if raw else None
        ok = status in (200, 201) and isinstance(decoded, dict) and decoded.get("success") is True
        return ok, status, time.perf_counter() - began, decoded
    except Exception as error:
        return False, 0, time.perf_counter() - began, f"{type(error).__name__}: {error}"


def percentile(values: list[float], percent: float) -> float:
    if not values:
        return 0.0
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, math.ceil(percent * len(ordered)) - 1)]


async def run(args):
    marker = f"LOAD-{uuid.uuid4().hex[:12]}"
    email = f"{marker.lower()}@example.test"
    password = "LoadTestOnly123!"
    status, registration = request("POST", f"{args.gateway}/register",
        {"name": "FoodService Load Test", "email": email, "password": password})
    if status not in (200, 201) or not registration.get("success"):
        raise RuntimeError(f"registration failed: HTTP {status} {registration}")
    status, login = request("POST", f"{args.gateway}/login", {"email": email, "password": password})
    if status != 200 or not login.get("token"):
        raise RuntimeError(f"login failed: HTTP {status} {login}")
    token = login["token"]
    restaurant_name = f"{marker} Kitchen"
    restaurant_body = {"name": restaurant_name, "address": "Load Test Only", "phone": "TEST-1000",
        "rating": 4.5, "latitude": 23.0225, "longitude": 72.5714,
        "deliveryRadiusKm": 8.0, "imageUrl": "https://example.test/load.jpg"}
    status, created = request("POST", f"{args.gateway}/restaurants", restaurant_body)
    if status not in (200, 201) or not created.get("success"):
        raise RuntimeError(f"restaurant setup failed: HTTP {status} {created}")
    _, restaurants = request("GET", f"{args.gateway}/restaurants")
    restaurant_id = next(int(r["id"]) for r in restaurants if r.get("name") == restaurant_name)

    payloads = [{"restaurantId": restaurant_id, "totalAmount": 218.0,
        "deliveryLatitude": 23.024, "deliveryLongitude": 72.573,
        "deliveryAddress": f"{marker}-{index:04d}", "itemSummary": "Load Test Meal x 1",
        "subtotal": 199.0, "discountAmount": 20.0, "deliveryFee": 39.0}
        for index in range(args.orders)]
    start = asyncio.Event()
    tasks = [asyncio.create_task(post_order(args.host, args.port, token, item, start)) for item in payloads]
    wall_start = time.perf_counter()
    start.set()
    results = await asyncio.gather(*tasks)
    wall_seconds = time.perf_counter() - wall_start

    _, orders = request("GET", f"{args.orders_url}/orders", timeout=120)
    persisted = [o for o in orders if str(o.get("deliveryAddress", "")).startswith(marker)]
    persisted_ids = {int(o["id"]) for o in persisted}
    _, payments = request("GET", f"{args.payments_url}/payments", timeout=120)
    matching_payments = [p for p in payments if int(p.get("orderId", -1)) in persisted_ids]
    payment_order_ids = [int(p["orderId"]) for p in matching_payments]
    latencies = [r[2] for r in results]
    http_success = sum(1 for r in results if r[0])
    errors: dict[str, int] = {}
    for ok, status_code, _, detail in results:
        if not ok:
            key = f"HTTP {status_code}: {detail}"[:300]
            errors[key] = errors.get(key, 0) + 1
    summary = {
        "testMode": True, "marker": marker, "requested": args.orders,
        "concurrency": args.orders, "httpSuccess": http_success,
        "persistedOrders": len(persisted), "paymentRecords": len(matching_payments),
        "uniquePaymentOrders": len(set(payment_order_ids)),
        "duplicatePaymentRecords": len(payment_order_ids) - len(set(payment_order_ids)),
        "wallSeconds": round(wall_seconds, 3),
        "throughputOrdersPerSecond": round(http_success / wall_seconds, 2),
        "latencyMs": {"min": round(min(latencies) * 1000, 2),
            "mean": round(statistics.mean(latencies) * 1000, 2),
            "p50": round(percentile(latencies, .50) * 1000, 2),
            "p95": round(percentile(latencies, .95) * 1000, 2),
            "p99": round(percentile(latencies, .99) * 1000, 2),
            "max": round(max(latencies) * 1000, 2)},
        "errors": errors,
    }
    summary["passed"] = all((http_success == args.orders, len(persisted) == args.orders,
        len(matching_payments) == args.orders, len(set(payment_order_ids)) == args.orders))
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(f"Raw summary: {output.resolve()}")
    return 0 if summary["passed"] else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Verified concurrent local order load test")
    parser.add_argument("--orders", type=int, default=1000)
    parser.add_argument("--gateway", default="http://127.0.0.1:8085")
    parser.add_argument("--orders-url", default="http://127.0.0.1:8082")
    parser.add_argument("--payments-url", default="http://127.0.0.1:8083")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8085)
    parser.add_argument("--output", default="tests/results/load-1000-latest.json")
    raise SystemExit(asyncio.run(run(parser.parse_args())))
