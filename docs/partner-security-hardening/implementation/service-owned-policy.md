# Implementation Plan: Service-Owned Partner Authorization

## Selected Design And Constraints

The user requested implementation. Use service-owned membership checks with gateway defense in depth. Preserve customer reads.

## Source Revision And Drift Check

Base ee225bd; CRLF-only workspace drift was identified before changes.

## Affected Components

Common policy, Restaurant service/schema, API Gateway, partner frontend, tests and docs.

## Ordered Work Packages

1. Policy and exhaustive unit matrix.
2. Add membership, lifecycle, version, idempotency, audit and outbox migrations.
3. Add authenticated v1 partner routes and strict schemas.
4. Add gateway auth/rate limits and remove public writes.
5. Connect portal; add order projection and events.
6. Backfill, reconcile, canary and disable legacy routes.

## Compatibility And Migration

Legacy customer GET remains. Existing restaurants require reviewed owner/status assignment. Use feature flags and dual reads.

## Tactical Protections During Migration

Block public writes at gateway, isolate service ports, restrictive CORS, request limits and alerts.

## Tests And Security Validation

Execute the full catalogue in PARTNER_PORTAL_TEST_CASES.md plus penetration and dependency/secret scans.

## Performance And Resource Benchmarks

Measure p50/p95/p99, throughput, DB wait, CPU and RSS for realistic reads/writes and event backlog.

## Rollout And Rollback

Canary internal partners, then cohorts. Rollback disables new writes while preserving new data and audit events.

## Acceptance Criteria

No cross-restaurant write succeeds; no partner self-approves; retries are idempotent; stale writes conflict; secrets are absent from logs; backup/restore and target load pass.

## Open Decisions

Providers, retention, SLO/error budget, load target and admin approval policy.
