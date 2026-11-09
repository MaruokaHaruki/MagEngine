# 資産／粒子の直接回帰検証。独立プロジェクトは作らず、ビルド済みEngineを使う。
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$Audio,
    [ValidateSet('Asset', 'Particle')][string]$Suite = 'Asset',
    [switch]$Gpu
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$folder = if ($Suite -eq 'Particle') { 'particle-regression' } else { 'asset-safety' }
$testName = if ($Suite -eq 'Particle') { 'ParticleRegressionTests' } else { 'AssetSafetyTests' }
if (($Audio -and $Suite -ne 'Asset') -or ($Gpu -and $Suite -ne 'Particle')) { throw 'AudioはAsset、GpuはParticleで指定してください。' }
$output = [IO.Path]::GetFullPath((Join-Path $root "../generated/validation/$folder/$Configuration"))
$library = Join-Path $root "../generated/outputs/MagEngine/$Configuration/MagEngine.lib"
if (-not (Test-Path -LiteralPath $library)) { throw '先にMagEngineを指定構成でビルドしてください。' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) { throw 'Visual C++の開発環境がありません。' }
$vcvars = Join-Path $install 'VC/Auxiliary/Build/vcvars64.bat'
New-Item -ItemType Directory -Path $output -Force | Out-Null

# Engineのinclude設定を再利用し、テスト側の独自設定とのずれを避ける。
[xml]$settings = Get-Content -LiteralPath (Join-Path $root 'msbuild/EngineSettings.props') -Raw
$includes = @($settings.Project.ItemDefinitionGroup.ClCompile.AdditionalIncludeDirectories -split ';' |
    Where-Object { $_ -and $_ -notmatch '%\(' } |
    ForEach-Object { '/I"' + $_.Replace('$(ProjectDir)', "$root/") + '"' })
$runtime = if ($Configuration -eq 'Debug') { '/MTd /D_DEBUG' } else { '/MT /DNDEBUG' }
$assimp = if ($Configuration -eq 'Debug') { 'assimp-vc143-mtd.lib' } else { 'assimp-vc143-mt.lib' }
$exe = Join-Path $output "$testName.exe"
$response = @(
    "/nologo /std:c++20 /EHsc /utf-8 $runtime",
    ($includes -join ' '),
    ('"' + (Join-Path $root "tests/$testName.cpp") + '"'),
    ('"' + $library + '"'),
    ('"' + (Join-Path $root "externals/assimp/lib/$Configuration/$assimp") + '"'),
    ('/Fo"' + (Join-Path $output "$testName.obj") + '" /Fe"' + $exe + '"'),
    'user32.lib'
)
$rsp = Join-Path $output 'compile.rsp'
[IO.File]::WriteAllLines($rsp, $response)
$batch = Join-Path $output 'compile.cmd'
[IO.File]::WriteAllLines($batch, @('@echo off', ('call "' + $vcvars + '" >nul'), 'if errorlevel 1 exit /b 1', ('cl @"' + $rsp + '"'), 'exit /b %errorlevel%'))
& $env:ComSpec /d /c $batch
if ($LASTEXITCODE -ne 0) { throw '回帰検証のコンパイルに失敗しました。' }
# Debugの静的ライブラリはCPU検証でもDXCのimportを保持するため、両Suiteへ配置する。
& {
    $sdkBin = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
    $compilerBin = Get-ChildItem -LiteralPath $sdkBin -Directory | Sort-Object Name -Descending |
        Where-Object { (Test-Path -LiteralPath (Join-Path $_.FullName 'x64/dxcompiler.dll')) -and (Test-Path -LiteralPath (Join-Path $_.FullName 'x64/dxil.dll')) } |
        Select-Object -First 1
    if (-not $compilerBin) { throw 'Windows SDKのShaderコンパイラーDLLがありません。' }
    foreach ($dll in @('dxcompiler.dll', 'dxil.dll')) {
        Copy-Item -LiteralPath (Join-Path $compilerBin.FullName "x64/$dll") -Destination (Join-Path $output $dll) -Force
    }
}
$arguments = @($root, (Join-Path $output 'fixtures'))
if ($Audio) { $arguments += '--audio' }
if ($Gpu) { $arguments += '--gpu' }
& $exe @arguments
if ($LASTEXITCODE -ne 0) { throw "回帰検証に失敗しました。終了コード: $LASTEXITCODE" }
