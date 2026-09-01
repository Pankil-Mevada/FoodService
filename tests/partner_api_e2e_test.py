#!/usr/bin/env python3
"""Black-box restaurant partner API test for a running local FoodService stack."""

import json
import sys
import time
import urllib.error
import urllib.request

BASE_URL = "http://127.0.0.1:8085"
PASSWORD = "Partner#12345"


def request(method, path, body=None, token=None, headers=None, expected=200):
    request_headers = {"Accept": "application/json", **(headers or {})}
    data = None
    if body is not None:
        request_headers["Content-Type"] = "application/json"
        data = json.dumps(body).encode("utf-8")
    if token:
        request_headers["Authorization"] = f"Bearer {token}"
    req = urllib.request.Request(
        BASE_URL + path, data=data, headers=request_headers, method=method
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as response:
            status = response.status
            payload = response.read().decode("utf-8")
    except urllib.error.HTTPError as error:
        status = error.code
        payload = error.read().decode("utf-8")
    assert status == expected, f"{method} {path}: expected {expected}, got {status}: {payload}"
    return json.loads(payload) if payload else None


def register_and_login(name, email):
    request("POST", "/register", {"name": name, "email": email, "password": PASSWORD}, expected=201)
    result = request("POST", "/login", {"email": email, "password": PASSWORD})
    assert result.get("token"), "Login did not return a JWT"
    return result["token"]


def main():
    case_id = time.time_ns()
    owner_token = register_and_login("Partner Owner", f"partner.owner.{case_id}@example.test")
    other_token = register_and_login("Other Partner", f"partner.other.{case_id}@example.test")

    request("GET", "/partner/restaurants", expected=401)
    owner_empty = request("GET", "/partner/restaurants", token=owner_token)
    other_empty = request("GET", "/partner/restaurants", token=other_token)
    assert owner_empty == [] and other_empty == [], "New partners must receive []"

    created = request(
        "POST",
        "/partner/restaurants",
        {
            "name": "Isolation Test Kitchen",
            "phone": "9999999999",
            "address": "1 Test Road, Bengaluru",
            "latitude": 12.9716,
            "longitude": 77.5946,
            "deliveryRadiusKm": 8,
            "preparationMinutes": 20,
            "baseDeliveryFee": 39,
            "perKmFee": 5,
            "imageUrl": "https://example.test/kitchen.jpg",
        },
        token=owner_token,
        headers={"X-Correlation-ID": f"partner-e2e-{case_id}"},
        expected=201,
    )
    restaurant_id = created["restaurantId"]
    assert created["status"] == "DRAFT"
    assert len(request("GET", "/partner/restaurants", token=owner_token)) == 1
    assert request("GET", "/partner/restaurants", token=other_token) == []
    request("GET", f"/partner/restaurants/{restaurant_id}", token=other_token, expected=404)

    public_rows = request("GET", "/restaurants")
    assert not any(row.get("id") == restaurant_id for row in public_rows)
    assert request("GET", f"/partner/restaurants/{restaurant_id}/menu-items", token=owner_token) == []

    request(
        "POST",
        f"/partner/restaurants/{restaurant_id}/menu-items",
        {
            "name": "Launch Dosa",
            "description": "Partner integration test item",
            "pricePaise": 17900,
            "dietType": "VEG",
            "available": True,
        },
        token=owner_token,
        headers={"Idempotency-Key": f"partner-menu-{case_id}"},
        expected=201,
    )
    submitted = request(
        "POST",
        f"/partner/restaurants/{restaurant_id}/submit",
        {"version": 1},
        token=owner_token,
        headers={"Idempotency-Key": f"partner-submit-{case_id}"},
    )
    assert submitted["status"] == "PENDING_REVIEW"
    public_rows = request("GET", "/restaurants")
    assert not any(row.get("id") == restaurant_id for row in public_rows)
    assert len(request("GET", f"/partner/restaurants/{restaurant_id}/audit", token=owner_token)) >= 3

    print(
        "PASS partner API: empty arrays, JWT, tenant isolation, private draft, "
        f"menu, submission and audit (restaurant {restaurant_id})"
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"FAIL partner API: {error}", file=sys.stderr)
        raise
