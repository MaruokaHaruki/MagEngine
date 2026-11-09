# 制作エディター基盤 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Object3dの配置・設定・保存・Undo・再生調整を共通のImGui制作画面から行えるようにする。

**Architecture:** CPU上の編集文書を正本とし、描画実体と再生データを分離する。エンジンは文書・操作・UI契約を持ち、Game側の編集ホストが実体と再生を所有する。Object3dを先に完成させ、Cloudは後続とする。

**Tech Stack:** Windows、C++20、DirectX 12、既存ImGui、既存nlohmann JSON、PowerShell／MSBuild。

**Spec:** [承認済み設計](../specs/2026-11-09-editor-foundation-design.md)

## Global Constraints

- MagEngine.libとGame.exeの2プロジェクトを維持。新しいEditor/TestプロジェクトやUIライブラリを追加しない。externalsは変更しない。
- GPU待機・RenderWorld／RenderPassの責務を維持。ゲーム固有のSceneやPlayerをエンジンへ取り込まない。
- 初期版はフラットなオブジェクト一覧、単体選択、ワールド軸の操作に限定する。
- 親子付け、複数選択、Prefab、アニメーション編集、任意の既存ゲームシーンの自動変換、C++ホットリロード、スクリプトエディターは対象外。
- エディターUIはまずDebug対象とし、ReleaseゲームはUIなしで保存文書から実体を生成できる。
- 日本語のAPI契約・所有権・単位と既存の3段階の区切り線を守る。作業前にAGENTS.mdと必読文書を読む。
- ユーザー変更を保持。各タスクで関連する既存実装、全呼び出し元、差分を再確認する。コミット・ブランチ操作は依頼がない限り行わない。
- 文書はリポジトリが正本。Obsidianの反映／未反映を報告する。現時点では未反映。

## Review Focus

1. 日本語・空白を含む制作ルート、実行時の作業ディレクトリ変更でも正しい資産と保存先を使う（Task 3／5）。
2. 同名オブジェクト、削除後の選択、Undoによる復元でもIDと対象が取り違わされない（Task 1／2／6）。
3. ドラッグ中にフォーカスが外れる、数値入力中にショートカットを押す場合も履歴とゲーム入力が壊れない（Task 6／7）。
4. 画面縮小・最小化・DPI変更後も選択座標が合い、無効な大きさでGPU資源を作らない（Task 7）。
5. 再生の生成失敗、停止直後の再生、反映待ちの読込要求で文書とGPU資源の寿命を維持する（Task 8）。

## 実装前の確認とタスク共通の検証

以下のパスはproject/からの相対。新規実装はengine/editor/、ゲーム側の接続はapplication/editor/へ置く。既存ファイルを一括移動しない。新規cpp/hは所属するvcxprojとfiltersへ登録する。

各タスクは失敗確認→最小実装→成功確認→関連仕様・履歴更新の順。将来のテスト名は下記で固定する。新しいCPUテストのRunEditorTests.ps1はSuiteをDocument／History／Persistence／Viewport／Session／Allから選べるようにし、終了コード0のみを成功とする。Releaseでもassertが消えて検証が無効にならないCheck関数を使う。

Task 1で追加する検証コマンド（project/で実行）：

```powershell
msbuild MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64
./scripts/RunEditorTests.ps1 -Configuration Debug -Suite All
./scripts/VerifyEngineBoundary.ps1
```

CPUテストはWindow／GPUなし。UIのチェックは別途実機で行う。ビルド失敗や未実施を成功扱いにしない。タスク途中は対象Suiteを使い、完了時にAllを実施する。

## ファイルの責務

| ファイル群 | 責務 |
| --- | --- |
| engine/editor/EditorDocument.* | 値だけの文書・安定ID・整合検証 |
| engine/editor/EditorHistory.* | Undo／Redoとドラッグのまとめ |
| engine/editor/EditorDocumentStore.* | JSON、競合検出、安全な保存、Blenderの限定インポート |
| engine/editor/EditorTypeRegistry.* | 型の検証・設定UI・選択境界の登録契約 |
| engine/editor/EditorPanels.* | 一覧・設定・資産・コマンド導線 |
| engine/editor/EditorViewport.* | シーン画像・カメラ・座標変換・選択・軸操作 |
| engine/editor/EditorPlaySession.* | 値の再生コピー、明示変更候補、選択反映 |
| application/editor/EditorHost.* | 描画実体の所有・更新・再生接続 |
| tests/Editor*Tests.cpp、scripts/RunEditorTests.ps1 | 独立プロジェクトを作らない直接CPU検証 |

