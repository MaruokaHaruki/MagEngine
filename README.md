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
- `application/`: ゲーム側の実装
- `engine/`: 描画、入力、音声、数学、基盤処理
- `scene/`: シーン管理と各シーン
- `resources/`: Shader、Texture、Model、Sound、Config
- `externals/`: DirectXTex、imgui、assimpヘッダなどの外部依存
- `tests/`: RenderGraph / PipelineRecipeなどのCPU検証コード

## ビルド手順
1. `MagEngine.sln`をVisual Studioで開く
2. Configurationを`Debug`、Platformを`x64`に設定
3. Build Solutionを実行
4. 実行プロジェクトを起動

## 注意
- ShaderとResourceは実行時に相対パス（例: `resources/shader/...`）で参照されます。
- 初回ビルド時にShaderやResourceの相対パスが正しく解決されることを確認してください。
- ビルド生成物は`generated/`および各構成別出力フォルダに生成されるため、提出物には含めていません。
