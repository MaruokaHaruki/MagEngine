# MagEngine

## 概要
DirectX 12を用いた自作ゲームエンジンです。

## 主な実装
- RenderPass / RenderWorld
- RenderGraphによるResource Usage検証
- Transition Planと限定的Barrier自動発行
- PipelineRecipe + PipelineBuilder
- PostEffect Ping-Pong管理
- Sprite / Particle / Object3D / Skybox / Cloud / Trail描画

## 動作環境
- Windows 10 / 11
- Visual Studio 2022以降
- Desktop development with C++
- Windows SDK
- DirectX 12対応GPU

## フォルダ構成
- `application/`: この作品固有のゲームロジック
- `engine/base/`: DirectX 12初期化、Window、フレームワーク、EngineContext
- `engine/render/`: Renderer、RenderGraph、RenderPass、Pipeline、PostEffect、描画Resource管理
- `engine/graphics/`: Sprite、Particle、Object3D、Skybox、Cloud、Trail、Lineなどの描画機能
- `engine/math/`: Vector、Matrix、Transform、数学関数
- `engine/input/`: Keyboard、Mouse、GamePad入力
- `engine/audio/`: 音声再生、音声管理
- `engine/integration/`: Windows APIや外部機能との連携処理
- `scene/`: Scene遷移、Scene基底、各Scene実装
- `resources/`: 実行時に必要なShader、DDS Texture、Model、Config、Font
- `resources/tools/`: フォントアトラス・DDS変換用の開発ツール。実行時には不要
- `tests/`: RenderGraph / PipelineRecipeなどのCPU検証コード
- `docs/`: 設計資料、分析レポート、使用ガイド
- `externals/`: DirectXTex、imgui、assimpヘッダなどの外部依存

## ビルド手順
1. `MagEngine.sln`をVisual Studioで開く
2. Configurationを`Debug`、Platformを`x64`に設定
3. Build Solutionを実行
4. 実行プロジェクトを起動

## 注意
- ShaderとResourceは実行時に相対パス（例: `resources/shader/...`）で参照されます。
- `resources/texture/source/` と `resources/sound/` はローカル管理です。公開リポジトリで動作させるための最小構成には含めません。
- 初回ビルド時にShaderやResourceの相対パスが正しく解決されることを確認してください。
- ビルド生成物は`generated/`および各構成別出力フォルダに生成されるため、提出物には含めていません。
