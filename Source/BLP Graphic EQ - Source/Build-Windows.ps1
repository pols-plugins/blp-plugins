param([string]$JuceSource = "")
$ErrorActionPreference = 'Stop'
$buildPath = Join-Path $PSScriptRoot 'build'
$configureArgs = @('-S', $PSScriptRoot, '-B', $buildPath, '-G', 'Visual Studio 17 2022', '-A', 'x64')
if ($JuceSource) { $configureArgs += "-DJUCE_DIR=$JuceSource" }
& cmake @configureArgs
if ($LASTEXITCODE) { throw 'CMake configuration failed' }
& cmake --build $buildPath --config Release --parallel 1
if ($LASTEXITCODE) { throw 'Build failed' }
& ctest --test-dir $buildPath -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'DSP tests failed' }
$bundle = Join-Path $buildPath 'GraphicEQ_artefacts\Release\VST3\BLP Graphic EQ.vst3'
$hostTest = Join-Path $buildPath 'HostTests_artefacts\Release\HostTests.exe'
Push-Location $buildPath
try {
    & $hostTest $bundle
    if ($LASTEXITCODE) { throw 'VST3 integration tests failed' }
} finally { Pop-Location }
Write-Host "Built and tested: $bundle"
