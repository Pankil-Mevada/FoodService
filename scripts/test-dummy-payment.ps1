[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$OrderId,

    [ValidateSet('processing', 'succeeded', 'failed', 'cancelled')]
    [string]$Status = 'succeeded',

    [string]$GatewayUrl = 'http://localhost:8085',

    [string]$WebhookSecret = 'test-webhook-secret'
)

$ErrorActionPreference = 'Stop'
$gateway = $GatewayUrl.TrimEnd('/')

try {
    Write-Host "Looking up payment for order #$OrderId..." -ForegroundColor Cyan
    $payment = Invoke-RestMethod `
        -Uri "$gateway/payments/order/$OrderId" `
        -Method Get `
        -TimeoutSec 10

    if ($payment -is [string]) {
        throw "Payment lookup returned '$payment'. Confirm the order has a pending payment."
    }

    if (-not $payment.transactionId) {
        throw "No payment exists for order #$OrderId. Create a new order with an amount from 1 to 1000000, then use its order number."
    }

    Write-Host "Current payment status: $($payment.status)" -ForegroundColor Yellow

    $payload = @{
        transactionId     = $payment.transactionId
        status            = $Status
        providerPaymentId = "dummy-payment-order-$OrderId"
    } | ConvertTo-Json

    $updated = Invoke-RestMethod `
        -Uri "$gateway/payments/webhooks/provider" `
        -Method Post `
        -Headers @{ 'X-Webhook-Secret' = $WebhookSecret } `
        -ContentType 'application/json' `
        -Body $payload `
        -TimeoutSec 10

    Write-Host "Dummy payment updated successfully." -ForegroundColor Green
    Write-Host "Order:       #$($updated.orderId)"
    Write-Host "Transaction: $($updated.transactionId)"
    Write-Host "Status:      $($updated.status)" -ForegroundColor Green
    Write-Host "Provider ID: $($updated.providerPaymentId)"
    Write-Host "The web UI should update automatically within 2-3 seconds."
}
catch {
    Write-Error "Dummy payment failed: $($_.Exception.Message)"
    exit 1
}
