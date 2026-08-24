[CmdletBinding()]
param(
    [string]$DriverToken = 'local-driver-test-token',
    [string]$OrderSyncSecret = 'local-order-sync-secret'
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $repoRoot
$missing = @()
foreach ($name in 'RAZORPAY_KEY_ID', 'RAZORPAY_KEY_SECRET', 'RAZORPAY_WEBHOOK_SECRET') {
    if (-not [Environment]::GetEnvironmentVariable($name, 'Process')) { $missing += $name }
}
if ($missing.Count -gt 0) {
    throw "Missing environment variable(s): $($missing -join ', '). Run this script in the PowerShell window where you configured the Razorpay Test keys."
}
if (-not $env:RAZORPAY_KEY_ID.StartsWith('rzp_test_')) {
    throw 'RAZORPAY_KEY_ID is not a Test Mode key (expected rzp_test_ prefix).'
}
$env:DRIVER_LOCATION_TOKEN = $DriverToken
$env:ORDER_SYNC_SECRET = $OrderSyncSecret
$requiredBridge = @('RAZORPAY_KEY_ID/u', 'RAZORPAY_KEY_SECRET/u', 'RAZORPAY_WEBHOOK_SECRET/u', 'DRIVER_LOCATION_TOKEN/u', 'ORDER_SYNC_SECRET/u')
$existingBridge = @($env:WSLENV -split ':' | Where-Object { $_ })
$env:WSLENV = (@($existingBridge + $requiredBridge) | Select-Object -Unique) -join ':'
wsl bash /mnt/c/Users/Pankil/Documents/ChatGPT/FoodService/scripts/start-all.sh
if ($LASTEXITCODE -ne 0) { throw "WSL launcher failed with exit code $LASTEXITCODE." }
Write-Host ''
Write-Host 'Frontend: http://localhost:5173/'
Write-Host "Logs:     $repoRoot\.run"
