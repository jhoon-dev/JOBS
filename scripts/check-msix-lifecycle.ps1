param(
    [Parameter(Mandatory=$true)][string]$InitialPackage,
    [Parameter(Mandatory=$true)][string]$UpgradePackage,
    [string]$OutputDirectory = 'build/msix-acceptance'
)
$ErrorActionPreference = 'Stop'
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Unsigned executable MSIX validation requires an elevated Windows 11/Server 2025 session. No policy or certificate changes are made.'
}
if (-not (Get-Command Add-AppxPackage).Parameters.ContainsKey('AllowUnsigned')) { throw 'AllowUnsigned is unavailable on this OS' }
$expectedName = 'ToolMoa.JOBS.MsixDevelopment'
$oid = 'OID.2.25.311729368913984317654407730594956997722=1'
$initial = Get-Content -Raw ([IO.Path]::ChangeExtension($InitialPackage, '.json')) | ConvertFrom-Json
$upgrade = Get-Content -Raw ([IO.Path]::ChangeExtension($UpgradePackage, '.json')) | ConvertFrom-Json
foreach ($entry in @(@($InitialPackage, $initial), @($UpgradePackage, $upgrade))) {
    $path, $info = $entry
    if ($info.mode -ne 'development' -or -not $info.test_probe_included -or $info.identity.IdentityName -ne $expectedName -or
        -not $info.identity.Publisher.EndsWith($oid) -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower() -ne $info.sha256) {
        throw 'Only hash-verified owned development probe packages may be installed by this test'
    }
}
if ($initial.source_revision -ne $upgrade.source_revision -or [version]$upgrade.version -le [version]$initial.version) { throw 'Upgrade source or version mismatch' }
if (Get-AppxPackage -Name $expectedName) { throw 'Development package already installed; refusing to replace existing data' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Use a fresh acceptance output directory' }
New-Item -ItemType Directory -Path $output | Out-Null
$nonce = [Guid]::NewGuid().ToString('N')
$testRoot = Join-Path $output "msix-smoke-$nonce"
New-Item -ItemType Directory -Path $testRoot | Out-Null
@{ nonce=$nonce } | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $testRoot 'test-owner.json')
$report = [ordered]@{ status='running'; source_revision=$initial.source_revision; os=[Environment]::OSVersion.VersionString; initial_sha256=$initial.sha256; upgrade_sha256=$upgrade.sha256; checks=@(); store_submission=$false; wack='not run' }
$legacyRoot = Join-Path $env:LOCALAPPDATA 'ToolMoa/JOBS'
$legacyExisted = Test-Path -LiteralPath $legacyRoot
New-Item -ItemType Directory -Force -Path $legacyRoot | Out-Null
$sentinel = Join-Path $legacyRoot "msix-smoke-$nonce.txt"
Set-Content -LiteralPath $sentinel -Value $nonce -Encoding ASCII
$sentinelHash = (Get-FileHash -LiteralPath $sentinel).Hash
$script:ownedProcesses = @()
$script:installedFamily = $null
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
[ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IJobsActivation {
    [PreserveSig] int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string id, [MarshalAs(UnmanagedType.LPWStr)] string args, uint options, out uint pid);
    [PreserveSig] int ActivateForFile(IntPtr id, IntPtr items, IntPtr verb, out uint pid);
    [PreserveSig] int ActivateForProtocol(IntPtr id, IntPtr items, out uint pid);
}
public static class JobsActivation {
    public static uint Run(string id, string args) {
        var manager = (IJobsActivation)Activator.CreateInstance(Type.GetTypeFromCLSID(new Guid("45ba127d-10a8-46ea-8ab7-56ea9078943c")));
        uint pid;
        Marshal.ThrowExceptionForHR(manager.ActivateApplication(id, args, 0, out pid));
        return pid;
    }
}
'@
function Invoke-Probe($package, $phase) {
    $args = '--root "' + $testRoot + '" --phase ' + $phase + ' --nonce ' + $nonce
    $probeId = [JobsActivation]::Run(($package.PackageFamilyName + '!PackageProbe'), $args)
    $script:ownedProcesses += $probeId
    $resultPath = Join-Path $testRoot "probe-$phase.json"
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while (-not (Test-Path -LiteralPath $resultPath)) {
        if ($timer.Elapsed.TotalSeconds -gt 100) { throw "Packaged $phase probe timed out (PID $probeId)" }
        Start-Sleep -Milliseconds 200
    }
    $result = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
    if ($result.status -ne 'success' -or -not $result.packaged -or $result.nonce -ne $nonce -or
        $result.package_family -ne $package.PackageFamilyName -or $result.source_revision -ne $initial.source_revision) {
        throw "Packaged $phase probe failed: $($result | ConvertTo-Json -Compress)"
    }
    $process = Get-Process -Id $probeId -ErrorAction SilentlyContinue
    if ($process -and -not $process.WaitForExit(15000)) { throw 'Probe did not exit after publishing result' }
    return $result
}
try {
    Add-AppxPackage -Path ([IO.Path]::GetFullPath($InitialPackage)) -AllowUnsigned
    $package = Get-AppxPackage -Name $expectedName
    if (-not $package -or [version]$package.Version -ne [version]$initial.version) { throw 'Initial package not registered' }
    $script:installedFamily = $package.PackageFamilyName
    $first = Invoke-Probe $package 'initial'
    $report.checks += 'installed and activated under actual package identity; create/save/open/calculate/undo/workspace/checkpoint'
    $privateRoot = Join-Path $env:LOCALAPPDATA ('Packages/' + $package.PackageFamilyName + '/LocalState')
    if (-not ([IO.Path]::GetFullPath($first.settings_file)).StartsWith([IO.Path]::GetFullPath($privateRoot), [StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $first.settings_file) -or -not (Test-Path -LiteralPath $first.recovery_directory)) { throw 'Private package state missing' }
    $external = @{}
    foreach ($name in @('Project.jobs','Team.jobs','Projects.jobsworkspace')) { $external[$name] = (Get-FileHash -LiteralPath (Join-Path $testRoot $name)).Hash }
    Add-AppxPackage -Path ([IO.Path]::GetFullPath($UpgradePackage)) -AllowUnsigned
    $package = Get-AppxPackage -Name $expectedName
    if ([version]$package.Version -ne [version]$upgrade.version -or $package.PackageFamilyName -ne $script:installedFamily) { throw 'Upgrade identity changed' }
    $second = Invoke-Probe $package 'verify'
    $report.checks += 'version upgrade preserved settings; checkpoint recovered; external project/workspace reopened and calculated'
    Remove-AppxPackage -Package $package.PackageFullName
    $timer = [Diagnostics.Stopwatch]::StartNew()
    while ((Test-Path -LiteralPath $privateRoot) -and $timer.Elapsed.TotalSeconds -lt 30) { Start-Sleep -Milliseconds 200 }
    if ((Get-AppxPackage -Name $expectedName) -or (Test-Path -LiteralPath $first.settings_file) -or (Test-Path -LiteralPath $first.recovery_directory)) { throw 'Uninstall retained application settings/recovery' }
    foreach ($name in $external.Keys) { if ((Get-FileHash -LiteralPath (Join-Path $testRoot $name)).Hash -ne $external[$name]) { throw "External document changed: $name" } }
    if ((Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) { throw 'Standalone edition state changed' }
    $report.checks += 'uninstall removed package settings/recovery; outside documents and standalone state preserved'
    $report.status = 'success'
    $report.package_family = $script:installedFamily
    $report.initial = $first
    $report.upgrade = $second
} catch {
    $report.status = 'failed'
    $report.error = $_.Exception.Message
    throw
} finally {
    foreach ($processId in $script:ownedProcesses) {
        $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
        if ($process -and $process.Path -and $process.Path -like '*\ToolMoa.JOBS.MsixDevelopment_*\jobs_msix_probe.exe') { Stop-Process -Id $processId -Force }
    }
    $remaining = Get-AppxPackage -Name $expectedName
    if ($remaining -and $script:installedFamily -eq $remaining.PackageFamilyName) { Remove-AppxPackage -Package $remaining.PackageFullName }
    if ((Test-Path -LiteralPath $sentinel) -and (Get-Content -Raw -LiteralPath $sentinel).Trim() -eq $nonce) { Remove-Item -LiteralPath $sentinel }
    if (-not $legacyExisted -and @(Get-ChildItem -LiteralPath $legacyRoot -Force).Count -eq 0) { Remove-Item -LiteralPath $legacyRoot }
    $report | ConvertTo-Json -Depth 10 | Set-Content -Encoding UTF8 (Join-Path $output 'lifecycle.json')
}
$report | ConvertTo-Json -Depth 10
