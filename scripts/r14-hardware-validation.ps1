<#
.SYNOPSIS
    Guided helper for R14 hardware validation (source-map/runtime mapping, runtime memory diagnostics, BE_DEBUG_RUNTIME_INFO evaluation).

.DESCRIPTION
    Creates an artifact directory, records environment/context, optionally captures firmware build size outputs,
    and prints an exact step-by-step checklist. This script does NOT modify firmware configuration, flash devices,
    or mark validation as passed.

.PARAMETER OutDir
    Directory for logs/artifacts (created if missing). Defaults to docs/reports/artifacts/r14-<timestamp>.

.PARAMETER Port
    Serial port for device monitor/log capture (informational/checklist), e.g. COM5.

.PARAMETER DeviceIp
    Device IP/hostname for Web IDE/console access (informational/checklist).

.PARAMETER FirmwareEval
    Include firmware size-evaluation helper steps (idf.py build/size outputs). Requires ESP-IDF environment.

.PARAMETER IdfPy
    idf.py command/invocation to use in firmware-eval mode. Default: "idf.py".

.PARAMETER SkipSizeCommands
    In -FirmwareEval mode, do not execute idf.py commands; only print them and record guidance.

.EXAMPLE
    ./scripts/r14-hardware-validation.ps1 -Port COM5 -DeviceIp 192.168.1.50

.EXAMPLE
    ./scripts/r14-hardware-validation.ps1 -FirmwareEval -IdfPy "idf.py" -OutDir docs/reports/artifacts/r14-firmware
