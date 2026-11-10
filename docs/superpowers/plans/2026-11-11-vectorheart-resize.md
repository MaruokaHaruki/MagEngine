# Vectorheartテーマとウィンドウ追従 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 前回選択された、このチャットでの順次実装を維持する。

**Goal:** ウィンドウ変更・DPI変更に追従する描画と、視認性を優先したMagEngine独自のVectorheart UIを既存機構で実装する。

**Architecture:** WinAppは最新のサイズ／DPI要求を保持し、Frameworkの安全なフレーム境界でDirectXCoreがサイズ依存資源を更新する。適用済み描画サイズをカメラ・画像・入力・必要なHUDへ伝える。テーマは小さな共通定義と既存ImGui描画で適用し、保存レイアウトと操作を維持する。

**Tech Stack:** C++20、Windows／DXGI／DirectX 12、同梱ImGui 1.91.9b、既存フォント、PowerShell／MSBuild。

**Spec:** [承認済み設計](../specs/2026-11-11-vectorheart-resize-design.md)

## Global Constraints

- 視認性、操作性、既存機能と操作方法の維持を優先。
- ダークグレー、ライムイエロー、シアンを基本とし、既存の4分類色を維持する。
- 既存ImGuiと描画系を活用し、不要な依存や大規模な作り直しを避ける。
- 現在のGPU待機、RenderWorld／RenderPass／RenderGraphの責務、2プロジェクトとGame→Engineの依存方向を維持する。
- externalsを変更しない。独立エディタープロジェクト、テーマ編集画面、常時アニメーション、新規外部フォントは追加しない。
- ウィンドウ追従を採用し、中央パネルの組替えだけでは描画資源を再生成しない。
- コミットやブランチ操作は依頼なく行わない。既存の未コミット実装と文書差分を保持する。
- AGENTS.mdとリンク先規約を確認し、日本語コメント・既存の区切り線・所有権と単位の契約を守る。

## Review Focus

1. 最小化と連続サイズ要求：0寸法の生成・描画を行わず、復帰時の最新要求だけ適用する（Tasks 1／2）。
2. 同サイズと反復リサイズ：RTV／DSV／SRVを増やさず、古い参照・状態・バックバッファ番号を使わない（Task 2）。
3. 4:3等の画面と復帰直後：カメラ、画像、選択ray、入力、HUDが同じ適用済みサイズを使う（Task 3）。
4. 100→150→200→100%の倍率変更：寸法が累積せず、フォントとクリック位置がずれない（Task 4）。
5. 狭い幅・長い名前・編集中の配置：操作が欠けず、装飾が入力や保存レイアウトを壊さない（Tasks 5／6）。

## ファイルの責務

パスはproject/基準。Engineの既存globを使い、追加ファイルはMagEngine.vcxproj.filtersへ登録する。検証用プロジェクトは追加しない。

| ファイル | 責務 |
| --- | --- |
| engine/base/core/WindowSurfaceState.h（新規） | 最新のサイズ／DPI要求と適用状態をGPUなしで検証できる値として保持 |
| engine/base/core/WinApp.h/.cpp | 実クライアント寸法、WM_SIZE／WM_DPICHANGED、DPI対応 |
| engine/base/core/DirectXCore.h/.cpp | GPU待機、ResizeBuffers、サイズ依存資源とビューの更新 |
| engine/base/core/SrvSetup.h/.cpp、engine/render/resource/texture/TextureManager.h/.cpp | 既存スロットのSRV書換えと寸法メタデータ |
| engine/base/framework/MagFramework.h/.cpp | フレーム開始前の要求適用と失敗・最小化の制御 |
| engine/camera/CameraManager.*、application/editor/EditorHost.*、engine/editor/EditorViewportMath.* | 縦横比と選択計算の更新 |
| engine/base/imGui/EditorTheme.h/.cpp（新規）、ImguiSetup.h/.cpp | 共通の色・寸法・フォントとDPI倍率 |
| engine/base/imGui/EditorUiSystem.*、EditorWorkspaceLayout.*、engine/editor/EditorPanels.* | サイズに応じた配置と限定した装飾 |
| tests/WindowSurfaceTests.cpp（新規）、tests/EditorViewportTests.cpp、tests/EditorWorkspaceTests.cpp | 状態、座標、テーマ・配置の直接検証 |
| scripts/RunEditorTests.ps1 | Surface Suiteと生成領域でのCPU検証 |

## 共通の検証と進め方

