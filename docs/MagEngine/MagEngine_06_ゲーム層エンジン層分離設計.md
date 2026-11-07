---
tags: [MagEngine, 設計, アーキテクチャ]
created: 2026-11-05
updated: 2026-11-06
status: 2プロジェクト構成へ整理
---
# MagEngine ゲーム層・エンジン層の分離設計

## 決定

ソリューションは **MagEngineとGameの2プロジェクト** に限定する。現行ゲームはゲーム層として残し、依存方向はGame → MagEngine → 外部ライブラリとする。

Core、Minimal、Test、ImGuiの独立プロジェクトは作らない。以前の検証記録は[履歴](MagEngine_04_仕様改修履歴.md)に残すが、削除済み構成を現在の実行経路として扱わない。

## ビルド構成

| プロジェクト | 種類 | 対象 |
| --- | --- | --- |
| MagEngine | 静的ライブラリ | engine以下、外部ImGuiソース |
| Game | 実行アプリ | main.cpp、application、scene |

Gameだけをスタートアップ対象にする。GameはMagEngineへのProjectReferenceを持ち、Engineのソースを直接コンパイルしない。エンジンのinclude設定にapplication／sceneを追加しない。

- 共通設定：msbuild/EngineSettings.props。
- ImGui：既存外部ソースをMagEngine内でコンパイルする。外部ソースは変更しない。
- DirectXTex：既存の外部プロジェクトをx64指定で内部ビルドし、そのライブラリをMagEngine.libにまとめる。ソリューションには表示しない。
- 出力：../generated/outputs/MagEngine/{Debug,Release}/MagEngine.lib、../generated/outputs/Game/{Debug,Release}/Game.exe。
- Gameに必要なDLLとresourcesはmsbuild/DeployRuntime.targetsで配置する。実行時の資産参照ルートはexeの配置場所。
- 旧Core／Minimal／Testsの生成物は現行の起動・配布対象ではない。

## 責務と接続

| エンジン層 | ゲーム層 |
| --- | --- |
| Window、DirectX、入力取得、時間、描画フェーズ、音声再生、資源管理 | 操作の意味、ゲーム進行、使用する資産と設定 |
| MagFrameworkの実行ループ、EngineContext | application/EngineApp、SceneFactory、SceneManager、各Scene |
| 汎用Editorパネル登録と診断 | 作品固有のパネル、Scene切替、Player／Enemy／Wave |

MagFrameworkの既存の継承方式で、以下の4箇所をゲーム層へ接続する。

1. InitializeApplication：共通初期化後にゲームのScene管理と作品用設定を初期化する。
2. UpdateApplication：エンジン更新後にFrameTimeを渡し、ゲームを更新する。
3. RegisterApplicationRenderables：ゲームがRenderWorldへ描画対象を登録する。
4. FinalizeApplication：GPU完了後、共通サービスの破棄前にゲームを終了する。

EngineContextはエンジン所有、ゲームからは非所有参照とする。SceneManagerやPlayerをEngineContextへ追加しない。初期化・更新・描画が例外終了した場合も終了処理を通す。ゲームの終了処理は部分初期化状態でも安全に呼べるものとする。

共通描画順とGPU待機は維持する。SceneManager終了時は、ゲームのthisを参照するパネルを先に解除する。作品用フォント・トレイル設定はゲームが選ぶ。トレイル保存先は実際に読み込んだ設定パスを使う。

## 検証と完了条件

- MagEngine単独およびソリューションのDebug／Releaseビルドが通る。
- scripts/VerifyEngineBoundary.ps1でEngineからゲームへのinclude依存、ビルド対象、2プロジェクト制約を確認する。
- 現行ゲームがライブラリ利用側として動作する。ユーザーから今回「問題なく動作している」との報告を受けた。前回の操作確認待ちはこの報告を取り込む。
- 既存tests/RenderValidationTests.cppは保持し、ソリューションに独立したTest・検証専用アプリのプロジェクトを追加しない。必要な直接回帰検証は生成領域の一時実行ファイルで行える。ME-01〜ME-03ではtests/AssetSafetyTests.cpp、ME-04〜ME-06ではtests/ParticleRegressionTests.cppをscripts/RunAssetSafetyTests.ps1のSuite指定で実行する。現構成で未実行のテストを成功扱いにしない。
- CIはソリューションビルド、境界検査、Gameの成果物収集を行う。Hosted CIはローカルビルドとは別に扱う。
- 日本語コメント・既存の区切り線・所有権の記述を維持し、仕様と履歴を同時更新する。

今回の構成整理ではゲーム挙動を変更しない。内部基盤 → 制作エディタ → 独自技術の順で進める方針も維持する。第1段階の安全性課題まで全て解決済みとはしない。

最新の確認結果とObsidianへの反映状況は[履歴](MagEngine_04_仕様改修履歴.md)を参照する。
