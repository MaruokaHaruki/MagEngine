# MagEngine コメントルール for Codex

## Point

このエンジンでコメントを書く場合は、既存の `/// @brief` / `/// @details` / `/// @note` 形式を基本にし、処理内容の逐語説明ではなく、理由・前提・制約・所有権・副作用を日本語で補足する。

## Reason

既存コードでは、ヘッダーの公開APIや構造体にDoxygen風コメントが多く、cpp側では関数区切り、処理ブロック、`NOTE` による設計意図の補足が使われている。特にレンダリング、SceneContext、Player系、Cloud系では「なぜその状態を保持するか」「どの依存を誰が持つか」「テスト専用か通常経路か」をコメントで明示する傾向がある。

一方で、単に `// 更新`、`// 移動`、`// 代入` のような処理内容だけのコメントも混在している。新規コードでは既存形式に合わせつつ、保守時に役立つ情報を優先する。

## 解析対象

- `engine/`
- `application/`
- `scene/`
- ルート直下の自作C++ファイル

`externals/` 配下の外部ライブラリは対象外とする。

## 基本ルール

コメントは必ず日本語で書く。ただし、既存API名、DirectX用語、Doxygenタグ、ログ文字列、識別子名は英語のままでよい。

公開API、公開構造体、重要な内部関数には原則コメントを書く。特にヘッダーでは、利用者がcppを読まなくても呼び出し条件や副作用を判断できる粒度にする。

処理の手順そのものはコードで読めるため、コメントでは次を優先する。

- なぜその処理が必要か
- どの前提条件があるか
- 呼び出し順序の制約
- 所有権があるか、非所有参照か
- 毎フレーム実行されるか
- Debug専用、テスト専用、通常経路外か
- GPU/CPU、DirectX、シェーダーとのレイアウト制約
- 値の根拠、調整意図、境界値
- 失敗時や未設定時の扱い

## ファイルヘッダー

既存コードでは、主要なヘッダーに次のようなファイルヘッダーがある。新規の主要クラスではこの形式を推奨する。

```cpp
/*********************************************************************
 * \file   RenderGraph.h
 * \brief  RenderPass間の論理リソース依存を構築・検証するクラス
 *********************************************************************/
```

作者・日付が既存ファイルにある場合は揃えてよいが、不要に追加しなくてよい。既存ファイルへ追記する場合は、そのファイルの形式を優先する。

## ヘッダーの公開APIコメント

公開関数は `/// @brief` を基本にする。呼び出し順序、未設定時の挙動、戻り値の意味が重要な場合は `@details`、`@note`、`@warning`、`@param`、`@return` を追加する。

```cpp
/// @brief 敵マネージャーの設定
/// @param enemyManager ゲーム内すべての敵を管理するマネージャー
/// @details ミサイルの自動追尾とロックオン機能に必須
/// @note Initialize() 直後に必ず呼び出す必要がある
void SetEnemyManager(EnemyManager *enemyManager);
```

短いgetter/setterでも、所有権・nullptr可否・戻り値の意味が曖昧ならコメントする。

```cpp
/// @brief CPUテスト用にassertせず検証結果を返す
RenderGraphValidationResult ValidateForTesting(const std::vector<RenderPassEntry> &passes) const;
```

## 構造体・定数バッファのコメント

GPUへ渡す構造体、定数バッファ、シェーダーと共有するレイアウトには、用途と制約を必ず書く。

```cpp
/**----------------------------------------------------------------------------
 * \brief  BulletHoleBuffer 弾痕配列定数バッファ(GPU用)
 * \note   最大4個の弾痕を管理（高速化優先）
 */
struct alignas(16) BulletHoleBuffer {
	static constexpr int kMaxBulletHoles = 4; // 最大弾痕数（高速化のため削減）
};
```

`alignas(16)`、padding、配列上限、HLSL側との対応、毎フレーム転送の有無は、将来壊れやすいためコメント対象にする。

## cppの関数区切り

既存コードでは関数の前に区切りコメントが多い。新規に大きめの関数を追加する場合は、既存ファイルの形式に合わせる。

```cpp
///=============================================================================
/// 初期化
/// NOTE: EngineサービスはEngineContextから取得し、SceneContextとの重複を避ける
void SceneManager::Initialize(...) {
}
```

小さい関数や既に見通しのよいファイルでは、無理に区切りコメントを増やさない。周囲のスタイルを優先する。

## NOTEコメント