各タスクはテスト作成→失敗確認→最小実装→成功確認。UI装飾の自明な色代入を逐一テストせず、倍率・幅・入力・寿命など壊れると利用者へ影響する契約を検証する。CPU成功とGPU／画面成功は分ける。

Task 1でRunEditorTestsへSurfaceを追加し、Allへ含める。設定は既存スクリプトを再利用し、出力は生成領域。GPUを使う確認は明示的に別実行し、CPU Suiteがデバイス未初期化のままDirectXCoreを呼ばないようにする。同構成の検証を並行起動しない。

```powershell
msbuild MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64
./scripts/RunEditorTests.ps1 -Configuration Debug -Suite All
./scripts/VerifyEngineBoundary.ps1
git diff --check
```

ReleaseはConfigurationを変更する。まず通常設定を試し、既知のPDB／LTO問題が再発したら原因と検証用設定を分けて記録する。製品の最適化設定は変更しない。

## Task 1: 実クライアント寸法と最新要求を保持する

**Files:** Create WindowSurfaceState.h、WindowSurfaceTests.cpp。Modify WinApp.h/.cpp、RunEditorTests.ps1、MagEngine.vcxproj.filters。

**Interfaces:** `SurfaceSize { uint32_t width, height; }`、`WindowSurfaceState::Request(SurfaceSize)`、`Pending() -> optional<SurfaceSize>`、`Commit(SurfaceSize)`、`IsMinimized() -> bool`、`Applied() -> SurfaceSize`。`WinApp::GetWindowWidth/Height() const`を実サイズ取得へ移し、`GetSurfaceState()`を提供する。1280×720は初期値だけに使う。

- [ ] `LatestResizeWins`を書く。1600×900→1024×768要求後のPendingが1024×768であることをCheckする。
- [ ] `SameAppliedSizeDoesNotResize`、`MinimizedNeverProducesGpuSize`、`RestoreAppliesLatestPositiveSize`を書く。0×0では最小化扱い、Commit済みサイズへの要求は再生成不要、復帰後の正寸法だけが対象になることをCheckする。
- [ ] Surface Suiteを実行し未実装の失敗を確認する。
- [ ] WM_SIZEで要求を記録。WM_NCCREATE時のthis登録、WM_CLOSEの保留、ImGuiメッセージ処理を保持する。サイズ変更イベントが初期化途中に来てもGPUを触らない。
- [ ] 全GetWindowWidth／Height呼出元を調べ、静的呼出があれば適切な所有者経由へ移す。初期化前・閉じる途中のHWND不在も扱う。
- [ ] Surface Suiteを両構成で実行し、ビルドと境界検査を通す。

## Task 2: Descriptorを再利用して描画資源をリサイズする

**Files:** Modify DirectXCore.h/.cpp、SrvSetup.h/.cpp、TextureManager.h/.cpp、MagFramework.h/.cpp、WindowSurfaceTests.cpp。

**Interfaces:** `DirectXCore::ResizeSurface(SurfaceSize, string& error) -> bool`、`GetRenderSize() const -> SurfaceSize`、`TextureManager::RefreshRenderTextureViews()`、`SrvSetup::RefreshRenderTextureViews()`。更新成功時だけSurfaceState::Commitを呼ぶ。通常の資産Textureは更新対象にしない。

- [ ] Surface Suiteへ`FailedApplyDoesNotCommitSize`を追加。実GPU確認では適用前後の寸法とDescriptorAllocatorの割当数を記録する小さな検証手順を用意し、1600×900↔1024×768を繰り返して割当数が増えないことを確認する。
- [ ] 失敗を確認した後、既存のCreateRenderTargetViews／CreateRenderTextureRTV等を「初回割当」と「既存ハンドルへビュー作成」に分ける。DSV、TextureManagerのRenderTexture0／1、予約SRV、FrameworkのeditorImageDescriptorsも同じスロットへ書き直す。
- [ ] FrameworkのUIフレームとGPUコマンド記録を始める前に最新要求を取り出す。GPU完了待ち→候補オフスクリーン画像と深度生成→旧バックバッファ参照解除→ResizeBuffers→バックバッファ取得とRTV→候補交換とDSV／SRV／メタデータ→viewport／scissor・フレーム状態更新の順で適用する。
- [ ] コマンドリスト、フレームコンテキスト等の旧参照も調査し、DXGIのResizeBuffers前提を満たす。現在バックバッファ番号とRenderGraphのSceneColor／Depth等の状態を新実体に合わせる。HResultはReleaseでも確認する。
- [ ] 最小化ではUI描画とGPU記録を開始しない。メッセージ処理は続け、無限の高負荷ループにせずWindowsのメッセージ待ちを使う。復帰後の正寸法を適用する。
- [ ] 更新失敗は理由を上位へ返す。ResizeBuffers後に安全な復旧ができない場合、無効な資源で次フレームへ進めず既存Finalize経路で終了する。
- [ ] 両構成ビルド、Surface Suite、D3D12デバッグ層での連続リサイズ・最大化・最小化復帰を確認。GPU確認不能なら未確認として残す。

