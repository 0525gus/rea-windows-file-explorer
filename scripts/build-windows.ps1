[CmdletBinding()]
param(
    [string]$SourceDirectory = (Split-Path -Parent $PSScriptRoot),
    [string]$BuildDirectory = '',
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')][string]$Configuration = 'Release',
    [string]$QtPrefix = '',
    [switch]$RunTests,
    [switch]$Fresh
)

$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $workspace 'build-windows' }
if (-not $QtPrefix) { $QtPrefix = Join-Path $workspace 'tools\qt\6.8.3\msvc2022_64' }
$sourcePath = (Resolve-Path -LiteralPath $SourceDirectory).Path
$buildPath = [IO.Path]::GetFullPath($BuildDirectory)
$qtPath = (Resolve-Path -LiteralPath $QtPrefix).Path
$visualStudio = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools'
$vcvars = Join-Path $visualStudio 'VC\Auxiliary\Build\vcvars64.bat'
$cmake = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$ninja = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$python = 'C:\Python314\python.exe'
$runner = Join-Path $PSScriptRoot 'run-build-stage.py'
foreach ($p in @($vcvars, $cmake, $ctest, $ninja, $python, $runner, (Join-Path $sourcePath 'CMakeLists.txt'))) {
    if (-not (Test-Path -LiteralPath $p -PathType Leaf)) { throw "Required build input is missing: $p" }
}

# vcvars executes only inside this child cmd. Its environment output remains in
# memory and is never printed, logged, or written to a file.
$vcCommand = 'call "' + $vcvars + '" >nul && set'
$vcInfo = [Diagnostics.ProcessStartInfo]::new()
$vcInfo.FileName = [Environment]::GetEnvironmentVariable('ComSpec')
$vcInfo.UseShellExecute = $false
$vcInfo.CreateNoWindow = $true
$vcInfo.RedirectStandardOutput = $true
$vcInfo.RedirectStandardError = $true
$vcInfo.Environment.Clear()
$systemVariables = @('SystemRoot', 'SystemDrive', 'windir', 'ComSpec', 'TEMP', 'TMP', 'ProgramData', 'ProgramFiles', 'ProgramFiles(x86)', 'ProgramW6432', 'CommonProgramFiles', 'CommonProgramFiles(x86)', 'CommonProgramW6432', 'PROCESSOR_ARCHITECTURE', 'NUMBER_OF_PROCESSORS', 'OS', 'PATHEXT', 'USERPROFILE', 'LOCALAPPDATA', 'APPDATA')
foreach ($environmentName in $systemVariables) {
    $value = [Environment]::GetEnvironmentVariable($environmentName)
    if ($null -ne $value) { $vcInfo.Environment[$environmentName] = $value }
}
$systemRoot = [Environment]::GetEnvironmentVariable('SystemRoot')
$vcInfo.Environment['PATH'] = (Join-Path $visualStudio '..\..\Installer') + ';' + (Join-Path $systemRoot 'System32') + ';' + $systemRoot + ';' + (Join-Path $systemRoot 'System32\Wbem') + ';' + (Join-Path $systemRoot 'System32\WindowsPowerShell\v1.0')
# cmd.exe parses its /c command with cmd quoting, unlike the ordinary native
# argv quoting used for every other process below. This command contains only
# the fixed, verified vcvars path and the literal environment-capture operation.
$vcInfo.Arguments = '/d /s /c "' + $vcCommand + '"'
$vcProcess = [Diagnostics.Process]::new()
$vcProcess.StartInfo = $vcInfo
try {
    [void]$vcProcess.Start()
    $vcStdout = $vcProcess.StandardOutput.ReadToEndAsync()
    $vcStderr = $vcProcess.StandardError.ReadToEndAsync()
    $vcProcess.WaitForExit()
    $vcOutput = $vcStdout.GetAwaiter().GetResult() -split '\r?\n'
    $vcError = $vcStderr.GetAwaiter().GetResult()
    if ($vcProcess.ExitCode -ne 0) { throw "vcvars64 failed: $vcError" }
    if ($vcError) { throw "vcvars64 reported an unexpected diagnostic: $vcError" }
}
finally { $vcProcess.Dispose() }
$childEnvironment = @{}
foreach ($line in $vcOutput) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) { $childEnvironment[$line.Substring(0, $separator)] = $line.Substring($separator + 1) }
}
$vcOutput = $null
$childEnvironment['PATH'] = (Join-Path $qtPath 'bin') + ';' + (Split-Path -Parent $ninja) + ';' + (Split-Path -Parent $cmake) + ';' + $childEnvironment['PATH']
$childEnvironment['QT_QPA_PLATFORM'] = 'offscreen'
$childEnvironment['QT_PLUGIN_PATH'] = Join-Path $qtPath 'plugins'
$logDirectory = Join-Path $buildPath ('logs\' + (Get-Date -Format 'yyyyMMddTHHmmssfff'))
[IO.Directory]::CreateDirectory($logDirectory) | Out-Null

function Invoke-BuildStage([string]$StageName, [string[]]$Command) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $python
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.WorkingDirectory = $workspace
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.Environment.Clear()
    foreach ($environmentName in $childEnvironment.Keys) { $info.Environment[$environmentName] = $childEnvironment[$environmentName] }
    foreach ($argument in @($runner, '--cwd', $workspace, '--log-prefix', (Join-Path $logDirectory $StageName), '--timeout', '180', '--') + $Command) {
        $info.ArgumentList.Add($argument)
    }
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.WaitForExit()
        $outText = $stdout.GetAwaiter().GetResult()
        $errText = $stderr.GetAwaiter().GetResult()
        if ($outText) { Write-Output $outText.TrimEnd() }
        if ($errText) { Write-Output $errText.TrimEnd() }
        if ($process.ExitCode -ne 0) { throw "$StageName failed with exit code $($process.ExitCode); retained logs: $logDirectory" }
    }
    finally { $process.Dispose() }
}

$configureCommand = @($cmake, '-S', $sourcePath, '-B', $buildPath, '-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Configuration", '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', "-DCMAKE_PREFIX_PATH=$qtPath", "-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_INSTALL_PREFIX=$(Join-Path $buildPath 'install')")
if ($Fresh) { $configureCommand += '--fresh' }
Invoke-BuildStage 'configure' $configureCommand
Invoke-BuildStage 'build' @($cmake, '--build', $buildPath, '--config', $Configuration, '--parallel', '2')
if ($RunTests) { Invoke-BuildStage 'test' @($ctest, '--test-dir', $buildPath, '--output-on-failure', '--timeout', '30', '-C', $Configuration) }
Write-Output "Build logs: $logDirectory"