## Task 1: 編集文書と安定ID

**Files:** Create engine/editor/EditorDocument.h/.cpp、tests/EditorDocumentTests.cpp、scripts/RunEditorTests.ps1。Modify MagEngine.vcxproj/.filters。

**Interfaces:** `using EditorObjectId = uint64_t`（0は無効）。`EditorObjectRecord`はid、name、typeId、modelPath、MagMath::Transform、visible、nlohmann::json settingsを持つ。`EditorDocument`はobjectsと単調増加nextIdを持つ。`AddObject(EditorDocument&, EditorObjectRecord) -> EditorObjectId`、`FindObject(EditorDocument&, EditorObjectId) -> EditorObjectRecord*`、`ValidateDocument(const EditorDocument&, std::string& error) -> bool`。

- [ ] `StableIdsSurviveRemoval`、`DuplicateNamesDoNotAlias`、`RejectDuplicateIdsAndNonFiniteTransform`を書く。削除済IDが再利用されず、不正値の拒否理由を返すことを確認する。
- [ ] 既存RunAssetSafetyTests.ps1のコンパイル経路を参考にRunEditorTests.ps1を追加。生成先は../generated/validation/editor/<Configuration>。型未実装によるコンパイル失敗を確認する。
- [ ] 上記APIと文書検証を実装。ID枯渇も黙って再利用せず失敗とする。
- [ ] Debug／ReleaseのDocument Suiteを通す。テスト実行ファイルをソリューションへ追加していないことを境界検査で確認する。

## Task 2: Undo／Redoと編集トランザクション

**Files:** Create engine/editor/EditorHistory.h/.cpp、tests/EditorHistoryTests.cpp。Modify所属プロジェクト、RunEditorTests.ps1。

**Interfaces:** Task 1の文書を使用。`EditorHistory::Execute(EditorDocument&, const EditorDocument& replacement) -> bool`、`Undo(EditorDocument&) -> bool`、`Redo(EditorDocument&) -> bool`、`Reset()`。初期版は値文書の前後スナップショットで実装し、履歴上限100操作。選択はIDで持ち、不在なら解除する。UIの連続ドラッグは開始前文書を保持し終了時に1回Executeへ渡す。

- [ ] `UndoDeleteRestoresSameId`、`DuplicateUsesFreshId`、`NewEditClearsRedo`、`NoOpDoesNotAddHistory`を書く。履歴未実装の失敗を確認する。
- [ ] 最小の履歴と上限処理を実装。復元後もID発行の高水位を維持し、破棄されたRedo側のIDを別対象へ再利用しない。
- [ ] History Suiteを実行し全ケースが終了コード0になることを確認する。

## Task 3: 保存・読込・インポート

**Files:** Create engine/editor/EditorDocumentStore.h/.cpp、tests/EditorPersistenceTests.cpp。Modify所属プロジェクト、RunEditorTests.ps1。

**Interfaces:** `EditorDocumentStore::Load(const std::filesystem::path&, EditorDocument&, std::string& error) -> bool`、`Save(const std::filesystem::path&, const EditorDocument&, std::string& error) -> bool`、`ImportBlender(const std::filesystem::path&, EditorDocument&, std::string& error) -> bool`。Storeは最後に正常読込／保存したファイル内容を競合判定用に保持する。形式バージョン1、型は初期版object3dのみ。

- [ ] `RoundTripPreservesIdsAndValues`、`RejectFutureVersionAndUnknownType`、`LoadFailureLeavesDocumentUntouched`、`SaveFailurePreservesOriginal`、`ExternalEditIsConflict`、`JapaneseProjectPath`、`RejectHierarchyImport`を書く。未実装で失敗を確認する。
- [ ] 一時文書を検証後に交換。保存は同一フォルダの一時ファイルへ書込み・close成功後、Windowsの置換操作で更新する。失敗時の元ファイル維持をファイル操作の失敗注入で確認する。
- [ ] 相対資産パスのルート外参照、欠損資産、重複ID、不正値を拒否。検証には明示した制作ルートを使い、カレントディレクトリへ依存させない。Blender変換は既存LevelDataLoaderの実処理・fixtureで単位を確認して一度だけ適用する。親子データは拒否する。
- [ ] Persistence SuiteをDebug／Releaseで通す。書込失敗を成功扱いしない。

## Task 4: 型登録とObject3d実体の同期