## Task 3: カメラ・画像・入力・画面依存処理へ実サイズを伝える

**Files:** Modify CameraManager.h/.cpp、Camera.cpp、EditorHost.h/.cpp、EditorUiSystem.h/.cpp、EditorViewportMath.h/.cpp、MagFramework.cpp、EditorViewportTests.cpp。関連するHUD／画面演出は下記棚卸し結果の必要箇所だけ変更する。

**Interfaces:** `CameraManager::SetRenderAspect(float aspect)`で所有するカメラへ反映する。EditorHostはcontext.graphics->GetRenderSizeを使ってカメラを更新する。`EditorUiSystem::SetRenderSize(SurfaceSize)`を更新成功時に呼び、共通画像と入力のrenderSizeを一致させる。

- [ ] `FourByThreeCenterRay`、`ResizedImageMapsInputCorrectly`をViewport Suiteへ追加。1024×768の画像中心が正面rayと中心座標(0,0)になり、余白部分は非選択になることをCheckする。
- [ ] 未実装の失敗を確認し、16:9固定のEditorPanelsの投影とカメラ初期値を実際の画像縦横比へ移す。表示矩形と描画解像度を混同しない。
- [ ] SpriteSetupの正射影、ポストエフェクトの画素寸法、HUD、GameClearAnimation／GameOverAnimation／SceneTransition／StartAnimation／GameOverUIのキャッシュ寸法を調べる。初期化時だけコピーしている値は描画サイズ変更時または必要な更新時に再取得する。作品固有のワールド座標は変えない。
- [ ] 全ウィンドウ定数の一括置換はせず、変更した契約と移行呼出元を記録する。Viewport／Input／既存回帰Suite、両構成ビルドを確認する。
- [ ] 通常ゲームと編集モードで中心・四隅の入力、異なる縦横比、遷移中のサイズ変更を確認する。

## Task 4: DPI倍率とフォント更新を既存経路へ接続する

**Files:** Modify WinApp.*、ImguiSetup.*、MagFramework.*、Game.vcxprojまたは既存manifest（調査して一方だけ選ぶ）。Create EditorTheme.h/.cpp。Modify WindowSurfaceTests.cpp、EditorWorkspaceTests.cpp、MagEngine.vcxproj.filters。

**Interfaces:** `WinApp::GetDpiScale() const -> float`、`ImguiSetup::ApplyDpi(float scale)`。`EditorTheme`は100%時の寸法と色を持ち、`ApplyEditorTheme(ImGuiStyle&, float scale)`は毎回基準から計算する。設定編集UIや保存形式は追加しない。

- [ ] `DpiScalingDoesNotAccumulate`を書く。倍率1→1.5→2→1の適用後に行高・余白が最初の値へ戻り、1.5倍の値が基準の1.5倍になることをCheckする。無効倍率は安全に拒否する。
- [ ] 失敗を確認後、プロセスDPI設定をmanifest／APIで二重に指定していないか確認し、Per Monitor V2を既存初期化へ最小追加する。WM_DPICHANGEDの推奨矩形を使い、実クライアント寸法を再取得してTask 1へ渡す。
- [ ] 既存Firge-Regularを本文に使い、必要な見出し用サイズも同じアトラスへ登録する。フォントポインタを保持するloadedFonts_を更新し、古いポインタを使い続けない。
- [ ] 実際に倍率が変わったフレーム境界でGPU待機し、フォントアトラスとバックエンドのフォント画像を既存RebuildFonts／同梱ImGuiバックエンドの再作成APIで更新する。externalsやフォントファイルを変更しない。
- [ ] ヘッドレスで倍率検証、実画面で可能な倍率とモニター移動を確認する。利用できない環境は未確認とする。

## Task 5: 共通テーマと狭い幅での配置を整える

**Files:** Modify EditorTheme.*、ImguiSetup.cpp、EditorWorkspaceLayout.cpp、EditorUiSystem.cpp、EditorPanels.cpp、EditorWorkspaceTests.cpp。

