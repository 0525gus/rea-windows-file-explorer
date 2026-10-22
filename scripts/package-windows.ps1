[CmdletBinding()]
param([string]$BuildDirectory='', [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$project=Split-Path -Parent $PSScriptRoot
if(-not $BuildDirectory){$BuildDirectory=Join-Path $project 'build-windows'}
if(-not $OutputDirectory){$OutputDirectory=Join-Path $project ('dist\windows-'+(Get-Date -Format 'yyyyMMddTHHmmssfff'))}
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Package output must be a new directory.'}
$binary=Join-Path $build 'rea_explorer.exe'
$qt=Join-Path $project 'tools\qt\6.8.3\msvc2022_64'
$runtime='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Redist\MSVC\14.51.36231\x64\Microsoft.VC145.CRT'
foreach($required in @($binary,(Join-Path $qt 'bin\windeployqt.exe'),$runtime,(Join-Path $project 'docs\evaluation\qt-licenses\manifest.json'))){if(-not(Test-Path -LiteralPath $required)){throw "Missing package input: $required"}}
[IO.Directory]::CreateDirectory($output)|Out-Null
Copy-Item -LiteralPath $binary -Destination $output
$info=[Diagnostics.ProcessStartInfo]::new()
$info.FileName=Join-Path $qt 'bin\windeployqt.exe';$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
foreach($arg in @('--release','--no-translations','--no-plugins','--no-compiler-runtime','--dir',$output,(Join-Path $output 'rea_explorer.exe'))){$info.ArgumentList.Add($arg)}
$process=[Diagnostics.Process]::new();$process.StartInfo=$info
try{[void]$process.Start();$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync();$process.WaitForExit();$outText=$stdout.GetAwaiter().GetResult();$errText=$stderr.GetAwaiter().GetResult();[IO.File]::WriteAllText((Join-Path $output 'deployment.log'),$outText+$errText);if($process.ExitCode-ne 0){throw "windeployqt failed: $($process.ExitCode)"}}finally{$process.Dispose()}
foreach($plugin in @('platforms\qwindows.dll','styles\qmodernwindowsstyle.dll')){$target=Join-Path $output $plugin;[IO.Directory]::CreateDirectory((Split-Path -Parent $target))|Out-Null;Copy-Item -LiteralPath (Join-Path $qt ('plugins\'+$plugin)) -Destination $target}
Get-ChildItem -LiteralPath $runtime -Filter '*.dll' -File|ForEach-Object{Copy-Item -LiteralPath $_.FullName -Destination $output}
Copy-Item -LiteralPath (Join-Path $project 'docs\evaluation\qt-licenses') -Destination (Join-Path $output 'licenses') -Recurse
Copy-Item -LiteralPath (Join-Path $project 'docs\evaluation\qt-attributions') -Destination (Join-Path $output 'qt-attributions') -Recurse
Copy-Item -LiteralPath (Join-Path $project 'docs\evaluation\dependencies.md') -Destination (Join-Path $output 'DEPENDENCIES.md')
Copy-Item -LiteralPath (Join-Path $project 'README.md') -Destination $output
[IO.File]::WriteAllText((Join-Path $output 'qt.conf'),"[Paths]`nPrefix=.`nPlugins=.`n")
$source=Join-Path $output 'source';[IO.Directory]::CreateDirectory($source)|Out-Null
foreach($directory in @('src','include','tests','scripts')){Copy-Item -LiteralPath (Join-Path $project $directory) -Destination $source -Recurse}
foreach($file in @('CMakeLists.txt','README.md','.clang-format','.gitattributes')){Copy-Item -LiteralPath (Join-Path $project $file) -Destination $source}
$files=@(Get-ChildItem -LiteralPath $output -File -Recurse|Where-Object{$_.Name-ne 'package-manifest.json'}|ForEach-Object{[ordered]@{path=[IO.Path]::GetRelativePath($output,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}})
$manifest=[ordered]@{created_at=(Get-Date).ToUniversalTime().ToString('o');build_binary_sha256=(Get-FileHash -LiteralPath $binary).Hash.ToLowerInvariant();qt_version='6.8.3';compiler_runtime='MSVC 14.51.36231 x64';scope='Project app, selected Qt runtime plugins, CRT, license texts, corresponding application sources; no installers executed';files=$files}
[IO.File]::WriteAllText((Join-Path $output 'package-manifest.json'),($manifest|ConvertTo-Json -Depth 8))
Write-Output $output