**Files:** Create engine/editor/EditorTypeRegistry.h/.cpp、application/editor/EditorHost.h/.cpp。Modify MagEngine.vcxproj/.filters、Game.vcxproj/.filters。Read engine/graphics/object3d/Object3d.*、engine/render/resource/model/ModelManager.*。

**Interfaces:** `EditorTypeDescriptor`はtypeId、defaultSettings、およびvalidate、drawSettings、getBoundsコールバックを持つ。`EditorTypeRegistry::Register(EditorTypeDescriptor) -> bool`、`Find(std::string_view) -> const EditorTypeDescriptor*`。設定UIは変更候補値を返し、実体へ直接書かない。`EditorHost::ApplyDocument(const EditorDocument&, std::string& error) -> bool`、`Update(const FrameTime&)`、`RegisterRenderables(RenderWorld&)`、`Finalize()`。生成・適用はHostへ登録する型別ファクトリーで行い、EngineがGameを参照しない。

- [ ] 型重複、未知型、不正設定が文書変更前に拒否されるCPU検証をDocument Suiteへ追加し、未実装で失敗確認する。
- [ ] object3d型とHostを実装。モデル成功後に候補実体を確定し、失敗時は既存実体を保持する。共有モデルキャッシュと個別Object3dの所有権を区別する。最初のboundsはモデルから得たローカルAABBを使う。
- [ ] CPU検証と両構成ビルド、境界検査を実行。実機で文書更新後の描画とモデル失敗時の維持を確認する。

## Task 5: 編集ホストの起動と制作ルート

**Files:** Modify main.cpp、application/EngineApp.h/.cpp、engine/base/framework/MagFramework.h/.cpp。Create application/editor/EditorLaunchOptions.h/.cpp。Modify Game.vcxproj/.filters、README.md。

**Interfaces:** `ParseEditorLaunchOptions(std::span<const std::wstring>, EditorLaunchOptions&, std::string& error) -> bool`。オプションは`--editor --project <absolute-root> [--scene <relative-scene>]`。EngineAppに起動設定を渡し、editor指定時のみEditorHostへUpdate／RegisterRenderables／Finalizeを委譲。Releaseは`--scene`で文書を読み込んで実体を生成可能にし、`--editor`要求は理由を示して拒否する。

- [ ] 日本語・空白のパス、相対ルート、不正引数、ルート外scene指定、作業ディレクトリ変更を含む引数解析テストをDocument Suiteに追加。失敗を確認する。
- [ ] WindowsのUnicodeコマンドライン解析を使って実装。制作ルートは起動引数またはファイル選択で明示指定し、未指定なら保存不可と理由を表示。ローカル設定に最後のルートのみ保持できる。保存先は<project>/resources/config/scenes/とし、配布コピーを既定値にしない。
- [ ] 終了時の保存／破棄／取消を扱うため、Frameworkの終了要求をゲーム側が一時保留できる最小の仮想フックを追加する。正常終了、部分初期化失敗、通常ゲームの既存経路を保つ。
- [ ] テスト・両構成ビルドと通常ゲーム／編集起動を実機確認する。

## Task 6: 一覧・設定・資産・保存導線

**Files:** Create engine/editor/EditorPanels.h/.cpp。Modify engine/base/imGui/EditorUiSystem.h/.cpp、engine/base/imGui/ImguiSetup.cpp、application/editor/EditorHost.*。Create docs/MagEngine/MagEngine_07_エディター操作ガイド.md。

**Interfaces:** `EditorPanels::Register(EditorUiSystem&)`、`Unregister(EditorUiSystem&)`。Panelsは文書・履歴・選択ID・型レジストリの非所有参照を受け取る。`RequestDocumentAction(EditorDocumentAction)`で保存・読込・終了を要求し、未保存確認完了まで破壊的切替を行わない。dirty判定は保存済み値との比較とし、Undoで保存時点へ戻れば解除する。

- [ ] 保存後編集→Undoでdirty解除、選択削除の解除、同名対象の区別、未保存確認の取消による維持をCPU検証へ追加。失敗を確認する。
- [ ] 名前付き一覧、追加・複製・削除、object3dのモデル参照・Transform・表示、検索可能な対応資産一覧とエラー表示を実装する。実体へ直接変更せず文書と履歴を経由。日本語フォントは同梱資産を調査し、なければ既存Windows日本語フォントを実行環境で選んで読み込む。フォントをリポジトリへコピーしない。
- [ ] DockSpaceに左一覧・中央Scene・右設定・下資産／エラーの既定配置とレイアウト復元を実装。ImGui内部APIは使わず、既存公開APIで設定可能な範囲を利用する。
- [ ] 数値入力・ドラッグの確定と取消、ウィンドウ外へのフォーカス移動を確認。確定済み値のみ1履歴にまとめ、Escはドラッグ前へ戻す。文字入力中のDelete／Undoをオブジェクト操作へ漏らさない。
- [ ] 操作ガイドに実機手順を記載し、配置→複製→数値編集→Undo→保存→再読込を確認。古いデバッグUIが編集実体を直接操作しないことも確認する。

