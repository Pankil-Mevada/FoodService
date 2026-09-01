# Security Hardening Proposal: Own Partner Authorization At The Resource Boundary

## Decision

Decide whether partner ownership is enforced only at the gateway or again by the Restaurant service against its membership data.

## Executive Recommendation

Option 1, gateway guards, is a useful temporary containment. Option 2, service-owned membership policy, makes the resource owner enforce the invariant. I recommend Option 2, with Option 1 retained during migration.

## Evidence

| Evidence | Finding | What it establishes |
| --- | --- | --- |
| E1 | Public restaurant mutation | Restaurant routes accept writes without an authenticated actor. |
| E2 | User-only token | Current JWT does not establish partner membership. |
| E3 | Public proxy | Gateway forwards unauthenticated restaurant writes. |
| E4 | Ownership-free schema | Persistence cannot answer whether actor owns resource. |

## Current Design And Failure Mode

Observed source shows that an internet caller can reach gateway restaurant mutations and the service has no actor or membership to evaluate. We infer that authorization is currently a convention rather than an enforceable invariant. That permits horizontal privilege escalation and accidental publication.

## Desired Invariants

- Every mutation authenticates an actor and authorizes the final restaurant resource.
- Customer, partner and admin capabilities are distinct.
- Partner routes cannot approve or unsuspend their own restaurant.
- Mutation and append-only audit/outbox event commit together.
- Replayed commands cannot create duplicate state.

## Constraints And Non-Goals

Keep the customer API available during migration. This proposal does not choose KYC, storage or payout vendors.

## Before Architecture

See [before diagram](../diagrams/partner-resource-authorization-before.mmd). The privileged service has no owned policy boundary.

## Options

### Option 1: Gateway guards

The gateway validates JWT and membership before proxying. It is fast to introduce and immediately narrows internet exposure. What gives me pause is that the service still accepts an unsigned direct call, and policy can drift between routes.

See [after diagram](../diagrams/partner-resource-authorization-gateway-guards-after.mmd).

| Change | Before | After | Security consequence | Cost |
| --- | --- | --- | --- | --- |
| Authorization | None | Gateway check | Narrows public attack path | Gateway owns business policy |
| Private service | Trusts caller | Still trusts caller | Bypass remains | Network isolation required |

### Option 2: Service-owned membership policy

The Restaurant service verifies identity context, resolves membership for the final ID, evaluates one policy and writes state plus audit event transactionally. The strongest case is that accidental direct exposure does not remove authorization. It adds membership queries and migration work; indexes, short-lived caching and load tests should bound that cost.

See [after diagram](../diagrams/partner-resource-authorization-service-policy-after.mmd).

| Change | Before | After | Security consequence | Cost |
| --- | --- | --- | --- | --- |
| Policy owner | No owner | Restaurant service | One authoritative decision | Schema and code migration |
| State write | Restaurant only | Restaurant plus audit/outbox | Traceable and replayable | Transaction/index overhead |
| Admin action | Same surface | Separate permission | Blocks self-approval | Admin identity required |

## Comparison

| Dimension | Gateway guards | Service-owned policy |
| --- | --- | --- |
| Security | Improves; direct bypass remains | Strong improvement; final resource checked |
| Performance | One gateway lookup | One indexed service lookup; benchmark needed |
| Memory | Neutral | Small policy/cache overhead |
| Reliability | Gateway dependency | Service and membership DB dependency |
| Operability | Simple but duplicated policy | More migrations and clearer audit |
| Migration | Fast | Incremental owner backfill required |

## Recommendation

I recommend Option 2 under the launch constraint. Option 1 should still be deployed as defense in depth. Gateway-only enforcement becomes acceptable only for a short migration with proven network isolation and a removal date.

## Evidence Coverage And Residual Risk

Option 2 addresses E1-E4 structurally. Tactical route blocking remains necessary during migration. Compromised owner accounts, malicious uploads, insider admin abuse and provider failures remain and need the controls in the partner architecture document.

## Migration And Rollout

Add lifecycle/version/membership/audit tables, backfill owners, introduce authenticated v1 routes, dual-read, disable legacy writes, then remove them. Roll back by disabling new routes, never by reopening public mutation.

## Validation Plan

Run the authorization matrix, ID enumeration, replay, concurrency, service-direct, migration, audit-redaction and load tests. Compare p95 write latency before and after on the same dataset; investigate if policy adds more than 100 ms at target concurrency.

## Implementation Work Packages

Central policy and tests; schema/migration; authenticated service routes; gateway defense; portal API; audit/outbox; legacy removal; security/load/accessibility validation.

## Open Questions

Identity provider, admin dual-control, owner backfill approval, compliance providers, retention periods and peak workload.
