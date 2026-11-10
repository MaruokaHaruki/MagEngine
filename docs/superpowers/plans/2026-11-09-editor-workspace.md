# 制作エディターの画面整理 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. ユーザーが選択した、このチャットでの順次実装を維持する。

**Goal:** 分類色、中央のゲーム全体表示、固定ドック、3種類の配置、自作配置の保存・復元を共通のImGui画面へ実装する。

**Architecture:** 表示分類とパネル所有スコープを分離し、安定IDをレイアウト保存の基準にする。共通の画像パネルと入力変換をEngine側で管理し、編集ホストは選択・軸操作・編集操作を接続する。INIは独自の版付き保存文書にまとめ、初期ドック構築だけに同梱DockBuilderを限定利用する。

**Tech Stack:** C++20、Windows、DirectX 12、同梱ImGui 1.91.9b、既存nlohmann JSON、PowerShell／MSBuild。

**Spec:** [承認済み設計](../specs/2026-11-09-editor-workspace-design.md)

## Global Constraints

- MagEngineとGameの2プロジェクト、Game→Engineの依存方向を維持する。
- 既存のImGuiを使い、新しいUI技術や依存を増やさない。
- DockBuilderは初期ドック構築だけに限定し、EditorUiSystem付近の1か所へ閉じ込める。
- データ編集・保存・Undo・再生値の選択反映の契約は維持する。
- ReleaseのUIなし起動・文書読込は維持する。
- 通常Debug起動でも共通画面を使う。未登録のゲーム／Sceneの実体やパネルは生成しない。
- UI状態をゲーム配布資産やGit管理対象へ混ぜない。ユーザー変更と従来imgui.iniを保持する。
- 実装前にAGENTS.mdと必読文書を確認。日本語コメント、既存の区切り線、所有権と単位の契約を守る。
- コミット・ブランチ操作・Obsidian反映は自動実行せず、実施状況を報告する。

## Review Focus

1. Scene切替と遅延登録：色分けを変えても古いSceneのコールバックを残さず、再登録パネルは保存位置か標準領域へ戻る（Task 1／3）。
2. 日本語の保存名、同名上書き、プロジェクトキー衝突：別の配置やプロジェクトを黙って上書きしない（Task 2）。
3. 画面外・巨大サイズ・循環したドック・破損INI：ImGuiへ渡す前に拒否し、標準配置へ回復する（Task 2／3）。
4. 入力欄操作直後と復帰時：1フレームの入力漏れ、遅れた押下イベント、縮小画面でのマウス位置ずれを防ぐ（Task 4／6）。
5. 未確定ドラッグ中の配置切替・終了：手動変更だけを正しく確定し、レイアウト保存がシーン保存や再生値反映を代行しない（Task 5／7）。

## ファイルと境界

パスはproject/基準。既存Engineのソースglobを再利用し、追加ファイルはMagEngine.vcxproj.filtersへ登録する。新しい検証プロジェクトは作らない。

| 対象 | 役割 |
| --- | --- |
| engine/base/imGui/EditorPanelRegistry.h/.cpp（新規） | GPUなしのパネルID・分類・寿命・表示状態 |
| engine/base/imGui/EditorWorkspaceLayout.h/.cpp（新規） | 3プリセットのドック構築と共通シェル |
| engine/base/imGui/EditorLayoutStore.h/.cpp（新規） | 保存文書、INI検証、名前・保存先、安全な読書き |
| engine/editor/EditorViewport.h/.cpp（新規） | 共通画像表示・入力対象状態・編集オーバーレイ接続 |
| engine/base/imGui/EditorUiSystem.*、ImguiSetup.* | パネル描画、色、メニュー、UIフレーム管理 |
| engine/editor/EditorPanels.*、application/editor/EditorHost.* | 制作ツールバー、設定、分離タブ、共通画像への接続 |
| engine/input/Input.*、engine/editor/EditorViewportMath.* | ゲーム画像基準の位置・移動量、入力許可 |
| application/EngineApp.*、engine/base/framework/MagFramework.* | 起動モードと現在フレームのUI／入力の接続 |
| scene/base/SceneManager.cpp、scene/privateScene/DebugScene.cpp、scene/publicScene/GamePlayScene.cpp | 安定ID・分類・所有スコープを明示して登録 |
| tests/EditorWorkspaceTests.cpp（新規）、tests/EditorInputTests.cpp | CPU／ヘッドレスImGui検証 |