## Task 7: シーン画面・カメラ・選択・軸操作

**Files:** Create engine/editor/EditorViewport.h/.cpp、tests/EditorViewportTests.cpp。Modify engine/base/core/DirectXCore.h/.cpp、engine/render/post_effect/PostEffectManager.h/.cpp、engine/base/framework/MagFramework.h/.cpp、application/editor/EditorHost.*、engine/editor/EditorPanels.*。Read engine/camera/Camera.*、engine/input/Input.*、engine/graphics/line/LineManager.*。

**Interfaces:** `ViewportRect`は画面上の画像領域（余白を除く）。`ScreenToViewportRay(Vector2, const ViewportRect&, const Camera&) -> std::optional<EditorRay>`、`PickObject(const EditorRay&, const EditorDocument&, const EditorTypeRegistry&) -> EditorObjectId`。`EditorViewport::Draw()`は表示・操作候補の収集、`UpdateCamera(float seconds)`はホバー／フォーカス中だけカメラ更新する。`GetViewportInputState()`でHostに入力所有権を渡す。

- [ ] `LetterboxCoordinatesPickCorrectly`、`ZeroViewportReturnsNoRay`、`NearestBoundsWins`、`TransformRoundTripUsesRadians`、`DragProducesOneHistoryEntry`を追加し、未実装の失敗を確認する。CPUの変換・交差計算をWindowなしで検証する。
- [ ] 既存最終カラー出力をImGuiへ表示する経路を追加。初期版は既存レンダー解像度を維持し、画面内へ縦横比を保って表示する。必要なら最終出力の専用コピーを作り、backbufferを直接SRVにしない。RenderGraphへ使用状態を記録し、現行のFenceとDescriptor寿命を保つ。
- [ ] レターボックスを除いた画像領域で選択・編集カメラ・ワールド軸ハンドルを実装する。ローカルAABBとrayの変換で選択。近い正の距離を優先し、一覧選択でも補えるようにする。移動・回転・拡縮はTransformを変更し、負／ゼロスケールなど初期版の非対応値は理由を示して拒否する。
- [ ] 入力所有権をHost経由で明確にする。UIの文字入力・ポップアップ・他パネルは優先し、シーン外クリックでカメラを動かさない。再生ゲーム入力もシーン画像への明示フォーカス時だけ許可する。
- [ ] Viewport Suite、両構成ビルド、境界検査を実行。実機でDock配置変更、最小化、復帰、DPI変更、カメラ移動、3軸操作、選択枠と対象の一致を確認。D3D12検証警告と通常ゲームの描画を確認する。

## Task 8: 編集／再生分離と選択反映

**Files:** Create engine/editor/EditorPlaySession.h/.cpp、tests/EditorSessionTests.cpp。Modify application/editor/EditorHost.*、engine/editor/EditorPanels.*、scripts/RunEditorTests.ps1。

**Interfaces:** `EditorPlaySession::Start(const EditorDocument&, std::string& error) -> bool`、`Pause(bool)`、`RecordUserEdit(EditorObjectId, std::string propertyKey, const nlohmann::json& value, std::string& error) -> bool`、`Stop()`、`ApplySelected(EditorDocument&, std::span<const EditorPropertyKey>, EditorHistory&, std::string& error) -> bool`、`DiscardPending()`。PropertyKeyはobjectIdとproperty名の組。Sessionは編集開始時の値、再生値、最後の明示編集値を分けて持つ。Task 2履歴への反映は1操作。

