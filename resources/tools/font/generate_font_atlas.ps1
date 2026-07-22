# 実行時依存ではないため、resources/tools配下でアセット生成手順を管理する。
param(
	[Parameter(Mandatory = $true)][string]$FontPath,
	[Parameter(Mandatory = $true)][string]$CharsetPath,
	[Parameter(Mandatory = $true)][string]$OutputPng,
	[Parameter(Mandatory = $true)][string]$OutputJson,
	[int]$FontSize = 40,
	[int]$AtlasSize = 1024,
	[int]$Padding = 4
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

if (-not (Test-Path -LiteralPath $FontPath)) { throw "Font file was not found: $FontPath" }
if (-not (Test-Path -LiteralPath $CharsetPath)) { throw "Charset file was not found: $CharsetPath" }
if ($FontSize -le 0 -or $AtlasSize -le 0 -or $Padding -lt 0) { throw 'FontSize, AtlasSize and Padding are invalid.' }

$charset = [System.IO.File]::ReadAllText((Resolve-Path -LiteralPath $CharsetPath), [System.Text.Encoding]::UTF8)
$characters = [System.Collections.Generic.List[char]]::new()
$seen = [System.Collections.Generic.HashSet[int]]::new()
foreach ($character in $charset.ToCharArray()) {
	if ($character -eq "`r" -or $character -eq "`n") { continue }
	if ($seen.Add([int][char]$character)) { $characters.Add($character) }
}
if (-not $seen.Contains([int][char]'?')) { $characters.Add('?'); [void]$seen.Add([int][char]'?') }
if ($characters.Count -eq 0) { throw 'Charset has no drawable characters.' }

$privateFonts = [System.Drawing.Text.PrivateFontCollection]::new()
$privateFonts.AddFontFile((Resolve-Path -LiteralPath $FontPath))
if ($privateFonts.Families.Count -ne 1) { throw "Unable to load a single font family: $FontPath" }
$font = [System.Drawing.Font]::new($privateFonts.Families[0], [single]$FontSize, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$family = $privateFonts.Families[0]
$style = [System.Drawing.FontStyle]::Regular
$ascender = [single]($family.GetCellAscent($style) * $FontSize / $family.GetEmHeight($style))
$descender = [single](-$family.GetCellDescent($style) * $FontSize / $family.GetEmHeight($style))
$lineHeight = [single]($family.GetLineSpacing($style) * $FontSize / $family.GetEmHeight($style))

$bitmap = [System.Drawing.Bitmap]::new($AtlasSize, $AtlasSize, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.Clear([System.Drawing.Color]::Transparent)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::White)
$format = [System.Drawing.StringFormat]::GenericTypographic
$format.FormatFlags = $format.FormatFlags -bor [System.Drawing.StringFormatFlags]::MeasureTrailingSpaces

$cellSize = $FontSize + $Padding * 2 + 8
$columns = [Math]::Floor($AtlasSize / $cellSize)
if ($columns -le 0) { throw 'Atlas is too small for selected font size and padding.' }
$glyphs = [System.Collections.Generic.List[object]]::new()
for ($index = 0; $index -lt $characters.Count; ++$index) {
	$cellX = ($index % $columns) * $cellSize
	$cellY = [Math]::Floor($index / $columns) * $cellSize
	if ($cellY + $cellSize -gt $AtlasSize) { throw "Atlas capacity exceeded at code point $([int][char]$characters[$index])." }
	$text = [string]$characters[$index]
	$path = [System.Drawing.Drawing2D.GraphicsPath]::new()
	$path.AddString($text, $family, [int]$style, [single]$FontSize, [System.Drawing.PointF]::new(0.0, 0.0), $format)
	$bounds = $path.GetBounds()
	$advance = [single]$graphics.MeasureString($text, $font, [System.Drawing.PointF]::new(0.0, 0.0), $format).Width
	$width = [Math]::Ceiling($bounds.Width)
	$height = [Math]::Ceiling($bounds.Height)
	$x = $cellX + $Padding
	$y = $cellY + $Padding
	if ($width -gt 0 -and $height -gt 0) { $graphics.DrawString($text, $font, $brush, [single]($x - $bounds.X), [single]($y - $bounds.Y), $format) }
	$glyphs.Add([ordered]@{ codePoint = [int][char]$characters[$index]; x = [int]$x; y = [int]$y; width = [int]$width; height = [int]$height; bearingX = [single]$bounds.X; bearingY = [single]($ascender - $bounds.Y); advance = $advance })
	$path.Dispose()
}

[System.IO.Directory]::CreateDirectory((Split-Path -Parent $OutputPng)) | Out-Null
[System.IO.Directory]::CreateDirectory((Split-Path -Parent $OutputJson)) | Out-Null
$bitmap.Save($OutputPng, [System.Drawing.Imaging.ImageFormat]::Png)
$json = [ordered]@{ texture = $OutputPng.Replace('\\', '/'); textureWidth = $AtlasSize; textureHeight = $AtlasSize; fontSize = $FontSize; ascender = $ascender; descender = $descender; lineHeight = $lineHeight; replacementCodePoint = [int][char]'?'; glyphs = $glyphs }
[System.IO.File]::WriteAllText($OutputJson, ($json | ConvertTo-Json -Depth 4), [System.Text.UTF8Encoding]::new($false))

$brush.Dispose(); $graphics.Dispose(); $bitmap.Dispose(); $font.Dispose(); $privateFonts.Dispose()
Write-Host "Generated $($glyphs.Count) glyphs: $OutputPng / $OutputJson"
