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
- C++ toolset v145を使用できるVisual Studio（現行プロジェクト設定）
- Desktop development with C++
- Windows SDK
- DirectX 12対応GPU

## フォルダ構成
- `application/`: この作品固有のゲームロジック
- `application/EngineApp.*`: 現行ゲームの起動、Scene管理、作品用資産の初期化
- `engine/base/`: DirectX 12初期化、Window、フレームワーク、EngineContext
- `engine/render/`: Renderer、RenderGraph、RenderPass、Pipeline、PostEffect、描画Resource管理
- `engine/graphics/`: Sprite、Particle、Object3D、Skybox、Cloud、Trail、Lineなどの描画機能
- `engine/math/`: Vector、Matrix、Transform、数学関数
- `engine/input/`: Keyboard、Mouse、GamePad入力
- `engine/audio/`: 音声再生、音声管理
- `engine/integration/`: Windows APIや外部機能との連携処理
- `scene/`: Scene遷移、Scene基底、各Scene実装
- `resources/`: Shader、Texture、Model、Sound、Config
- `tests/`: 既存の検証ソース（独立したTestプロジェクトは作成しない）
- `docs/`: 設計資料、分析レポート、使用ガイド
- `externals/`: DirectXTex、imgui、assimpヘッダなどの外部依存
- `msbuild/`: Engine共通設定と実行資産の配置
- `scripts/VerifyEngineBoundary.ps1`: Engineからゲームへの逆依存検査

## プロジェクトと依存

ソリューションに置くプロジェクトは次の2つだけです。

```text
Game.exe → MagEngine.lib → 外部ライブラリ
```

- **MagEngine**：エンジンの静的ライブラリ。engine/と既存ImGuiソースをビルドします。
- **Game**：現行ゲームの実行アプリ。application/、scene/、main.cppをビルドし、MagEngineを参照します。

Core・Minimal・Test・ImGui用の独立プロジェクトはありません。DirectXTexはMagEngineが参照する既存の外部依存であり、ソリューションには追加しません。外部ライブラリの原本は変更していません。

## ビルド・起動

1. MagEngine.slnをVisual Studioで開く。
2. DebugまたはRelease、x64を選択する。
3. Gameをスタートアッププロジェクトに設定してビルド・実行する。

開発者PowerShellからはプロジェクト直下で実行します。

```powershell
# 全体（Releaseでは構成名を変更）
msbuild MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64

# エンジン単独
msbuild MagEngine.vcxproj /m /p:Configuration=Debug /p:Platform=x64

# ゲームを起動
& ..\generated\outputs\Game\Debug\Game.exe

# 2プロジェクト構成と層の逆依存を検査
& .\scripts\VerifyEngineBoundary.ps1
```

## 配布と資産

- ゲームの出力先：../generated/outputs/Game/<Configuration>/。
- エンジンの出力先：../generated/outputs/MagEngine/<Configuration>/。
- Gameのビルド後に必要なDLLとresources/を実行フォルダへ配置します。
- ゲームはexe位置を資産参照ルートにするため、起動時の作業フォルダに依存しません。
- 配布にはGame.exeだけでなく、同じ出力フォルダのDLLとresources/を含めてください。
- CIはDebug／Releaseのビルドと層境界の検査を行い、ゲームの成果物を収集します。
- 以前のMinimal・Core・Test用出力は現行の起動先として使用しません。

詳細は[分離設計](docs/MagEngine/MagEngine_06_ゲーム層エンジン層分離設計.md)と[履歴](docs/MagEngine/MagEngine_04_仕様改修履歴.md)を参照してください。

## 資産・パーティクルの直接回帰検証

指定構成のMagEngineをビルドした後、次を実行します。検証用プロジェクトは追加せず、実行ファイルとfixtureはgenerated/validation/asset-safety/に生成します。

```powershell
& .\scripts\RunAssetSafetyTests.ps1 -Configuration Debug
& .\scripts\RunAssetSafetyTests.ps1 -Configuration Release
# 実XAudio2デバイスの無音再生・停止・寿命検証も行う場合
& .\scripts\RunAssetSafetyTests.ps1 -Configuration Release -Audio
```

CPU検証はWindow／GPUを初期化しません。Audio指定時も聴感やゲーム操作の確認は行いません。

ME-04〜ME-06の粒子検証も同じスクリプトから実行できます。

```powershell
& .\scripts\RunAssetSafetyTests.ps1 -Configuration Debug -Suite Particle
& .\scripts\RunAssetSafetyTests.ps1 -Configuration Release -Suite Particle -Gpu
```

Gpu指定ではWindowを作らず、実D3D12のPSO・Resource・DescriptorとMap済みデータを確認します。GPUへ描画コマンドは投入せず、ゲームの見た目・操作確認とは区別します。

## 注意
- ShaderとResourceは実行アプリのフォルダを基準に相対パス（例: `resources/shader/...`）で参照されます。
- ビルド生成物は`generated/`および各構成別出力フォルダに生成されるため、提出物には含めていません。