#>
[CmdletBinding()]
param(
    [string]$OutDir,
    [string]$Port,
    [string]$DeviceIp,
    [switch]$FirmwareEval,
    [string]$IdfPy = "idf.py",
    [switch]$SkipSizeCommands
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $OutDir = Join-Path $repoRoot "docs\reports\artifacts\r14-$stamp"
}
if (-not [System.IO.Path]::IsPathRooted($OutDir)) {
    $OutDir = Join-Path $repoRoot $OutDir
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$outResolved = (Resolve-Path -LiteralPath $OutDir).Path

function Write-Section([string]$Title) {
    Write-Host "`n=== $Title ===" -ForegroundColor Cyan
}

function Save-Text([string]$Name, [string]$Content) {
    $path = Join-Path $outResolved $Name
    $Content | Set-Content -LiteralPath $path -Encoding UTF8
    Write-Host "Wrote: $path"
}

function Try-RunCapture([string]$Name, [scriptblock]$Command) {
    $path = Join-Path $outResolved $Name
    try {
        $output = & $Command 2>&1 | Out-String
        $output | Set-Content -LiteralPath $path -Encoding UTF8
        Write-Host "Captured: $path"
    } catch {
        "FAILED: $($_.Exception.Message)" | Set-Content -LiteralPath $path -Encoding UTF8
        Write-Warning "Command failed for $Name (details saved): $($_.Exception.Message)"
    }
}

Write-Section "R14 Hardware Validation Helper"
Write-Host "Repo root: $repoRoot"
Write-Host "Artifacts: $outResolved"
if ($Port) { Write-Host "Serial port: $Port" }
if ($DeviceIp) { Write-Host "Device IP: $DeviceIp" }

$meta = @()
$meta += "timestamp=$(Get-Date -Format o)"
$meta += "repoRoot=$repoRoot"
$meta += "outDir=$outResolved"
$meta += "port=$Port"
$meta += "deviceIp=$DeviceIp"
$meta += "firmwareEval=$($FirmwareEval.IsPresent)"
$meta += "idfPy=$IdfPy"
try { $meta += "gitCommit=$(git -C $repoRoot rev-parse HEAD 2>$null)" } catch { $meta += "gitCommit=" }
try { $meta += "gitStatus=$(git -C $repoRoot status --short 2>$null | Measure-Object -Line | Select-Object -ExpandProperty Lines) changed lines" } catch { $meta += "gitStatus=" }
Save-Text "context.txt" ($meta -join [Environment]::NewLine)

$guide = Join-Path $repoRoot "docs\reports\2026-11-10-r14-hardware-validation-guide.md"
if (Test-Path -LiteralPath $guide) {
    Copy-Item -LiteralPath $guide -Destination (Join-Path $outResolved "GUIDE.copy.md") -Force
    Write-Host "Copied guide: docs/reports/2026-11-10-r14-hardware-validation-guide.md"
} else {
    Write-Warning "Guide not found at expected path: $guide"
}

$checklist = @"
R14 Hardware Validation Checklist (record pass/fail and notes)

PART A - Source-map/runtime error mapping (real device output)
[ ] A1 Prep: frontend served, device auth OK, editor shows dev profile + mappings > 0, serial log capture started
    - Firmware commit/build: ____________________
    - Frontend/browser: _________________________
    - Device/port/IP: ${Port} / ${DeviceIp}
[ ] A2 Compile/load/runtime errors captured (>=2-3 cases): serial + console + editor observations saved
    - Case 1: ____________________  format: ____________________  manual annotate: ____  auto annotate: ____
    - Case 2: ____________________  format: ____________________  manual annotate: ____  auto annotate: ____
    - Case 3: ____________________  format: ____________________  manual annotate: ____  auto annotate: ____
[ ] A3 Runtime app exception/raise mapped to correct TS source (sample captured)
[ ] A4 Source-map fidelity spot checks (nested expr/calls/inline/control flow) documented
[ ] A5 Raw unrecognized formats/mismatches saved for follow-up

PART B - Editor runtime memory diagnostics (/api/heap)
[ ] B1 Refresh shows free heap bytes + formatted value + raw response + timestamp (auth OK, device reachable)
[ ] B2 Auto-refresh (5s) updates/stable during short observation
[ ] B3 Unauthorized handled (401/sign-in state) and unreachable handled (error state) gracefully
[ ] B4 Compared with serial/shell metrics if available (sanity): ______________________________

PART C - BE_DEBUG_RUNTIME_INFO / prod firmware evaluation (optional, measure before any config change)
[ ] C1 Baseline recorded: commit, berry_conf.h values, idf.py size/components, boot/runtime heap, error/trace samples
[ ] C2 Candidate (temporary local change only) measured with identical environment; config restored unless decided
[ ] C3 Comparison documented: size/heap/error quality/R14 mapping impact; decision/recommendation recorded

RESULT SUMMARY
- Overall: PENDING / PASS / FAIL (with caveats)
- Key findings:
  1. __________________________________________
  2. __________________________________________
- Follow-ups:
  1. __________________________________________
  2. __________________________________________
"@
Save-Text "checklist.txt" $checklist

Write-Section "Exact execution steps"
Write-Host @"
1. PREPARE
   - Flash/run current firmware on target device and start serial logging.
     Suggested monitor command:
       idf.py -p ${Port} monitor
     Save serial output to: $outResolved\serial.log (or separate per-case logs).
   - Serve/open frontend editor and sign in to device (HTTP Basic auth).
   - Confirm editor shows TypeScript compile in dev profile with mappings > 0.
   - Confirm GET /api/heap works when authenticated (editor Runtime memory diagnostics -> Refresh).

2. PART A (source-map/runtime error mapping)
   - Compile TS samples that produce device/runtime Berry errors with line references.
   - For each case capture: TS source, generated Berry (if available), serial output, console transcript, editor state.
   - Test manual annotator (paste error text) and console auto-annotation behavior.
   - Save raw error strings/formats, including any not annotated correctly.

3. PART B (runtime memory diagnostics)
   - Validate manual refresh, auto-refresh (5s), unauthorized and unreachable handling in editor panel.
   - Optionally compare with 'berry profile' / R5 [R5] logs on serial.

4. PART C (optional firmware debug/profile evaluation)
   - Baseline: record berry_conf.h (BE_DEBUG_RUNTIME_INFO etc.), run build+size, capture runtime/error samples.
   - Candidate: TEMPORARILY change BE_DEBUG_RUNTIME_INFO locally for measurement only, rebuild/reflash/retest, then restore.
   - Compare size/heap/error quality and R14 mapping usefulness. Do not commit config changes without measured decision.

5. REPORT
   - Fill checklist.txt and create docs/reports/2026-11-10-r14-hardware-results.md (or dated results file).
   - Attach artifact paths and raw samples for unrecognized error formats.
"@

if ($FirmwareEval) {
    Write-Section "Firmware evaluation helpers (Part C)"
    $confPath = Join-Path $repoRoot "components\berry_lang\src\berry_conf.h"
    if (Test-Path -LiteralPath $confPath) {
        Try-RunCapture "berry_conf.snapshot.txt" { Get-Content -LiteralPath $confPath }
    } else {
        Write-Warning "berry_conf.h not found: $confPath"
    }

    $commands = @(
        "$IdfPy build",
        "$IdfPy size",
        "$IdfPy size-components"
    )
    Save-Text "firmware-eval-commands.txt" (($commands -join [Environment]::NewLine) + [Environment]::NewLine + `
        [Environment]::NewLine + "Run in ESP-IDF environment from repo root. For comparable measurements consider: $IdfPy fullclean" + `
        [Environment]::NewLine + "WARNING: Do not modify/commit BE_DEBUG_RUNTIME_INFO without measured decision.")

    if ($SkipSizeCommands) {
        Write-Host "SkipSizeCommands set: not executing idf.py commands. See firmware-eval-commands.txt"
    } else {
        Write-Host "Attempting to run idf.py commands (requires ESP-IDF environment)..."
        Push-Location $repoRoot
        try {
            Try-RunCapture "idfpy-build.txt" { & cmd /c "$IdfPy build" }
            Try-RunCapture "idfpy-size.txt" { & cmd /c "$IdfPy size" }
            Try-RunCapture "idfpy-size-components.txt" { & cmd /c "$IdfPy size-components" }
        } finally {
            Pop-Location
        }
    }
}

Write-Section "Done"
Write-Host "Artifacts/checklist prepared in: $outResolved"
Write-Host "Guide: docs/reports/2026-11-10-r14-hardware-validation-guide.md"
Write-Host "Next: perform hardware steps, save logs/samples, and complete checklist/results report." -ForegroundColor Green
