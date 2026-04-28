Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host
Write-Host "+------------------------------+"
Write-Host "|    FicTrac install (pixi)    |"
Write-Host "+------------------------------+"
Write-Host

Set-Location $PSScriptRoot

function Find-FicTracExecutable {
    $candidates = @(
        (Join-Path $PSScriptRoot "build\fictrac.exe"),
        (Join-Path $PSScriptRoot "build\Release\fictrac.exe"),
        (Join-Path $PSScriptRoot "build\Debug\fictrac.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -Path $candidate -PathType Leaf) {
            return $candidate
        }
    }

    return $null
}

# 1. Bootstrap pixi if missing (no admin rights required; installs to
#    %USERPROFILE%\.pixi\bin and updates the user PATH).
$pixi = Get-Command pixi -ErrorAction SilentlyContinue
if (-not $pixi) {
    Write-Host "+-- Installing pixi -----------+"
    & powershell -ExecutionPolicy Bypass -Command "irm -useb https://pixi.sh/install.ps1 | iex"

    $pixiBinDir = Join-Path $env:USERPROFILE ".pixi\bin"
    if ((Test-Path -Path $pixiBinDir) -and -not (($env:Path -split ";") -contains $pixiBinDir)) {
        $env:Path = "$pixiBinDir;$env:Path"
    }

    $pixi = Get-Command pixi -ErrorAction SilentlyContinue
    if (-not $pixi) {
        throw "Failed to install pixi. Add $pixiBinDir to PATH and retry."
    }
}

# 2. Resolve dependencies into .pixi\envs\default
Write-Host
Write-Host "+-- Resolving dependencies ----+"
Write-Host
& pixi install

# 3. Configure + build
Write-Host
Write-Host "+-- Building FicTrac ----------+"
Write-Host
& pixi run build

# 4. Smoke check
Write-Host
$fictracExe = Find-FicTracExecutable
if ($null -ne $fictracExe) {
    Write-Host "FicTrac built successfully -> $fictracExe"
}
else {
    throw "Build failed: fictrac.exe not found under .\build."
}
