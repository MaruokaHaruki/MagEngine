# 編集データの直接CPU検証。独立したプロジェクトやGPU初期化は不要。
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [ValidateSet('Document', 'History', 'Persistence', 'Viewport', 'Session', 'Input', 'All')][string]$Suite = 'All'
)
$ErrorActionPreference = 'Stop'
$editorRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$editorOutput = [IO.Path]::GetFullPath((Join-Path $editorRoot "../generated/validation/editor/$Configuration"))
New-Item -ItemType Directory -Force $editorOutput | Out-Null
$vsFinder = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsInstall = & $vsFinder -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) { throw 'Visual C++の開発環境がありません。' }
[xml]$editorSettings = Get-Content -LiteralPath (Join-Path $editorRoot 'msbuild/EngineSettings.props') -Raw
$editorIncludes = @($editorSettings.Project.ItemDefinitionGroup.ClCompile.AdditionalIncludeDirectories -split ';' |
    Where-Object { $_ -and $_ -notmatch '%\(' } |
    ForEach-Object { '/I"' + $_.Replace('$(ProjectDir)', "$editorRoot/") + '"' })
$editorSuites = if ($Suite -eq 'All') { @('Document', 'History', 'Persistence', 'Viewport', 'Session', 'Input') } else { @($Suite) }
foreach ($editorSuite in $editorSuites) {
    $editorTest = Join-Path $editorRoot "tests/Editor${editorSuite}Tests.cpp"
    if (-not (Test-Path -LiteralPath $editorTest)) { throw "未実装のSuite: $editorSuite" }
    $editorSources = @(Join-Path $editorRoot 'engine/editor/EditorDocument.cpp')
    if ($editorSuite -eq 'Document') { $editorSources += Join-Path $editorRoot 'application/editor/EditorLaunchOptions.cpp' }
    if ($editorSuite -eq 'History') { $editorSources += Join-Path $editorRoot 'engine/editor/EditorHistory.cpp' }
    if ($editorSuite -eq 'Persistence') { $editorSources += Join-Path $editorRoot 'engine/editor/EditorDocumentStore.cpp' }
    if ($editorSuite -eq 'Session') {
        $editorSources += Join-Path $editorRoot 'engine/editor/EditorHistory.cpp'
        $editorSources += Join-Path $editorRoot 'engine/editor/EditorPlaySession.cpp'
    }
    if ($editorSuite -eq 'Viewport') { $editorSources += Join-Path $editorRoot 'engine/editor/EditorViewportMath.cpp' }
    if ($editorSuite -eq 'Input') { $editorSources = @(Join-Path $editorRoot 'engine/input/Input.cpp') }
    $editorRuntime = if ($Configuration -eq 'Debug') { '/MTd /D_DEBUG' } else { '/MT /DNDEBUG' }
    $editorExe = Join-Path $editorOutput "Editor${editorSuite}Tests.exe"
    $editorRsp = Join-Path $editorOutput "${editorSuite}.rsp"
    $editorArgs = @('/nologo /std:c++20 /EHsc /utf-8 /W3 /WX /Gy', $editorRuntime, ($editorIncludes -join ' '), ('"' + $editorTest + '"'))
    $editorArgs += $editorSources | ForEach-Object { '"' + $_ + '"' }
    if ($editorSuite -eq 'Input') { $editorArgs += '"' + (Join-Path $editorRoot "../generated/outputs/MagEngine/$Configuration/MagEngine.lib") + '"' }
    $editorArgs += '/Fo"' + $editorOutput + '/" /Fe"' + $editorExe + '"'
    $editorArgs += '/link /OPT:REF user32.lib'
    [IO.File]::WriteAllLines($editorRsp, $editorArgs)
    $editorBatch = Join-Path $editorOutput "${editorSuite}.cmd"
    [IO.File]::WriteAllLines($editorBatch, @('@echo off', ('call "' + (Join-Path $vsInstall 'VC/Auxiliary/Build/vcvars64.bat') + '" >nul'), 'if errorlevel 1 exit /b 1', ('cl @"' + $editorRsp + '"'), 'exit /b %errorlevel%'))
    & $env:ComSpec /d /c $editorBatch
    if ($LASTEXITCODE -ne 0) { throw "コンパイル失敗: $editorSuite" }
    & $editorExe (Join-Path $editorOutput 'fixtures')
    if ($LASTEXITCODE -ne 0) { throw "検証失敗: $editorSuite" }
}
