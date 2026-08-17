$ErrorActionPreference = 'Stop'
wsl bash /mnt/c/Users/Pankil/Documents/ChatGPT/FoodService/scripts/stop-all.sh
if ($LASTEXITCODE -ne 0) { throw "WSL stop script failed with exit code $LASTEXITCODE." }
Write-Host 'FoodService processes started by start-all.ps1 have been stopped.'
