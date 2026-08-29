# Runtime logging

Every C++ microservice uses `RequestLoggingMiddleware` for one consistent,
single-line request completion record. Logs never include request bodies,
passwords, JWTs, Razorpay secrets, signatures, UPI values, or card data.

## Levels

| Level | Meaning |
|---|---|
| `DEBUG` | Request arrival and Crow diagnostic details when explicitly enabled. |
| `INFO` | Successful HTTP response (`100`–`399`) and normal domain flow. |
| `WARNING` | Client/auth/validation/conflict response (`400`–`499`). |
| `ERROR` | Server or dependency failure (`500`–`599`). |
| `CRITICAL` | Reserved for process-level failures requiring immediate attention. |

Each completion record includes `service`, `correlationId`, `method`, `path`,
`status`, and `durationMs`. API Gateway forwards its correlation ID, allowing a
browser request to be matched across service log files.

The default threshold is `INFO`. Before starting services, select a threshold:

```bash
export FOODSERVICE_LOG_LEVEL=DEBUG   # most detail
export FOODSERVICE_LOG_LEVEL=INFO    # normal development default
export FOODSERVICE_LOG_LEVEL=WARNING # warnings and errors only
export FOODSERVICE_LOG_LEVEL=ERROR   # errors only
```

## Background logs

After `scripts/start-all.sh`, follow all service output from PowerShell:

```powershell
Get-Content .run\users.log,.run\restaurants.log,.run\orders.log,.run\payments.log,.run\notifications.log,.run\gateway.log,.run\frontend.log -Wait -Tail 30
```

Follow only payment and gateway activity:

```powershell
Get-Content .run\payments.log,.run\gateway.log -Wait -Tail 50
```

Press `Ctrl+C` to stop following logs; it does not stop the services. Use
`scripts/stop-all.ps1` to stop the background stack.