`NOTE` は、処理の説明ではなく設計判断を残すために使う。

```cpp
// NOTE: Sceneの旧Singletonフォールバックを禁止するため、初期化時点で必須依存を検証する。
assert(engineContext != nullptr);
```

適した内容:

- 依存関係をこの場所で解決する理由
- 既存設計との重複を避ける理由
- テスト専用経路である理由
- Debug/Releaseで処理を分ける理由
- GPUステートやBarrierの順序制約

避ける内容:

```cpp
// NOTE: xに1を足す
x += 1;
```

このような処理説明だけの `NOTE` は使わない。

## メンバ変数コメント

ヘッダーのメンバ変数には、意味・所有権・寿命・単位・範囲を必要に応じて書く。

```cpp
MagEngine::Camera *camera_ = nullptr; // CameraManagerが所有するカメラへの非所有参照
float missileRecoveryTime = 1.0f;     // 1発回復に必要な秒数
```

単に変数名を日本語化するだけのコメントは避ける。ただし、既存の一覧形式に合わせる場合は短い分類コメントを許容する。

## 処理ブロックコメント

長い関数内では、責務の切り替わりを示す短いコメントを置いてよい。

```cpp
// 弾痕はシェーダー側のSDF判定に渡すため、CPU側で正規化済み方向にそろえる。
MagMath::Vector3 normalizedDirection = Normalize(direction);
```

次のようなコメントは、コードと同じことを繰り返すだけなので避ける。

```cpp
// 方向ベクトルを正規化
MagMath::Vector3 normalizedDirection = Normalize(direction);
```

## 条件付きコンパイルコメント

`#endif` には対応する条件を書く。既存コードでも使われているため継続する。

```cpp
#if ENABLE_IMGUI
#ifdef _DEBUG
void DrawImGui();
#endif // _DEBUG
#endif // ENABLE_IMGUI
```

Debug専用UIやテスト専用処理には、Releaseに含めない理由を近くに書く。

```cpp
/// COMMENT: DEBUGビルド時のみ実行。UIはReleaseでは不要。
```

## テスト専用コメント

通常経路では使わない関数には、名前だけでなくコメントでも明示する。

```cpp
/// @brief 依存循環テスト専用。通常のGraph構築経路では使用しない。
void AddDependencyForTesting(RenderPassId before, RenderPassId after, RenderResourceId resource);
```

テスト専用APIは、本番処理から呼ばない前提をコメントに含める。

## パフォーマンス関連コメント

毎フレーム処理、GPU転送、描画順、アロケーション回避、配列上限の根拠はコメント対象にする。

```cpp
// 毎フレームGPUへ転送するため、弾痕数は固定長にしてアロケーションを避ける。
std::array<BulletHoleGPU, kMaxBulletHoles> bulletHoles_;
```

数値調整には、単なる「品質重視」だけでなく、可能なら何を犠牲にして何を優先したかを書く。

```cpp
float stepSize = 3.0f; // レイマーチ回数を抑えるための値。雲の輪郭品質よりFPSを優先。
```

## 所有権コメント

ポインタや参照をメンバに保持する場合は、所有者を明記する。

```cpp
SceneContext *sceneContext_ = nullptr; // SceneManagerが所有し、Sceneは非所有参照として扱う。
```

`unique_ptr` や `vector` など所有が明確な場合でも、寿命や破棄順が重要なら補足する。

## クラス内の並びと責務境界

クラスを整理する場合は、既存の区切り線形式を保ったまま、公開APIを次の責務順に並べる。

1. 初期化・終了・フレーム更新
2. 主要操作（生成、判定、状態遷移など）
3. 描画登録または外部システムとの連携
4. 状態取得
5. 設定変更
6. privateの内部処理と状態

すべてのクラスにこの順序を強制するのではなく、既存の利用順やファイル内の慣習と衝突しない場合だけ適用する。各区分には「何をするか」よりも、更新順・責務の分離・呼び出し側の制約が分かるコメントを置く。

コンポーネントでは、入力収集、状態更新、Transform反映、描画登録を混在させない。たとえば入力値を受け取る関数と、その結果をTransformへ反映する関数が分かれている場合は、呼び出し順を `@note` で明記する。

```cpp
/// @brief 入力から目標速度を更新する
/// @note 同一フレームのUpdate()前に呼び出す。Transformへの反映は行わない。
void ProcessInput(float inputX, float inputY);
```

## インライン関数と設定値の前提

