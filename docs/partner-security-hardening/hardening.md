# Security Hardening Review: Restaurant Partner Writes

## Evidence Basis

I inspected the gateway, JWT and restaurant route/schema boundaries at revision ee225bd. Public mutation plus missing resource ownership creates one cross-cutting authorization opportunity. No exploit test was run.

## Constraints

We preserve the existing customer marketplace while migrating incrementally. Security, correctness and rollback outrank short-term convenience. External KYC and storage providers are not selected.

## Opportunity Portfolio

| Opportunity | Evidence | Options | Recommendation | Proposal |
| --- | --- | --- | --- | --- |
| Own partner authorization at the resource boundary | Public writes, user-only JWT, ownership-free schema (E1-E4) | Local gateway guards; service-owned membership policy | Service-owned policy | [Proposal](proposals/partner-resource-authorization.md) |

## Recommendation Summary

I recommend service-owned membership authorization. Gateway-only checks are attractive as a fast containment step, but they can drift and are bypassed if a private service is accidentally exposed. The service must authorize the final restaurant ID and record the decision in the same transactional boundary as the write.

## Next Decisions

Choose the production identity/KYC, object storage/scanning and event-bus providers; define admin dual-control; approve the legacy owner backfill.
