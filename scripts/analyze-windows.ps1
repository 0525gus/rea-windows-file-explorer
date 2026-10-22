[CmdletBinding()]
param([string]$BuildDirectory = '', [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $project 'build-windows' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $project ('build-windows\analysis-' + (Get-Date -Format 'yyyyMMddTHHmmssfff')) }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Analysis output must be a new directory.' }
$tidy = 'C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Tools\Llvm\x64\bin\clang-tidy.exe'
$python = 'C:\Python314\python.exe'
$runner = Join-Path $PSScriptRoot 'run-build-stage.py'
$database = Join-Path $BuildDirectory 'compile_commands.json'
foreach ($required in @($tidy, $python, $runner, $database)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing analysis input: $required" }
}
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$arguments = @('--checks=-*,clang-analyzer-*', '--quiet', '-p', $BuildDirectory)
$includeRoots = @(
    'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\include',
    'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt',
    'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared',
    'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um',
    'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\winrt'
)
foreach ($includeRoot in $includeRoots) {
    if (-not (Test-Path -LiteralPath $includeRoot)) { throw "Missing compiler include root: $includeRoot" }
    $arguments += '--extra-arg=/imsvc' + $includeRoot
}
$records = @()
$sourceRoot = ($project.Replace('\', '/') + '/src/')
$units = @(Get-Content -LiteralPath $database -Raw | ConvertFrom-Json | Where-Object { $_.file.StartsWith($sourceRoot) })
foreach ($unit in $units) {
    $relative = [IO.Path]::GetRelativePath($project, $unit.file).Replace('\', '/')
    $prefix = Join-Path $OutputDirectory ($relative.Replace('/', '_').Replace('.', '_'))
    & $python $runner --cwd $project --log-prefix $prefix --timeout 60 -- $tidy $unit.file @arguments
    $records += [ordered]@{source=$relative;source_sha256=(Get-FileHash -LiteralPath $unit.file).Hash.ToLowerInvariant();runner_exit=$LASTEXITCODE;metrics=($prefix+'.metrics.json');stdout=($prefix+'.stdout.log');stderr=($prefix+'.stderr.log')}
}
$report = [ordered]@{created_at=(Get-Date).ToUniversalTime().ToString('o');tool=$tidy;checks='clang-analyzer-*';scope='Current Windows production translation units from compile database; no POSIX or macOS compilation';units=$records}
[IO.File]::WriteAllText((Join-Path $OutputDirectory 'report.json'), ($report | ConvertTo-Json -Depth 8))
Write-Output $OutputDirectory
if (@($records | Where-Object { $_.runner_exit -ne 0 }).Count) { throw 'One or more analysis units failed; retained reports must be inspected.' }
