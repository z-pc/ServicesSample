$ErrorActionPreference = 'Stop'

function Assert-Admin {
  $currentIdentity = [Security.Principal.WindowsIdentity]::GetCurrent()
  $principal = New-Object Security.Principal.WindowsPrincipal($currentIdentity)
  if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'This script must be run as Administrator (required for firewall + service installation).'
  }
}

Assert-Admin

# ==========================
# ServiceSample installer script
# - Opens firewall port
# - Creates/updates Windows Service
# - Configures recovery actions
# - Starts service
# - Self-deletes after success
# ==========================

# --- Service configuration (edit as needed) ---
$ServiceName  = 'ServiceSample'
$DisplayName  = 'ServiceSample'
$Description  = 'ServiceSample service'

$Port         = 8080

# Recovery options
$ResetPeriodSeconds = 86400  # 24h
$ApplyOnNonCrashFailures = $true

# Actions: first/second/subsequent failure
$FirstFailureAction  = 'restart'
$FirstFailureDelayMs = 5000
$SecondFailureAction  = 'restart'
$SecondFailureDelayMs = 10000
$SubsequentFailureAction  = 'restart'
$SubsequentFailureDelayMs = 30000

# --- Derived paths ---
$AppDir  = $PSScriptRoot
$ExePath = Join-Path $AppDir 'app_services.exe'

if (-not (Test-Path -LiteralPath $ExePath)) {
  throw "Executable not found: $ExePath"
}

# Ensure firewall rule exists for the port (program scoped)
$ruleName = "$ServiceName (TCP)"
$existing = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
if ($null -ne $existing) {
  $existing | Remove-NetFirewallRule | Out-Null
}

$fwParams = @{
  DisplayName = $ruleName
  Direction   = 'Inbound'
  Action      = 'Allow'
  Program     = $ExePath
  Protocol    = 'TCP'
  LocalPort   = $Port
}

New-NetFirewallRule @fwParams | Out-Null

# If service exists, stop + delete it first (fresh install semantics)
$svc = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
if ($null -ne $svc) {
  try {
    Stop-Service -Name $ServiceName -Force -ErrorAction SilentlyContinue
  } catch {}

  sc.exe delete "$ServiceName" | Out-Null

  # Wait a bit for SCM to finalize deletion
  $deadline = (Get-Date).AddSeconds(10)
  while ((Get-Date) -lt $deadline) {
    if ($null -eq (Get-Service -Name $ServiceName -ErrorAction SilentlyContinue)) { break }
    Start-Sleep -Milliseconds 200
  }
}

# Create service
$binPath = '"' + $ExePath + '" --service'

# Prefer New-Service (PowerShell-native). Recovery settings are applied via sc.exe below.
New-Service -Name $ServiceName -BinaryPathName $binPath -DisplayName $DisplayName -Description $Description -StartupType Automatic | Out-Null

# Verify service creation
if ($null -eq (Get-Service -Name $ServiceName -ErrorAction SilentlyContinue)) {
  throw "Service creation failed; service '$ServiceName' not found after New-Service."
}

# Configure recovery actions
$actions = "$FirstFailureAction/$FirstFailureDelayMs/$SecondFailureAction/$SecondFailureDelayMs/$SubsequentFailureAction/$SubsequentFailureDelayMs"
sc.exe failure "$ServiceName" reset= $ResetPeriodSeconds actions= $actions | Out-Null

if ($ApplyOnNonCrashFailures) {
  sc.exe failureflag "$ServiceName" 1 | Out-Null
} else {
  sc.exe failureflag "$ServiceName" 0 | Out-Null
}

# Start service
Start-Service -Name $ServiceName

# Self delete (spawn a separate PowerShell so deletion isn't blocked)
$scriptPath = $PSCommandPath
Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -ArgumentList @(
  '-NoProfile',
  '-ExecutionPolicy', 'Bypass',
  '-Command',
  "Start-Sleep -Seconds 1; Remove-Item -LiteralPath '$scriptPath' -Force"
) | Out-Null