既存EditorViewportMathは純粋な計算として保持し、GPU所有を混ぜない。EditorViewportはGPU待機後に破棄する永続SRVと、現在の画像領域を所有する。EditorUiSystemからGame型を参照しない。

## 共通の進め方と確認コマンド

各タスクはテスト作成→失敗の確認→最小実装→成功確認の順。UIの見た目は後述の実画面確認で判定し、ソース文字列の一致テストを作らない。

Task 1でRunEditorTests.ps1へWorkspace Suiteを追加する。テストは実際の登録・保存・ImGuiコンテキストを扱い、Window／GPUは初期化しない。ImGuiコードはビルド済みMagEngine.libからリンクする。Debug／ReleaseのCRTを揃える。テストの入出力先は生成領域とし、AppDataには書かない。同構成のテストは同じ出力を使うため並行実行しない。

project/で次を実行する。Suite Allは既存6系統とWorkspaceを含む。

```powershell
msbuild MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64
./scripts/RunEditorTests.ps1 -Configuration Debug -Suite Workspace
./scripts/RunEditorTests.ps1 -Configuration Debug -Suite All
./scripts/VerifyEngineBoundary.ps1
git diff --check
```

ReleaseはConfigurationだけ変更する。通常設定のビルドを先に試す。既知のC1902／LNK1103が再現した場合は原因を切り分け、検証用の生成領域の設定と通常設定を分けて記録する。製品の最適化設定を黙って変えない。

## Task 1: 分類とパネル寿命を分離する

**Files:** Create EditorPanelRegistry.h/.cpp、tests/EditorWorkspaceTests.cpp。Modify EditorUiSystem.h/.cpp、EditorPanels.cpp、SceneManager.cpp、DebugScene.cpp、GamePlayScene.cpp、scripts/RunEditorTests.ps1、MagEngine.vcxproj.filters。

**Interfaces:** `EditorUiCategory { Authoring, Engine, GameDebug, Tools }`、`EditorPanelScope { Engine, Scene }`、`EditorDockRegion { Left, Center, Right, Bottom }`。`EditorUiPanelDesc`は`id, name, category, scope, defaultRegion, defaultOpen, isOpen, drawFunc`。`EditorPanelRegistry::Register(EditorUiPanelDesc) -> bool`、`Unregister(std::string_view id)`、`ClearScenePanels()`、`Find(std::string_view id) -> EditorUiPanelDesc*`、`Panels() -> const std::vector<EditorUiPanelDesc>&`。UiSystemはregistryへ委譲し、公開登録を`RegisterPanel(EditorUiPanelDesc) -> bool`へ統一する。

- [ ] `ClassificationDoesNotChangeLifetime`を書く。GameDebug/Engine所有のScene SwitcherとGameDebug/Scene所有のGamePlayを登録し、ClearScenePanels後に前者だけ残ることをCheckする。
- [ ] `StableIdSurvivesRenamedLabel`、`DuplicateIdsAreRejected`を追加。表示名が同じでもIDが違えば別対象、同じIDの二重登録は失敗することをCheckする。
- [ ] Workspace Suiteで未実装による失敗を確認する。
- [ ] Registryを実装し、全RegisterPanel／UnregisterPanel／SetPanelOpen呼出元を移行する。ImGui名は`表示名###安定ID`、操作APIはIDで参照する。エンジン常駐IDは`engine.input`等、制作は`authoring.objects`等、Sceneは`game.scene.gameplay`等に固定する。
- [ ] 既存登録の寿命を維持する。Scene SwitcherはGameDebug／Engine所有、GamePlayとDebug SceneはGameDebug／Scene所有。制作パネルは編集Hostの明示解除で管理し、通常Scene切替と混同しない。
- [ ] Workspace SuiteとDebug／Releaseビルド・境界検査を実行。登録失敗をassertだけに依存させない。

## Task 2: レイアウト文書の安全な保存と検証

**Files:** Create EditorLayoutStore.h/.cpp。Modify EditorWorkspaceTests.cpp、RunEditorTests.ps1、MagEngine.vcxproj.filters。

