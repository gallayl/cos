<#
.SYNOPSIS
    Compile all COS Berry scripts as a host-side regression gate (R5).

.DESCRIPTION
    Validates that Berry source files compile with the pinned Berry compiler.
    This catches syntax and compile errors before deployment.

.PARAMETER BerryCompiler
    Path to the Berry executable (berry.exe / berry). If omitted, uses
    $env:BERRY_EXE, then searches common build locations under build/berry-source,
    then PATH.

.PARAMETER Root
    Repository root. Defaults to the script's parent directory.

.PARAMETER Path
    One or more directories/files to validate (relative to Root or absolute).
    Defaults to storage/flash_data.

.PARAMETER SkipIfCompilerMissing
    Exit 0 with a warning if no Berry compiler is found. Useful for local
    environments that haven't built the host Berry compiler yet; CI gates
    should NOT use this flag.

.EXAMPLE
    ./scripts/validate-berry-scripts.ps1 -BerryCompiler build/berry-source/berry.exe

.EXAMPLE
    $env:BERRY_EXE = 'C:\tools\berry.exe'; ./scripts/validate-berry-scripts.ps1
#>
[CmdletBinding()]
param(
    [string]$BerryCompiler,
    [string]$Root,
    [string[]]$Path,
    [switch]$SkipIfCompilerMissing
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not $Root) {
    $Root = Split-Path -Parent $PSScriptRoot
}
$Root = (Resolve-Path -LiteralPath $Root).Path

function Find-BerryCompiler {
    param([string]$Explicit)
    if ($Explicit) {
        if (Test-Path -LiteralPath $Explicit) { return (Resolve-Path -LiteralPath $Explicit).Path }
        throw "Berry compiler not found: $Explicit"
    }
    if ($env:BERRY_EXE -and (Test-Path -LiteralPath $env:BERRY_EXE)) {
        return (Resolve-Path -LiteralPath $env:BERRY_EXE).Path
    }
    $candidates = @(
        (Join-Path $Root 'build\berry-host-cos\bin\berry.exe'),
        (Join-Path $Root 'build\berry-host-cos\berry.exe'),
        (Join-Path $Root 'build\berry-source\berry.exe'),
        (Join-Path $Root 'build\berry-source\berry'),
        (Join-Path $Root 'build\berry-source\build\berry.exe'),
        (Join-Path $Root 'build\berry-source\build\Release\berry.exe'),
        (Join-Path $Root 'build\berry-source\out\berry.exe')
    )
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c) { return (Resolve-Path -LiteralPath $c).Path }
    }
    $cmd = Get-Command berry -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $cmd = Get-Command berry.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$compiler = Find-BerryCompiler -Explicit $BerryCompiler
if (-not $compiler) {
    $msg = @(
        'Berry compiler not found. Build the pinned Berry host compiler from build/berry-source'
        '(see components/berry_lang/CMakeLists.txt for the pinned revision), or pass -BerryCompiler / set BERRY_EXE.'
        'Example (after ESP-IDF configure creates build/berry-source): build Berry with its CMake/Makefile, then rerun.'
    ) -join ' '
    if ($SkipIfCompilerMissing) {
        Write-Warning "SKIP: $msg"
        exit 0
    }
    Write-Error $msg
    exit 2
}

if (-not $Path -or $Path.Count -eq 0) {
    $Path = @('storage/flash_data')
}

$files = New-Object System.Collections.Generic.List[string]
foreach ($p in $Path) {
    $full = if ([System.IO.Path]::IsPathRooted($p)) { $p } else { Join-Path $Root $p }
    if (-not (Test-Path -LiteralPath $full)) {
        Write-Warning "Path not found, skipping: $full"
        continue
    }
    $item = Get-Item -LiteralPath $full
    if ($item.PSIsContainer) {
        Get-ChildItem -LiteralPath $full -Recurse -Filter '*.be' -File | ForEach-Object { $files.Add($_.FullName) }
    } elseif ($item.Extension -ieq '.be') {
        $files.Add($item.FullName)
    }
}

$files = $files | Sort-Object -Unique
if ($files.Count -eq 0) {
    Write-Warning 'No .be files found to validate.'
    exit 0
}

Write-Host "R5 Berry compile validation: compiler=$compiler files=$($files.Count)"

$tempOut = Join-Path ([System.IO.Path]::GetTempPath()) ("cos-berry-validate-$PID.bec")
$tempSrc = Join-Path ([System.IO.Path]::GetTempPath()) ("cos-berry-validate-$PID.be")
# COS firmware registers these globals/native entry points at runtime. Host
# compilation with strict/named-global checks needs them declared so we validate
# syntax/compile structure without false undeclared-global failures.
$cosPrelude = 'var cos_import, cos_module, action, log, console, fs, net, ui, json;' + "`n"
$cosPreludeLines = 1
$failed = New-Object System.Collections.Generic.List[string]
$passed = 0

try {
    foreach ($file in $files) {
        $relative = $file
        if ($file.StartsWith($Root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $file.Substring($Root.Length).TrimStart('\', '/')
        }
        # Compile a temp wrapper with COS global declarations, preserving the
        # original file content for accurate syntax checks.
        $source = [System.IO.File]::ReadAllText($file)
        [System.IO.File]::WriteAllText($tempSrc, $cosPrelude + $source)
        $output = & $compiler -c $tempSrc -o $tempOut 2>&1
        $code = $LASTEXITCODE
        if (Test-Path -LiteralPath $tempOut) { Remove-Item -LiteralPath $tempOut -Force -ErrorAction SilentlyContinue }
        if ($code -ne 0) {
            Write-Host "FAIL: $relative (exit $code)" -ForegroundColor Red
            if ($output) {
                foreach ($line in $output) {
                    $text = [string]$line
                    $escapedTemp = [regex]::Escape($tempSrc)
                    $m = [regex]::Match($text, "(?<prefix>.*)$escapedTemp`:(?<line>\d+):(?<rest>.*)")
                    if ($m.Success) {
                        $origLine = [int]$m.Groups['line'].Value - $cosPreludeLines
                        if ($origLine -lt 1) { $origLine = 1 }
                        $text = $m.Groups['prefix'].Value + $file + ':' + $origLine + ':' + $m.Groups['rest'].Value
                    } else {
                        $text = $text.Replace($tempSrc, $file)
                    }
                    Write-Host "  $text"
                }
            }
            $failed.Add($relative)
        } else {
            $passed++
        }
    }
} finally {
    if (Test-Path -LiteralPath $tempOut) { Remove-Item -LiteralPath $tempOut -Force -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $tempSrc) { Remove-Item -LiteralPath $tempSrc -Force -ErrorAction SilentlyContinue }
}

Write-Host "Berry compile validation: passed=$passed failed=$($failed.Count) total=$($files.Count)"
if ($failed.Count -gt 0) {
    Write-Host 'Failed files:' -ForegroundColor Red
    $failed | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    exit 1
}
exit 0
