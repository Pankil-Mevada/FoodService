param([switch]$Full)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    foreach ($command in @('git', 'node', 'python')) {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
            throw "Required tool '$command' is not installed or is not on PATH"
        }
    }

    Write-Host '[quality] checking whitespace'
    git diff --check
    if ($LASTEXITCODE -ne 0) { throw 'Whitespace validation failed' }

    Write-Host '[quality] rejecting newly staged generated or secret files'
    $addedFiles = git diff --cached --name-only --diff-filter=A
    $trackedBad = $addedFiles | Select-String '(^|/)(build|build-wsl|build-ci|\.run|vcpkg_installed)/|\.(db|db-shm|db-wal|pyc)$|(^|/)\.env($|\.)'
    if ($trackedBad) { throw "Generated or secret files must not be committed:`n$trackedBad" }

    Write-Host '[quality] checking JavaScript syntax'
    Get-ChildItem frontend -Filter '*.js' | Sort-Object Name | ForEach-Object {
        node --check $_.FullName
        if ($LASTEXITCODE -ne 0) { throw "JavaScript syntax failed: $($_.Name)" }
    }

    Write-Host '[quality] checking Python syntax'
    python -m py_compile tests/e2e_test.py tests/load_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Python syntax validation failed' }

    if (-not $Full) {
        Write-Host '[quality] fast checks passed'
        return
    }

    if (-not $env:VCPKG_ROOT) { throw 'Set VCPKG_ROOT to a bootstrapped vcpkg directory' }
    $buildDir = if ($env:CI_BUILD_DIR) { $env:CI_BUILD_DIR } else { 'build-ci' }
    cmake -S . -B $buildDir -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
    if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
    cmake --build $buildDir --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
    ctest --test-dir $buildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}
finally { Pop-Location }
