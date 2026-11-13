# 欠損した登録とscene削除後の検査を、生成領域の最小構成で直接確認する。
param([string]$FixtureRoot = (Join-Path $PSScriptRoot '../generated/engine-only-cleanup/boundary-fixture'))
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath($FixtureRoot)
New-Item -ItemType Directory -Force (Join-Path $root 'engine'), (Join-Path $root 'application'), (Join-Path $root 'msbuild') | Out-Null
Set-Content (Join-Path $root 'engine/Empty.h') '#pragma once'
Set-Content (Join-Path $root 'application/Empty.cpp') 'int empty;'
Set-Content (Join-Path $root 'msbuild/EngineSettings.props') '<Project />'
Set-Content (Join-Path $root 'MagEngine.vcxproj') '<Project><ItemGroup><ClCompile Include="engine\**\*.cpp" /></ItemGroup></Project>'
Copy-Item (Join-Path $PSScriptRoot '../MagEngine.sln') (Join-Path $root 'MagEngine.sln') -Force
Set-Content (Join-Path $root 'Game.vcxproj') '<Project><ItemGroup><ClCompile Include="application\Missing.cpp" /></ItemGroup></Project>'
$rejected = $false
try { & (Join-Path $PSScriptRoot '../scripts/VerifyEngineBoundary.ps1') -ProjectRoot $root } catch {
    if ($_.Exception.Message -notlike '*登録先が存在しません*') { throw }
    $rejected = $true
}
if (-not $rejected) { throw 'MissingProjectItemWasAccepted' }
Set-Content (Join-Path $root 'Game.vcxproj') '<Project><ItemGroup><ClCompile Include="application\Empty.cpp" /></ItemGroup></Project>'
& (Join-Path $PSScriptRoot '../scripts/VerifyEngineBoundary.ps1') -ProjectRoot $root
Write-Output 'EngineBoundary PASS: missing registration rejected; absent scene accepted'
