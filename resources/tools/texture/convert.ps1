param(
    [int]$MipLevel = 3
)

$ErrorActionPreference = 'Stop'
$toolDirectory = $PSScriptRoot
$resourceDirectory = Split-Path -Parent (Split-Path -Parent $toolDirectory)
$sourceDirectory = Join-Path $resourceDirectory 'texture/source'
$converterPath = Join-Path $toolDirectory 'TextureConverter.exe'

if (-not (Test-Path -LiteralPath $converterPath)) {
    throw "TextureConverter.exe was not found: $converterPath"
}

if (-not (Test-Path -LiteralPath $sourceDirectory)) {
    throw "Texture source directory was not found: $sourceDirectory"
}

Get-ChildItem -LiteralPath $sourceDirectory -File | Where-Object { $_.Extension -in '.jpg', '.png' } | ForEach-Object {
    # 実行時アセットはtexture直下に置くため、変換元と出力先を明示して実行場所に依存させない。
    & $converterPath $_.FullName '-ml' $MipLevel '-o' $resourceDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "Texture conversion failed: $($_.FullName)"
    }
}