- [ ] `SimulationChangesNeverBecomeCandidates`、`ExplicitValueSurvivesLaterSimulation`、`StopKeepsEditDocumentUntouched`、`ApplySelectedIsOneUndo`、`FailedStartKeepsEditing`、`PendingChangesBlockNextAction`を書く。失敗を確認する。
- [ ] Sessionを実装。再生側の実体はHostが別所有し、生成完了後のみ切り替える。GPU待機後に旧実体を解除する。停止は候補を維持して編集文書の表示へ戻す。
- [ ] 再生／一時停止／停止と候補一覧（対象・設定・元値・変更値）を接続。型の検証を通った明示編集だけ記録し、シミュレーション更新はRecordUserEditを呼ばない。反映／破棄／取消前に再生・読込・終了を進めない。
- [ ] 再生中の構造変更を無効化。一時停止はシミュレーションの更新を止めつつ、表示と設定反映は進める。初期カメラは保存した視点から生成。独自振る舞い用の`EditorHost::SetRuntimeUpdate(std::function<void(const FrameTime&, EditorDocument&)>)`をゲーム側接続に用意し、テストではTransformを毎フレーム変えて分離を証明する。任意の既存Sceneの状態複製は追加しない。
- [ ] Session Suiteと実機の連続再生・停止、生成失敗、フォーカス入力、停止直後の読込を確認。音声を扱う振る舞いはHostに登録した開始・停止処理でセッションに限定し、通常ゲームの音声を無条件で停止しない。

## Task 9: 配布経路・回帰・文書の仕上げ

**Files:** Modify docs/MagEngine/MagEngine_00_索引.md、MagEngine_01_改良方針.md、MagEngine_03_改修仕様.md、MagEngine_04_仕様改修履歴.md、MagEngine_05_制作体験改修プラン.md、MagEngine_07_エディター操作ガイド.md、README.md。必要時Modify msbuild/DeployRuntime.targetsと既存.github/workflows/内の対象workflow（実装時にrgで正確な対象を特定）。

- [ ] 全設計要件を操作ガイドの確認表へ対応付け、実装済み／実機確認済み／未確認を区別する。Cloudは後続であり、本改修の完了条件へ混ぜない。
- [ ] `msbuild MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64`とReleaseを実行し、両構成のRunEditorTests.ps1 -Suite All、VerifyEngineBoundary.ps1を実行する。既存Asset／ParticleのCPU回帰も両構成で実行する。
- [ ] 別フォルダの配布コピーで、Releaseが保存文書から生成・描画できることを確認する。制作側への保存と配布側の読込先を区別し、実行フォルダへ誤保存しないことを確認する。
- [ ] 実機で配置→設定→Undo→保存→再起動→再生調整→停止→選択反映→Undo→再保存を一巡する。既存タイトル／ゲーム／シーン遷移／終了の回帰も確認する。
- [ ] `git diff --check`と差分を確認し、変更内容・実施確認・未確認とリスク・Obsidian未反映を報告する。Hosted CI未実行はローカル確認と分ける。

## 計画の自己確認

文書・履歴・型・保存はTasks 1〜4、起動と寿命はTask 5、画面はTasks 6〜7、編集／再生はTask 8、既存ゲームとReleaseはTask 9へ対応付けた。5つのReview Focusは各担当タスクへ検証を追加済み。ここにあるAPI名は実装で揃えて変更できるが、利用タスクと検証を同時に更新すること。

## 実行状況（2026-11-09）

ユーザーはこのチャットでの順次実装を選択済み。Tasks 1〜8の初期コードとTask 9の文書を実装した。コードとCPUの確認、画面の受け入れを区別するため、未実施の実機手順のチェックは残す。

- 文書・履歴・保存・選択計算・再生分離・入力遮断の6系統がDebug／ReleaseでPASS。入力Suiteは計画の入力漏れ検証を直接行うため追加した。
- 既存Asset／ParticleのCPU検証も両構成でPASS。Debug Asset検証で不足していたDXC DLLを既存スクリプトで配置するよう修正した。
- 通常DebugビルドはC1902。検証時に生成領域のpropsで/Z7を適用しビルドを確認。ReleaseはPDB／LTOの組合せでLNK1103も発生したため、生成領域の別IntDirとLTO無効設定で確認する。製品プロジェクトのデバッグ情報・最適化設定は変えていない。
- 別担当の静的レビューの重要指摘5件を修正。再生中のドラッグを1プロパティ単位へ限定する処理、取消、再生専用UndoをCPUから直接検証した。
- Tasks 4〜9のGPU・画面操作・配布コピー起動・既存ゲームの操作回帰・Hosted CIは未確認。画面操作ツールが起動不能（kernel assets、os error 3）であり、見た目と操作品質の受け入れは完了扱いにしない。
- コミット・ブランチ操作・Obsidian反映は未実施。進捗と一時ビルド設定・ログは生成領域に保持し、未コミット実装の証跡を失わないようにする。
