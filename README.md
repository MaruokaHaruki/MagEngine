# MagEngine

DirectX 12 と C++20 のゲーム制作環境です。

Visual Studio で `MagEngine.sln` を開き、`Debug | x64` を選んで F5 を押すと空の制作画面が開きます。Release は制作UIなしで起動します。

- `engine/` はエンジン本体
- `application/` は起動処理とエディターの接続
- `resources/` は実行用資産と再利用できる素材
- `externals/` は ImGui、DirectXTex、Assimp などの依存ライブラリ

ビルドすると `bin/<構成>/MagEngine.exe` と実行用資産が作られます。中間ファイルは `obj/` に出力されます。Visual Studio と VS Code の Debug 起動では、ソース側の制作ルートを使い、シーンを `resources/config/scenes/` に保存します。実行ファイルを直接起動して制作元を使う場合は `--editor --project <制作ルートの絶対パス>` を指定します。