**Interfaces:** 色と寸法はEditorThemeの基準値に集約。所属色を返す既存EditorCategoryColorはテーマを参照する。`EditorToolbarRows(float availableWidth, float scale) -> int`で幅による行分けを決め、実際のツールバー高さをドック構築へ伝える。しきい値は100%基準800pxを出発点とし実画面で調整する。

- [ ] `NarrowToolbarKeepsPrimaryActions`を追加。狭い幅では行数が増え、広い幅では1行を使えることをCheckする。保存・Undo・再生・レイアウト切替が幅不足で消えないことを実画面で確認する。
- [ ] ダーク背景#16191F、パネル#1D222A、本文#DCE2E8、ライム#D7F542、シアン#4ED9E8を提案値として共通定義へ移す。4分類色と警告／エラーの意味は維持する。
- [ ] SRGB描画先でUIが意図せず明るくならないか確認する。補正が必要ならUIの色だけを適用境界で扱い、ゲームの画像・マテリアル色を変更しない。
- [ ] 保存済みドック配置を保持し、サイズ変更でプリセットを再適用しない。メニュー・ツールバー・Inspectorの列幅を現在の幅から求め、名前が長い場合は本文折返し・スクロール・省略表示と説明で補う。
- [ ] Workspace／Session／Input Suiteと両構成ビルドを実行。800×600〜1920×1080で文字や操作の重なりを確認する。

## Task 6: 装飾を限定してMagEngineの個性を加える

**Files:** Modify EditorTheme.*、EditorUiSystem.cpp、EditorPanels.cpp、EditorWorkspaceLayout.cpp。既存のフォントとImDrawListだけを使用する。

- [ ] MAGENGINEの短いブランド見出し、ライムの短いアクセント線、シアンのフォーカス枠、見出しの45度切り欠き、少数の円・斜線をツールバー／タブ／見出しに追加する。ウィジェットの当たり領域は変更しない。
- [ ] 見出し用の既存太字フォントは短い英字へ限定し、日本語の本文・設定名はFirgeを維持する。未ロード時は本文フォントへ戻す。
- [ ] 狭い幅では装飾を省略して操作を優先。通常の入力欄、資産一覧、ゲーム画像へ模様を敷かない。点滅やアニメーションを入れない。
- [ ] 実画面で静止時・選択時・ホバー時・無効時・エラー時を見比べる。分類色が識別でき、本文を長時間読めるコントラストか確認する。装飾だけのテストやソース文字列テストは追加しない。

## Task 7: 全体回帰と確認結果を記録する

**Files:** Modify README.md、docs/MagEngine/MagEngine_03_改修仕様.md、MagEngine_04_仕様改修履歴.md、MagEngine_07_エディター操作ガイド.md、本計画。

- [ ] Debug／Releaseビルド、Surfaceを含むAll、既存Asset／Particle CPU回帰、層境界検査、diff --checkを実行する。
- [ ] 1280×720、1600×900、1920×1080、1024×768、800×600で画像全体、選択と入力、ツールバー、分類、保存／読込／再生を確認する。
- [ ] 連続リサイズ、最大化、最小化と復帰、DPI100／150／200%、ユーザー配置の復元、再生変更反映とUndo、ポストエフェクト、通常ゲームの遷移とRelease UIなし起動を確認する。
- [ ] GPUのDescriptor数とD3D12警告を確認し、失敗時は無効な資源で描画を続けないことを確認する。GPU資源生成失敗の強制検証ができない場合も未確認として書く。
- [ ] 別担当による最終レビューを実施し、重要な指摘は原因を確認して修正・検証する。ユーザー変更・保存配置を保持する。
- [ ] 操作ガイドへサイズ／DPI、テーマ、狭い幅での操作、失敗時の挙動を記載。実装済み／確認済み／未確認、ビルド環境の制限、コミット・Obsidianの未実施を分けて報告する。

## 自己確認と状態

サイズ要求はTask 1、資源と参照の更新はTask 2、縦横比と入力はTask 3、DPIはTask 4、幅と色はTask 5、限定装飾はTask 6、全体受け入れはTask 7へ対応付けた。Review Focusの5項目にも検証を割り当てた。共有APIは利用側と同時に変更する。

状態：設計承認済み。計画作成・自己確認済み。今回の変更は文書のみで、製品コード・テスト・ビルド設定は未変更。ユーザーの計画確認後、以前に選択済みの順次実装で進める。
