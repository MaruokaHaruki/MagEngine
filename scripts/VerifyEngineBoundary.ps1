# エンジンから作品への逆依存を、名前だけのincludeも含めて検出する。
param([string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path
$engineFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'engine') -Recurse -File |
    Where-Object { $_.Extension -in '.cpp', '.h' })
if ($engineFiles.Count -eq 0) { throw '検証対象のエンジンソースがありません。' }
$engineHeaders = @{}
foreach ($file in $engineFiles) { $engineHeaders[$file.Name] = $true }
$gameHeaders = @{}
foreach ($folder in @('application', 'scene')) {
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $root $folder) -Recurse -Filter '*.h') {
        $gameHeaders[$file.Name] = $true
    }
}
foreach ($file in $engineFiles) {
    $body = [IO.File]::ReadAllText($file.FullName)
    foreach ($match in [regex]::Matches($body, '(?m)^\s*#\s*include\s*"([^"]+)"')) {
        $include = $match.Groups[1].Value.Replace('\', '/')
        $name = [IO.Path]::GetFileName($include)
        if ($include -match '(^|/)(application|scene)/' -or
            ($gameHeaders.ContainsKey($name) -and !$engineHeaders.ContainsKey($name))) {
            throw "作品への逆依存: $($file.FullName) -> $include"
        }
    }
}

# プロジェクト側のincludeやコンパイル対象で、分離を迂回していないことも確認する。
[xml]$core = Get-Content -LiteralPath (Join-Path $root 'MagEngine.vcxproj') -Raw
$compileItems = @($core.Project.ItemGroup.ClCompile | Where-Object { $_.Include })
if ($compileItems.Count -eq 0) { throw 'Engineプロジェクトのコンパイル対象がありません。' }
foreach ($item in $compileItems) {
    if ($item.Include -notmatch '^(engine[\\/]|externals[\\/]imgui[\\/])') { throw "Engine外のコンパイル対象: $($item.Include)" }
}
$settings = [IO.File]::ReadAllText((Join-Path $root 'msbuild\EngineSettings.props'))
if ($settings -match '\$\(ProjectDir\)(application|scene)[\\/]?') {
    throw 'Engine共通設定にゲーム用includeパスが含まれています。'
}
[xml]$game = Get-Content -LiteralPath (Join-Path $root 'Game.vcxproj') -Raw
foreach ($item in $game.Project.ItemGroup.ClCompile) {
    if ($item.Include -match '^(engine|externals)[\\/]') { throw 'ゲームがEngineまたは外部ソースを重複コンパイルしています。' }
}
$solution = [IO.File]::ReadAllText((Join-Path $root 'MagEngine.sln'))
$projects = [regex]::Matches($solution, '(?m)^Project\("\{8BC9CEB8[^\n]+')
if ($projects.Count -ne 2) { throw 'ソリューションのC++プロジェクトはMagEngineとGameの2つに限定します。' }
Write-Output "Engine boundary passed: $($engineFiles.Count) source/header files; 2 projects"
