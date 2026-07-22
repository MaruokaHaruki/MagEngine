# Resources

実行時に参照するデータと、データ作成用ツールを分けて管理します。

| フォルダ | 用途 | GitHub公開 |
| --- | --- | --- |
| `config/`, `fonts/`, `levels/`, `model/`, `shader/`, `texture/`, `trail/` | 実行時に必要なデータ | 対象 |
| `texture/source/` | DDSの変換元PNG | 対象外（ローカル管理） |
| `sound/` | 現在はコードから再生されていないWAV | 対象外（ローカル管理） |
| `tools/` | フォントアトラス・DDS変換用ツール | スクリプトのみ対象 |

## DDS変換

`tools/texture/convert.ps1` は `texture/source/` のPNG・JPGを、実行時に読む `texture/` 直下へDDSとして出力します。
`TextureConverter.exe` は各開発環境で `tools/texture/` に配置してください。バイナリはGit管理しません。

## フォントアトラス生成

`tools/font/generate_font_atlas.ps1` は、フォントと文字セットから `texture/font_atlas.png` と `config/ui/font_atlas.json` を生成します。
