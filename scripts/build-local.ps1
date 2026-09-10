param([switch]$Run, [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $repo 'build'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'Install Visual Studio Build Tools with Desktop development with C++.' }
$vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1)
if (-not $vs) { throw 'Visual C++ Build Tools not found.' }
$cmake = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ninja = Join-Path $vs 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
$pythonCommand = Get-Command python -ErrorAction SilentlyContinue
$python = if ($pythonCommand) { $pythonCommand.Source } else { Join-Path $env:USERPROFILE '.platformio/penv/Scripts/python.exe' }
if (-not (Test-Path $python)) { throw 'Python 3 is required for embedding the device interface and running tests.' }
New-Item -ItemType Directory -Force $build | Out-Null
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
$commands = @(
 '@echo off',
 ('call "{0}" >nul' -f $vcvars),
 'if errorlevel 1 exit /b 1',
 ('"{0}" -S "{1}" -B "{2}" -G Ninja "-DCMAKE_MAKE_PROGRAM={3}" "-DPython3_EXECUTABLE={4}" -DCMAKE_BUILD_TYPE=Debug -UOWLANZI_APP_VERSION' -f $cmake,$repo,$build,$ninja,$python),
 'if errorlevel 1 exit /b 1',
 ('"{0}" --build "{1}" --parallel 4' -f $cmake,$build),
 'if errorlevel 1 exit /b 1'
)
if (-not $SkipTests) {
 $ctest=Join-Path (Split-Path $cmake) 'ctest.exe'
 $commands += ('"{0}" --test-dir "{1}" --output-on-failure' -f $ctest,$build)
 $commands += 'if errorlevel 1 exit /b 1'
}
$batch=Join-Path $build 'build-local.cmd'
[IO.File]::WriteAllLines($batch,$commands,[Text.Encoding]::ASCII)
& $env:ComSpec /d /c $batch
if ($LASTEXITCODE -ne 0) { throw 'Native build or tests failed.' }
if (-not $SkipTests) {
 Push-Location $repo
 try { & node --test tests/web-tests.mjs; if ($LASTEXITCODE -ne 0) { throw 'Web tests failed.' } } finally { Pop-Location }
}
if ($Run) {
 Push-Location $repo
 try { & (Join-Path $build 'owlanzi-tc002.exe') --demo } finally { Pop-Location }
}
