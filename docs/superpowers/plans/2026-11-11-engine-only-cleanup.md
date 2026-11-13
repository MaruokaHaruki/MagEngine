# エンジン専用構成への整理 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 以前に選択された、このチャットでの順次実装を維持する。

**Goal:** 旧ゲームコードを削除し、リソースとエディターを維持した新作制作の出発点を作る。

**Architecture:** MagEngine.libとGame.exeの2プロジェクトを維持する。Game側には最小起動処理と既存EditorHostだけを残し、SceneManager経由の旧ゲーム起動を除く。資産の場所と内容は変えない。

**Tech Stack:** C++20、Visual Studio／MSBuild、DirectX 12、既存ImGui、PowerShell。

**Spec:** [承認済み設計](../specs/2026-11-11-engine-only-cleanup-design.md)

## Global Constraints

- `resources/` 以下は名前・場所・内容を変更しない。通常起動で旧レベルや旧ゲームの設定を自動読込しない。
- エンジン、エディター、最小の起動処理を保持。MagEngine／Gameの2プロジェクトとGame→Engineの依存方向を維持する。
- externalsを変更しない。新しいゲーム機能、全体のフォルダ改名、生成物の一括削除は行わない。
- ブランチ `Ver2.1.0_Feature_NewGame` 上で作業。コミット・ブランチ操作をしない。実行前の未コミット変更は再確認して保持する。
- AGENTS.mdと既存の日本語コメント・API契約の規約を守る。実装済みと検証済みを区別する。

## Review Focus

1. 引数なし起動：Debugは空の制作画面、Releaseは制作UIなしで起動し、旧ゲームを読まない（Task 1）。
2. 明示したシーンと日本語・空白入りパス：既存の読込とパス境界検証を維持（Task 1）。
3. ゲームと混在した描画テスト：Enemy専用部分だけ削除し、RenderGraph等の共通検証を残す（Task 2）。
4. 削除後の参照：プロジェクト・include・検証スクリプトに欠損フォルダを必須とする経路を残さない（Task 2）。
5. 旧ゲーム由来の資産：未使用でも削除・再保存せず、相対パスとハッシュが一致する（Tasks 2／3）。

## ファイルと削除対象

パスはproject/基準。

| 対象 | 作業 |
| --- | --- |
| `application/EngineApp.h/.cpp` | 旧Scene所有と作品初期化を除き、EditorHostの接続だけ残す |
| `main.cpp`、`application/editor/EditorLaunchOptions.h/.cpp` | 解析後の起動既定値を解決。既存引数の拒否条件を保持 |
| `application/editor/EditorHost.*` | 保持。必要な説明の更新だけ行う |
| `application/collision/`、`enemy/`、`gameplay/`、`manager/`、`player/`、`tutorial/`、`ui/`、`utils/` | 配下の旧ゲームコードを削除 |
| `application/GameClearAnimation.h/.cpp`、`GameOverAnimation.h/.cpp`、`SceneTransition.h/.cpp`、`StartAnimation.h/.cpp` | 削除 |
| `scene/` | 旧Scene・Factory・Manager・基底構成を削除 |
| `Game.vcxproj`、`Game.vcxproj.filters` | 削除ファイルと専用include／フィルターを除く |
| `tests/RenderValidationTests.cpp` | EnemyGroupのinclude、3つのEnemyGroupFormationTestとmainの呼出だけ削除 |
| `tests/EditorDocumentTests.cpp` | 起動既定値の回帰検証を追加 |
| `scripts/VerifyEngineBoundary.ps1` | scene不在を許容し、残るゲーム層への逆依存検査を維持 |
| `scripts/RunAssetSafetyTests.ps1` | 必要なら既存方式でRender Suiteを追加し、共通描画テストを直接実行可能にする |
| README、現行仕様・分離設計・履歴・操作ガイド | 現在の構成と起動方法へ更新。過去資料は残す |

## Task 1: 旧ゲームを使わない起動経路へ切り替える

**Interfaces:** `bool ResolveEngineLaunchOptions(EditorLaunchOptions &options, const std::filesystem::path &runtimeRoot, bool editorAvailable, std::string &error)`を既存EditorLaunchOptionsに追加。解析後、mainが解決済みの絶対資産ルートとDebug可否を渡す。失敗時はoptionsを保持する。

