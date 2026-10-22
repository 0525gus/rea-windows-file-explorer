[CmdletBinding()]
param([ValidateSet('render', 'benchmark', 'package', 'smoke')][string]$Stage)
$ErrorActionPreference = 'Stop'
$armRoot = Split-Path -Parent $PSScriptRoot
$projectRoot = [IO.Path]::GetFullPath((Join-Path $armRoot '..\..'))
$qtRoot = Join-Path $projectRoot 'tools\qt\6.8.3\msvc2022_64'
$artifactRoot = Join-Path $armRoot 'artifacts'
$packageRoot = Join-Path $artifactRoot 'package'
$executable = Join-Path $armRoot 'build-windows\explorer_a.exe'
$stageArgs = @()
$qpa = 'windows'
switch ($Stage) {
    'render' { $stageArgs = @((Join-Path $projectRoot 'test-data\fixture-v1\read-only\unicode'), '--render', (Join-Path $artifactRoot 'p1-window.png')) }
    'benchmark' {
        $executable = Join-Path $armRoot 'build-windows\benchmark_a.exe'
        $stageArgs = @((Join-Path $projectRoot 'test-data\fixture-v1\read-only\bulk'), (Join-Path $artifactRoot 'benchmark-p1-windows.json'))
    }
    'package' {
        [IO.Directory]::CreateDirectory($packageRoot) | Out-Null
        Copy-Item -LiteralPath $executable -Destination (Join-Path $packageRoot 'explorer_a.exe') -Force
        $executable = Join-Path $qtRoot 'bin\windeployqt.exe'
        $stageArgs = @('--release', '--no-translations', '--no-compiler-runtime', '--no-opengl-sw', (Join-Path $packageRoot 'explorer_a.exe'))
    }
    'smoke' {
        $executable = Join-Path $packageRoot 'explorer_a.exe'
        $stageArgs = @((Join-Path $projectRoot 'test-data\fixture-v1\read-only\empty'), '--smoke')
    }
}
$info = [Diagnostics.ProcessStartInfo]::new()
$info.FileName = 'C:\Python314\python.exe'
$info.UseShellExecute = $false
$info.CreateNoWindow = $true
$info.RedirectStandardOutput = $true
$info.RedirectStandardError = $true
$info.Environment['QT_QPA_PLATFORM'] = $qpa
if ($Stage -eq 'smoke') {
    $info.Environment['PATH'] = $packageRoot + ';' + (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
} else {
    $info.Environment['PATH'] = (Join-Path $qtRoot 'bin') + ';' + $info.Environment['PATH']
}
$prefix = Join-Path $artifactRoot ('metrics\' + $Stage + '-' + (Get-Date -Format 'yyyyMMddTHHmmssfff'))
foreach ($argument in @((Join-Path $projectRoot 'scripts\run-build-stage.py'), '--cwd', $armRoot, '--log-prefix', $prefix, '--timeout', '60', '--', $executable) + $stageArgs) {
    $info.ArgumentList.Add($argument)
}
$process = [Diagnostics.Process]::Start($info)
$stdout = $process.StandardOutput.ReadToEndAsync()
$stderr = $process.StandardError.ReadToEndAsync()
$process.WaitForExit()
$stdout.GetAwaiter().GetResult()
$stderr.GetAwaiter().GetResult()
$stageExitCode = $process.ExitCode
$process.Dispose()
if ($stageExitCode -ne 0) { throw "$Stage failed: $stageExitCode; logs $prefix" }