**Interfaces:** `EditorLayoutSnapshot`は`version=1, name, imguiVersion, ini, panelOpen: map<string,bool>, selectedLayout`。`EditorLayoutStore(path root)`は保存先を明示的に受ける。`SaveNamed(const EditorLayoutSnapshot&, bool allowOverwrite, string& error) -> bool`、`LoadNamed(string_view name, EditorLayoutSnapshot&, string& error) -> bool`、`ListNames(string& error) -> vector<string>`、`SaveSession(string_view mode, const EditorLayoutSnapshot&, string& error) -> bool`、`LoadSession(string_view mode, EditorLayoutSnapshot&, string& error) -> bool`。`ValidateLayout(const EditorLayoutSnapshot&, string& error) -> bool`はCPUのみ。

- [ ] `NamedLayoutRoundTrip`、`SessionDoesNotOverwriteNamedLayout`、`JapaneseNameAndProjectIsolation`、`RejectUnsafeNameAndFutureVersion`、`FailedSavePreservesOriginal`、`FailedLoadPreservesSnapshot`を書く。保存名は前後空白除去後1〜64 Unicode文字、NUL・制御文字・`/`・`\`・`.`と`..`単独を拒否する。
- [ ] `RejectOversizedAndCyclicIni`を書く。INI上限1MiB、Window上限128、ドックノード上限256、非有限／非正のSize、重複ノードID、存在しない親、循環を拒否する。未知セクションや同梱版と互換性のないINIも拒否し、入力文書を変更しない。
- [ ] Suiteを実行し失敗を確認する。
- [ ] JSONと同一フォルダの一時ファイルからの置換保存を実装。既存EditorDocumentStoreのWindowsファイル操作契約を参考にするが、シーン形式と保存先は混ぜない。
- [ ] ファイル名は正規化名のFNV-1a 64bitを16進にして生成し、保存内容のnameと比較して衝突時は拒否する。名前をパスとして連結しない。sessionは`session-edit.json`／`session-game.json`に分ける。
- [ ] INIはImGuiのWindowとDocking設定に限定する。Capture時に対象外のTable設定等は保存対象から除く。ドックが未作成のウィンドウのみの配置も許可し、有限の画面外座標はTask 3でクランプする。成功と失敗の保存／復元をWorkspace Suiteで確認する。

## Task 3: 共通シェルと3プリセットを構築する

**Files:** Create EditorWorkspaceLayout.h/.cpp。Modify EditorUiSystem.h/.cpp、ImguiSetup.h/.cpp、EditorWorkspaceTests.cpp、MagEngine.vcxproj.filters。

**Interfaces:** `EditorLayoutPreset { Standard, SceneFocus, Debug }`。`EditorWorkspaceLayout::Build(EditorLayoutPreset, const EditorPanelRegistry&, ImGuiID dockspace, ImVec2 size)`、`DockNewPanel(const EditorUiPanelDesc&, ImGuiID dockspace)`。`EditorUiSystem::QueuePreset(EditorLayoutPreset)`、`QueueRestore(EditorLayoutSnapshot)`、`CaptureLayout(string name) -> EditorLayoutSnapshot`。適用はNewFrame後・最初のBegin前だけ。

- [ ] ヘッドレスImGuiで`PresetCreatesDockedWindows`、`DefaultPanelsExcludeDebugging`、`LatePanelUsesSavedOrDefaultRegion`、`InvalidRestoreFallsBackToStandard`を書く。1フレーム描画し各既知パネルのDockIdが0でないこと、SceneFocusで画像が広くなること、未登録パネルを生成しないことを確認する。
- [ ] 未実装の失敗を確認する。
- [ ] 不透明の全画面ルートと共通ツールバー領域、その下のDockSpaceを実装する。毎フレームSetNextWindowPosで個々のパネル位置を強制する旧リセット処理を撤去する。DockBuilderはこのcppだけで使い、同梱版のAddNode→SetNodeSize→SplitNode→DockWindow→Finishの順を守る。
- [ ] Standardは左20%・右25%、中央下25%。SceneFocusは補助パネルを閉じる。Debugは右を確認タブ、下を性能・メッセージ用タブにし、利用可能なPerformanceとメッセージだけを最初に開く。一覧が未登録なら左の幅を中央へ戻す。Standardでは最初にSceneFocus相当の空枠を作らない。
- [ ] メニュー／ドック／UI全体を非表示にする既存項目を共通シェル中は出さず、復帰とパネル表示の導線を残す。復元前にValidateLayoutを通し、失敗なら理由を表示して標準配置を構築する。既存ファイルは自動上書きしない。
- [ ] 画面外のフローティング配置は現在のメイン画面へ収める。DPI・小画面をヘッドレスの異なるDisplaySizeで確認し、ドックの最低サイズが必要なときはタブ／スクロールで補う。CPUテストと両構成ビルドを実行する。

## Task 4: ゲーム画像の座標と入力状態を共通化する

**Files:** Create engine/editor/EditorViewport.h/.cpp。Modify EditorViewportMath.h/.cpp、Input.h/.cpp、EditorInputTests.cpp、EditorViewportTests.cpp、RunEditorTests.ps1、MagEngine.vcxproj.filters。

**Interfaces:** `EditorViewportInputState`は`imageRect, renderSize, focused, allowInput`。`EditorViewport::Draw(uint64_t texture, Vector2 renderSize)`、`GetInputState() -> const EditorViewportInputState&`、`SetOverlay(function<void(const ViewportRect&)>)`。画像矩形とマウス位置は同じImGui座標系に揃える。`Input::SetApplicationViewport(optional<EditorViewportInputState>)`で通常のウィンドウ基準と画像基準を切り替える。

- [ ] `ScaledImageMapsToRenderCenter`を書く。画像rect=(100,200,640,360)、renderSize=(1280,720)で、点(420,380)の中心基準位置=(0,0)、点(580,470)=(320,180)、移動(10,5)=(20,10)をCheckする。画像外とサイズ0では中立値、viewport未指定では既存の取得値をCheckする。
- [ ] `ClickRequiredForGameInput`、`TextOrModalBlocksAllGameDevices`、`BlockedPressDoesNotReplayOnRestore`を書く。遮断中の押下は復帰時のTriggerとして発火せず、継続押下のPushとトリガーを区別する。
- [ ] Input／Viewport Suiteの失敗を確認する。
- [ ] 既存FitViewportとray／境界計算を再利用して共通画像パネルを実装する。ゲーム画像のクリックでfocused、画像外クリックで解除。allowInputは現在位置・前面・テキスト／モーダルを反映し、前フレームのホバーだけで許可しない。画像枠と「ゲーム操作中／クリックして操作」で状態を示す。
- [ ] マウス位置・移動量は描画解像度へ換算し、ホイール・ボタン番号は維持。入力遮断中もOS状態と前状態を取得し続け、復帰時の押下を再生しない。選択や編集カメラ用のImGui入力にはこの変換を適用しない。
- [ ] Input／Viewport SuiteをDebug／Releaseで通す。未指定時の既存Input契約も検証する。

## Task 5: 制作操作を整理し色と見出しを統一する

**Files:** Modify EditorPanels.h/.cpp、EditorUiSystem.h/.cpp、ImguiSetup.cpp、EditorWorkspaceTests.cpp、EditorSessionTests.cpp。

**Interfaces:** `EditorUiSystem::SetToolbar(function<void()>)`、`SetBeforeLayoutChange(function<void()>)`。登録解除時はコールバックも空にする。EditorPanelsは既存FinishEditingを切替前処理へ接続。DrawViewportの選択・軸操作部分は`DrawViewportOverlay(const ViewportRect&)`へ移し、画像のBegin／Imageと入力状態を共通EditorViewportへ任せる。

- [ ] `LayoutChangeCommitsOnlyActiveProperty`をEditorSessionTestsへ追加し、他のシミュレーション変化を反映候補へ含めず手動編集1項目だけ確定することをCheckする。メニューの名前付き保存だけではこの確定処理を呼ばないことをWorkspace側コールバックでCheckする。
- [ ] 保存・読込・Undoを左、再生制御を中央、状態・レイアウト選択を右へまとめる。制作操作という独立した150pxパネルを撤去する。通常ゲームでは保存／Undoや編集ホストの再生制御を出さず「実行中」と表示する。
- [ ] 資産、メッセージ、再生変更反映をそれぞれ安定IDの別パネルへ分け、下部へドック。候補数はタブラベルへ付け、IDは変えず、勝手に選択タブを変えない。操作・エラー状態の生成元は保持する。
- [ ] 分類色を制作#6EA8FE／エンジン#51C7B4／ゲームデバッグ#E5B567／ツール#B99AE8としてタブ・見出し・表示メニューへ適用。エラー赤／警告黄と所属色を分ける。分類名も表示し、色だけで区別しない。
- [ ] 共通行高・余白・ラベル幅を設定。Inspectorは名前／表示、Transform、描画設定、折りたたんだ詳細に分け、IDは詳細へ移す。X/Y/Zの文字と色目印を添える。既存ゲーム固有パネル内の全面整形はしない。
- [ ] Session／Workspace Suiteを実行し、Debug／Releaseビルドを確認する。実画面でラベル欠け、色の識別、狭い横幅のツールバーを確認する。

## Task 6: 2つの起動経路へ共通画面と当該フレームの入力を接続する

**Files:** Modify MagFramework.h/.cpp、EngineApp.h/.cpp、EditorUiSystem.h/.cpp、EditorHost.h/.cpp、EditorPanels.*、EditorWorkspaceTests.cpp。Read DirectXCore.*、RenderGraph.*、CameraManager.cpp、main.cpp。

**Interfaces:** EditorWorkspaceLayout.hに`EditorWorkspaceMode { Edit, Game }`を定義。`EditorUiSystem::ConfigureWorkspace(path projectRoot, EditorWorkspaceMode mode)`、`GetViewport() -> EditorViewport&`。Frameworkは画像用SRVを永続所有し、描画前から有効な同じSceneColorリソースへ接続する。Hostにあった重複画像用Descriptorを削除する。

- [ ] `NormalAndEditModeRegisterOnlyAvailablePanels`、`SceneReplacementDoesNotReuseCallback`、`UiFrameEndsOnExitOrException`を追加。通常起動には編集Hostの操作がなく、Scene更新中の登録変更は次のフレームで表示されることをCheckする。
- [ ] 失敗を確認する。
- [ ] Debugのフレーム順をInput.Update（生状態取得）→ImGui.Begin／安全な配置適用／共通UI描画→現在の入力状態確定→カメラ等の更新→UpdateApplication→描画フェーズ→ImGui.Render／描画へ整理する。NewFrameと終了を各フレーム1回に限定し、終了・例外でも開いたUIフレームを閉じる。Releaseの順序は変えない。
- [ ] 通常ゲームのUIコールバック描画中だけゲーム入力マスクを解除し、カメラとゲーム更新へ現在のマスク・座標変換を適用する。編集Hostの重複した前フレーム入力判定を撤去し、共通状態を使う。UI上で操作してもシミュレーション更新は継続する。
- [ ] ConfigureWorkspaceは共通UIが生成済み、ゲーム／編集パネルが登録済みの後に呼ぶ。レイアウト適用は次のUIフレーム開始まで遅延する。projectRootはlaunchOptionsの明示指定を優先し、通常起動で未指定ならexeフォルダを使う。
- [ ] 既存の全画面合成を不透明シェルで覆い、中央画像だけにゲームを見せる。現在のSceneColorはPixelShaderResource状態で参照し、RenderGraphの状態・Fence・Descriptor寿命を維持。新しい毎フレームDescriptor確保やbackbufferの直接SRV化はしない。
- [ ] CPU回帰・両構成ビルド・境界検査を実行。通常Debug、--editor、ReleaseのUIなし起動の各経路を実機確認する。CameraManager等もUI入力で動かないことを確認する。

## Task 7: メニューからの保存・セッション復元を接続する

**Files:** Modify EditorUiSystem.h/.cpp、EditorLayoutStore.*、EditorWorkspaceLayout.*、EngineApp.cpp、EditorWorkspaceTests.cpp。保存先取得に必要なWindows標準APIだけを使用する。

**Interfaces:** `EditorUiSystem::SaveLayoutAs(string_view name, bool overwrite, string& error) -> bool`、`QueueNamedLayout(string name)`、`SaveWorkspaceSession(string& error) -> bool`。Storeの寿命はUIより長くせず、終了保存は登録パネルを解除する前に行う。

- [ ] `SessionRestoresModeSeparately`、`NamedLayoutUnchangedAfterExit`、`UserRenamedPanelKeepsDock`、`UnregisteredIdsNeverCreateCallbacks`を書く。UIの保存先は生成領域へ注入し、AppDataへ実テストデータを残さない。
- [ ] 失敗を確認する。
- [ ] LocalAppData/MagEngine/Editor/<project-key>をWindows KnownFolderから求める。project-keyは正規化絶対パスのUTF-8をWindowsの大文字小文字規約へ揃えてFNV-1a 64bit化する。保存場所のプロジェクト識別情報を比較し、衝突なら既存データを使わず通知する。
- [ ] 日本語の「レイアウト」メニューに3プリセット、名前付き保存、保存一覧、標準へ戻すを接続する。名前付き保存は上書き確認後のみallowOverwrite=true。保存エラーはメッセージパネルへ表示する。削除UIは作らない。
- [ ] 初回は標準、次回は起動モード別のセッションを復元。正常終了時にはレイアウトだけをセッション保存し、名前付き配置は上書きしない。保存失敗はログとメッセージへ記録し、シーン保存や終了処理の後始末を妨げない。
- [ ] 共通UI初期化時にImGuiIO::IniFilenameをnullptrにし、従来INIは消さない。保存したパネルID／開閉とImGui文字列を整合させ、未登録IDの情報は保持しても実体や関数を復元しない。Workspace Suiteと両構成ビルドを実行する。

## Task 8: 回帰・見た目・操作ガイドを仕上げる

**Files:** Modify docs/MagEngine/MagEngine_07_エディター操作ガイド.md、MagEngine_00_索引.md、MagEngine_03_改修仕様.md、MagEngine_04_仕様改修履歴.md、README.md、本計画。

- [ ] 通常設定でDebug／Releaseをビルドし、新規Workspaceを含むRunEditorTests -Suite All、既存Asset／ParticleのCPU検証を両構成で実行する。境界検査とdiff --checkも実行する。通常設定の失敗と検証用設定の成功は別記する。
- [ ] 通常Debugと編集起動で、初期配置の重なりなし、4分類、ゲーム全体表示、3プリセット切替、遅延登録、名前付き保存、組み替え後の終了／再起動を実画面で確認する。
- [ ] UI欄へ文字入力中のキーとゲームパッド、画像外クリック、クリックで入力対象復帰、画像中心でのマウス位置、画像のドック移動・拡縮後の選択・軸操作を確認する。最小化・復帰・DPI変更も含める。
- [ ] 未確定ドラッグ中のレイアウト切替、Esc取消、反映待ちの配置保存、同名保存確認の取消、壊れたJSON／INI、書込不可、画面外配置の復旧を確認する。
- [ ] 既存ゲームのタイトル→ゲーム→シーン切替→終了と、Release文書起動を回帰確認する。Hosted CIはローカルと区別する。
- [ ] 操作ガイドへ分類、クリックでゲーム入力を有効にする操作、プリセット、保存場所、名前付きと自動セッションの違い、復旧方法を記載。実施済み／未確認・残る制約・Obsidian未反映を報告する。画面ツール障害時は実画面の受け入れを未完了として残し、CPUテストだけで見た目を保証しない。

## 自己確認と状態

分類と寿命はTask 1、保存の入力境界はTask 2、固定配置はTask 3、画像と入力はTasks 4／6、色と整理はTask 5、名前付き／自動復元はTask 7、実画面はTask 8へ対応付けた。Review Focusの5項目にも担当タスクと検証を割り当てた。共通API名とパネルIDは消費側と同時に変更する。

状態：設計承認済み、計画作成・自己確認済み。製品コード・ビルド設定・テストの変更なし。計画のユーザー確認後、以前に選択済みの順次実装で進める。コミット・Obsidian反映は未実施。

## 2026-11-11 実行結果

Tasks 1〜7の初期実装とTask 8の基本検証・操作ガイド更新を実施した。新規Workspaceを含む7系統のAllがDebug／ReleaseでPASS。検証用設定で両構成のビルドと層境界検査を確認した。Debugの既存Asset／Particle CPU回帰も通過した。

別担当の重要指摘2件を修正し、壊れたセッションの保全とUI診断中の入力制限解除を検証した。レイアウト適用もパネル描画前へ移した。実画面では中央画像と左右・下部パネルの重なりなしを確認した。

全てのプリセットの手操作、DPI変更・最小化復帰、通常ゲームの全操作、配布コピー、Hosted CIは未確認。これらの受け入れ手順は完了扱いにしない。通常ビルド環境のPDB問題、コミット・Obsidian反映も未解決／未実施として維持する。