- [ ] 既存Document Suiteへ`DefaultDebugOpensEmptyEditor`（空引数・絶対runtimeRootでeditor=true、scene空）、`DefaultReleaseHasNoEditor`（editorAvailable=falseでeditor=false）、`ExplicitSceneKeepsRuntimeMode`、`UnicodeProjectRootIsPreserved`、`InvalidRuntimeRootDoesNotMutateOptions`を追加する。
- [ ] `./scripts/RunEditorTests.ps1 -Suite Document`で未実装の失敗を確認する。既存の相対ルート拒否・シーン逸脱拒否も残す。
- [ ] 既定値はprojectRoot未指定ならruntimeRoot、scene空ならeditorAvailableをeditorへ反映する。明示シーンの実行モードは維持し、Releaseの明示--editor拒否も維持する。解析時の明示--editorには既存どおり絶対--projectが必要。
- [ ] EngineAppは常にEditorHostを構築する。SceneFactory／SceneManager、作品フォント・トレイルの自動ロードを削除。初期化・更新・描画登録・終了を既存ホストへ委譲し、部分初期化時の安全な終了と保存確認を維持する。
- [ ] Document SuiteをDebug／Releaseで実行。Expected: 両方PASS。

## Task 2: 旧ゲームファイルと参照を取り除く

**Interfaces:** Task 1のEngineAppがSceneManagerを必要としなくなった状態で実施。残すコンパイル対象はmain、EngineApp、application/editorとEngineライブラリ。

- [ ] 削除前に`resources/`の全相対パス・SHA256を生成領域へ記録する。削除対象の解決済み絶対パスがproject/applicationまたはproject/scene内であり、保持するeditor／EngineApp／resourcesを含まないことを確認する。
- [ ] 境界検査へ存在しないプロジェクト登録を拒否する検査を追加し、一時的な欠損ファイル登録を持つ検証用コピーで失敗を確認する。製品プロジェクトを壊した状態で保存しない。
- [ ] 表の旧ゲームコードを削除。単一PowerShell内で確認済みパスだけを操作し、別シェルへ削除パスを受け渡さない。
- [ ] Gameの登録・専用includeパス・フィルターを更新する。共通EngineSettingsと2プロジェクト構成は維持する。
- [ ] 境界検査はscene不在を正常に扱う。存在するapplicationへの逆依存検査は残す。
- [ ] RenderValidationTestsのEnemyGroupFormationTest_SlotOffsets／AttackAndEmptyUpdate／RuntimeMovementBoundsと呼出・includeだけ削除する。残りの関数とmainは保持する。Render Suiteは既存の直接コンパイル方式でEngineへリンクし、独立プロジェクトは追加しない。
- [ ] `./scripts/VerifyEngineBoundary.ps1`、`./scripts/RunAssetSafetyTests.ps1 -Suite Render`を実行。Expected: 2プロジェクトの境界検査と共通描画テストPASS。
- [ ] 残した製品コード・テスト・ビルド設定から削除先への参照を検索し、欠損参照0を確認する。過去文書の履歴とresources内の文字列は書換え対象外。

## Task 3: 起動・資産保持・回帰を確認して記録する

- [ ] 通常設定でDebug／Releaseのソリューションをビルド。既知のPDB／LTOエラーが再現した場合は生成領域の既存検証用設定で再確認し、製品設定は変更しない。Expected: 検証用設定を使った場合はその条件を明記した成功記録。
- [ ] 両構成でRunEditorTestsのAll、RunAssetSafetyTestsのAsset／Particle／Render、境界検査、git diff --checkを実行。Expected: 全PASS。ゲーム専用テストの削除をテスト総数減少の理由として記録する。
- [ ] Debug引数なしの空文書、Object3d追加・資産の割当・保存／再読込、明示--scene、Release UIなし起動を確認する。テスト文書は生成領域の制作ルートコピーに保存し、既存resourcesへ書かない。GUI確認はユーザーが許す場合だけ再開し、停止された場合は結果を未確認とする。
- [ ] resourcesの前後一覧・SHA256が全件一致し、git差分にも資産の変更がないことを確認する。生成物を削除しないため、旧出力は今回の検証成果物と区別する。
- [ ] 現行文書を更新し、承認設計の範囲内で別担当による最終コードレビューを受ける。重要な指摘は修正し、対応する検証を再実行する。
- [ ] 削除・保持範囲、確認結果、未確認事項、コミットなし、Obsidian未反映を報告する。

## 自己確認・状態

起動はTask 1、削除と参照・共通検証の保持はTask 2、資産の不変性と全体確認・文書はTask 3へ対応する。新しい実行基盤や資産管理機能は追加しない。設計・計画承認済み。Tasks 1〜3を実装・自動検証済み。手動GUIは再開せず、実EngineAppの有限フレーム検証と既存入力・保存CPU Suiteで確認。追加したStartup Suiteは生成領域の資産コピーと配置保存先を使用する。詳細はgenerated/engine-only-cleanup/progress.mdと仕様・改修履歴を参照。