ヘッダー内のインラインgetterや比率計算は、実装を読まなくてもゼロ除算・単位・有効範囲を判断できるようにする。初期化済みであることや正の設定値を前提にする場合は、その前提を `@note` または `// NOTE:` として残す。

```cpp
float GetGaugeRatio() const {
	// NOTE: Initialize()でmaxGauge_を正値に設定済みであることを前提とする。
	return gauge_ / maxGauge_;
}
```

設定関数が値をクランプしない場合は、呼び出し側が守る範囲をコメントで明示する。コメントだけで暗黙に補正されるように見せない。

## Debug専用クラスの境界

`_DEBUG` 専用のクラスやAPIは、宣言と実装を同じ条件付きコンパイルで囲み、Releaseに存在しない理由をクラスコメントへ書く。外部サービスを保持する場合は非所有参照であることと、Scene終了前に解除する必要があるかを明記する。

```cpp
/// @brief Scene専用のデバッグ入力を管理する
/// @details リリースビルドへ操作を持ち込まないため、_DEBUG時だけに存在する。
///          所有権は持たず、Scene終了前にFinalize()で依存を解除する。
class SceneDebugController {
};
```

## エラー・境界値コメント

assert、nullptrチェック、範囲クランプ、フォールバック値には、なぜその扱いにするかを書く。

```cpp
// 初期化漏れは描画順の破綻に直結するため、起動時に検出して以降の処理を止める。
assert(renderWorld != nullptr);
```

フォールバック値を置く場合は、暫定値か仕様値かを明確にする。

## 推奨タグ

- `/// @brief`: 公開APIの概要
- `/// @param`: 引数の意味、所有権、nullptr可否
- `/// @return`: 戻り値の意味、nullptrや空配列の扱い
- `/// @details`: 呼び出し側が知るべき詳細
- `/// @note`: 前提、理由、通常と異なる設計判断
- `/// @warning`: 誤用時のリスク、順序制約
- `// NOTE:`: cpp内の局所的な設計理由

## 非推奨

以下は新規コメントでは避ける。

- コードを読めば分かる処理説明だけのコメント
- 古い仕様のまま残ったコメント
- コメントアウトされた不要コード
- 英語と日本語が理由なく混在するコメント
- `TODO` だけで期限・理由・暫定対応が分からないコメント
- 実装と矛盾する `@note`、`@warning`
- 外部ライブラリ風の過剰な装飾を自作コードへ無理に増やすこと

## TODOコメント

`TODO` を残す場合は、最低限「何を」「なぜ後回しにしたか」「完了条件」を書く。

```cpp
// TODO: RenderGraph側で自動Barrier化した後に削除する。現状は既存手動Barrierとの互換確認が未完了。
```

単独の `// TODO` や `// 後で直す` は不可。

## Codexが編集する時の優先順位

1. 周囲のファイルのコメント形式を最優先する。
2. ヘッダーの公開APIにはDoxygen風コメントを付ける。
3. cppでは「なぜ」「前提」「制約」を短く書く。
4. 処理説明だけのコメントは増やさない。
5. ポインタ、GPUリソース、毎フレーム処理、Debug専用処理には制約を書く。
6. コメントが古くなった場合は、コード変更と同時に更新する。
7. 不明点はコメントで断定せず、調査できない場合は実装側で保守的に扱う。

## Example

良い例:

```cpp
/// @brief RenderWorldへ描画対象として登録
/// @param renderWorld フレーム内の描画対象を集約する非所有参照
/// @note Update() 後の最新Transformを前提にするため、Sceneの描画登録フェーズで呼び出す。
void RegisterRenderables(MagEngine::RenderWorld &renderWorld);
```

良い例:

```cpp
// PresentColorはSwapChain由来の外部リソースなので、Graph内では初期状態だけを検証対象にする。
renderGraph.SetInitialResourceState(RenderResourceId::PresentColor, RenderResourceState::Present);
```

悪い例:

```cpp
// renderWorldに登録する
RegisterRenderables(renderWorld);
```

悪い例:

```cpp
// 変数を初期化
float timer = 0.0f;
```

## Point

このエンジンのコメントは、既存のDoxygen風形式と日本語コメントを尊重しながら、保守に必要な「理由・前提・制約」を残す方針に統一する。Codexが新規実装や修正を行う場合も、コメント量を増やすこと自体を目的にせず、将来の誤用や仕様破壊を防ぐ情報を優先して書く。
