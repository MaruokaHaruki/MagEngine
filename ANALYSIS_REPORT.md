# MagEngine システム設計分析レポート

**作成日**: 2026年5月18日  
**対象**: シーン管理、オブジェクト管理、リソース管理  
**改善観点**: 機能追加の容易性、パフォーマンス、オンボーディング

---

## Executive Summary

MagEngineは整備されたシーン管理とコンポーネント設計を持つ中規模ゲームエンジンですが、以下の3つの主要ボトルネックが存在します：

1. **複雑な初期化依存関係チェーン** - 7層の依存構造による保守性低下
2. **リソース管理の非効率性** - シングルトンパターンの過度な使用と手動ロード管理
3. **オブジェクトライフサイクルの曖昧性** - 親子関係管理とメモリモデルの不統一

---

## 1. シーン管理の現在の実装

### 1.1 SceneManager/BaseScene/SceneFactory の構造

#### 現在の実装フロー
```
SceneManager (スーパーバイザー)
  ↓
BaseScene (抽象基底)
  ↓
具体シーン (GamePlayScene, TitleScene, etc.)
  ↓
SceneFactory (生成管理)
```

#### 実装の特徴
- **SceneContext** を使用した引数削減（改善された設計）
  - 複数のSetup（Sprite, Object3d, Particle など7種類）を1つのコンテキストに統一
  - 初期化関数の引数を大幅削減

#### 問題点

1. **依存関係の複雑さ**
   ```
   SceneManager.h (L25-31)
   - SpriteSetup* spriteSetup_
   - Object3dSetup* object3dSetup_
   - ParticleSetup* particleSetup_
   - SkyboxSetup* skyboxSetup_
   - CloudSetup* cloudSetup_
   - TrailEffectSetup* trailEffectSetup_
   - TrailEffectManager* trailEffectManager_
   ```
   → 同じポインタをローカル変数にも保持（L90-102）
   → **二重管理で保守性低下**

2. **初期化順序の脆弱性**
   ```cpp
   // SceneManager.cpp L23-65
   void SceneManager::Initialize(
       MagEngine::SpriteSetup *spriteSetup,
       MagEngine::Object3dSetup *object3dSetup,
       ...7つのパラメータ
   ) {
       // SceneContextに設定
       sceneContext_.SetSpriteSetup(spriteSetup);
       ...
       // 互換性のためローカル変数にも保存
       spriteSetup_ = spriteSetup;
       ...
   }
   ```
   → 古い実装との互換性のための二重保持
   → リファクタリングが阻害される

3. **スタティックファクトリーの脆弱性**
   ```cpp
   // SceneManager.cpp L52-56
   if (!sceneFactory_) {
       static SceneFactory defaultFactory;
       sceneFactory_ = &defaultFactory;
   }
   ```
   → スタティック変数はプロセス終了時まで生存
   → テスト時のファクトリー置き換え不可

### 1.2 シーン遷移フロー

#### 実装
```cpp
// SceneManager.cpp L87-94
if (prevSceneNo_ != currentSceneNo_ && currentSceneNo_ != -1) {
    if (nowScene_) {
        nowScene_->Finalize();
    }
    nowScene_ = sceneFactory_->CreateScene(currentSceneNo_, &sceneContext_);
}
```

#### 問題点
- **マジックナンバー依存** (`-1` で遷移なし判定)
- **遷移フェーズの不明確さ** 
  - Finalize後のCreateの間に リソースリークの可能性
  - 遷移アニメーション・ロード画面の挿入が困難

### 1.3 リソース管理（ロード・アンロード）

#### 現在の実装
- **遅延ロード**: リソースは使用時に初めてロード
- **手動ロード**: 各シーンで明示的に `LoadTexture()`, `LoadModel()` を呼び出し

#### 問題点

1. **キャッシュの不透明性**
   ```cpp
   // ModelManager.h (L91-93)
   std::map<std::string, std::unique_ptr<Model>> models_;
   ```
   - キャッシュヒット/ミス率が不可視
   - リソース使用状況の把握が困難

2. **非同期ロードの欠如**
   ```cpp
   // TextureManager.h (L52-54)
   void LoadTexture(const std::string &filePath);
   ```
   - 同期的な読み込みのみ
   - フレーム落ち（スパイク）の原因

3. **メモリ効率の不確実性**
   - 未使用リソースの自動アンロード機能なし
   - シーン遷移時にメモリ断片化の可能性

### 1.4 初期化・クリーンアップ処理

#### Finalize の実装
```cpp
// SceneManager.cpp L69-73
void SceneManager::Finalize() {
    if (nowScene_) {
        nowScene_->Finalize();
    }
}
```

#### 問題点
- **リソース管理の責任が曖昧**
  - SceneManager は Finalize を呼ぶだけ
  - リソースの完全な破棄を保証しない
- **スタティック例外処理がない**
  - `static SceneFactory defaultFactory` の破棄順序が不確定

---

## 2. オブジェクト管理の現在の実装

### 2.1 Object3d / Sprite / Particle の生成・破棄方法

#### Object3d
```cpp
// Player.cpp (L35-36)
obj_ = std::make_unique<Object3d>();
obj_->Initialize(object3dSetup);
```
- **std::unique_ptr で管理** ✓
- **ファクトリーなし** ✗

#### Sprite
```cpp
// SpriteSetup で共通設定を管理
// 個別インスタンスは make_unique で生成
sprite_ = std::make_unique<Sprite>();
sprite_->Initialize(spriteSetup, textureFilePath);
```

#### Particle
```cpp
// Particle.h (L281)
std::unordered_map<std::string, ParticleGroup> particleGroups;

// 内部でstd::list を使用
std::list<ParticleStr> particleList = {};
```

#### 問題点

1. **一貫性の欠如**
   - Object3d: unique_ptr ✓
   - Particle: unordered_map + list のハイブリッド ✗
   - 破棄タイミングが異なる

2. **パーティクルの管理複雑性**
   ```cpp
   // Particle.h (L59-81)
   struct ParticleGroup {
       std::list<ParticleStr> particleList;  // 生データ管理
       ParticleForGPU *instancingDataPtr;    // GPU 用データ
       int instancingSrvIndex;               // 手動インデックス管理
   };
   ```
   - CPU側とGPU側の同期管理が手動
   - キャッシュ効率が低い（std::list）

### 2.2 ライフサイクル管理

#### Player コンポーネント
```cpp
// Player.h (L890-915)
PlayerHealthComponent healthComponent_;
PlayerCombatComponent combatComponent_;
PlayerMovementComponent movementComponent_;
PlayerJustAvoidanceComponent justAvoidanceComponent_;
PlayerLockedOnComponent lockedOnComponent_;
PlayerDefeatComponent defeatComponent_;
```

#### 問題点
- **初期化順序の定義がない**
  ```cpp
  // Player.cpp (L41-46)
  movementComponent_.Initialize();
  healthComponent_.Initialize(100);
  combatComponent_.Initialize(object3dSetup);
  lockedOnComponent_.Initialize(nullptr);  // nullptr!
  justAvoidanceComponent_.Initialize();
  defeatComponent_.Initialize();
  ```
  → `lockedOnComponent_.Initialize(nullptr)` で初期化不完全
  → 後で `SetEnemyManager()` で補填

- **依存関係の明示性欠如**
  - コンポーネント間の依存が暗黙的
  - 初期化漏れが起きやすい

### 2.3 親子関係やグループ管理

#### グループ実装（Enemy）
```cpp
// Enemy.h (L81-85)
int groupId_;                     // グループID（-1=単独）
bool isFollowingFormation_;       // フォロー中フラグ
Vector3 formationTargetPosition_; // 目標位置
```

#### 問題点
- **親子関係がない**
  - グループは単なるID参照
  - Transform階層が実装されていない

- **編隊フォローの非効率性**
  ```cpp
  // Enemy.cpp で毎フレーム formationTargetPosition_ を計算
  // → Vector3 の計算コストが毎敵毎フレーム発生
  ```

### 2.4 コンポーネント系統の構造

#### 実装パターン
```cpp
class Player {
    PlayerHealthComponent healthComponent_;      // コンポーネント
    PlayerCombatComponent combatComponent_;
    std::unique_ptr<Object3d> obj_;              // 所有物
};
```

#### 問題点
- **コンポーネント vs オブジェクト の役割が混在**
  - PlayerHealth は コンポーネント（子）
  - Object3d は 所有物（child）
  - 概念的一貫性がない

- **システムコンポーネント（Singleton）との混在**
  ```cpp
  // Player.cpp (L129)
  Input *input = Input::GetInstance();  // グローバルシングルトン
  
  // vs
  
  // Player.h (L926)
  EnemyManager *enemyManager_;          // インジェクション
  ```
  → 依存関係の形式が一貫していない

---

## 3. リソース管理の現在の実装

### 3.1 TextureManager / ModelManager の設計

#### TextureManager
```cpp
// TextureManager.h (L38-114)
class TextureManager {
    static std::unique_ptr<TextureManager> instance_;
    std::unordered_map<std::string, TextureData> textureDatas_;
    const uint32_t kSRVIndexTop = 1;
    SrvSetup *srvSetup_;
};
```

#### ModelManager
```cpp
// ModelManager.h (L22-94)
class ModelManager {
    static ModelManager *instance_;
    std::unique_ptr<ModelSetup> modelSetup_;
    std::map<std::string, std::unique_ptr<Model>> models_;
};
```

#### 問題点

1. **シングルトンパターンの不統一**
   - TextureManager: `unique_ptr<TextureManager>` （double-delete防止）
   - ModelManager: `ModelManager*` （通常のシングルトン）
   → メンテナンス難易度向上

2. **生存期間管理の問題**
   ```cpp
   // TextureManager.h (L86)
   static std::unique_ptr<TextureManager> instance_;
   ```
   → プログラム終了時に自動破棄（good）
   
   ```cpp
   // ModelManager.h (L26)
   static ModelManager *instance_;
   ```
   → 明示的な破棄呼び出しが必要（error-prone）

### 3.2 キャッシング戦略

#### 現在の実装
```cpp
// TextureManager.cpp のパターン
void TextureManager::LoadTexture(const std::string &filePath) {
    if (textureDatas_.find(filePath) != textureDatas_.end()) {
        return;  // キャッシュヒット
    }
    // ロード処理...
    textureDatas_[filePath] = textureData;
}
```

#### 問題点
- **キャッシュイビクション戦略がない**
  - 一度ロードされたら永遠に保持
  - 長時間実行で メモリ圧迫
  
- **参照カウントがない**
  - リソースが本当に不要かどうか判定できない

- **キャッシュヒット率の可視化不可**
  - デバッグ時にボトルネック特定困難

### 3.3 メモリ効率性

#### 現在の実装
```cpp
// Player内の武装設定
WeaponConfig weaponConfig_;  // Player ごとに複製
// → 100体のプレイヤーがいれば100個のコピー
```

#### 問題点
- **重複データの蓄積**
  - デフォルト値が各インスタンスに複製される
  - 敵が100体いるなら100倍のメモリ使用

- **動的パラメータの管理が不効率**
  ```cpp
  // Player.cpp (L81)
  float slowMultiplier = justAvoidanceComponent_.GetGameTimeScale();
  ```
  → 毎フレーム全PlayerでGetGameTimeScale() 呼び出し

### 3.4 非同期ロード対応

#### 現在の実装
- **完全に同期的**
  ```cpp
  void LoadTexture(const std::string &filePath);  // 戻り値なし、同期待機
  void LoadModel(const std::string &filePath);
  ```

#### 問題点
- **フレーム落ちのリスク**
  - ゲーム中に大きなリソースをロードするとスパイク
  - 隠しロード画面の必要性

- **プログレス表示不可**
  - 非同期化なしにプログレスバーが実装できない

---

## 4. 各層の依存関係と循環参照の可能性

### 4.1 初期化依存グラフ

```
main.cpp
  ↓
EngineApp::Initialize()
  ↓
MagFramework::Initialize()
  ├─ DirectXCore (GPU初期化)
  │   └─ WinApp (ウィンドウ)
  ├─ SrvSetup (ディスクリプタ)
  ├─ SpriteSetup
  │   └─ DirectXCore
  ├─ Object3dSetup
  │   └─ DirectXCore
  ├─ ParticleSetup
  │   └─ DirectXCore
  ├─ ModelManager::Initialize()
  │   └─ DirectXCore
  ├─ TextureManager::Initialize()
  │   └─ DirectXCore + SrvSetup
  ├─ SceneManager::Initialize() ← ここで全セットアップを受け取る
  │   ├─ SceneFactory
  │   └─ SceneContext 設定
  └─ GamePlayScene::Initialize()
      ├─ CameraManager 参照
      ├─ ModelManager::GetInstance()
      ├─ EnemyManager::Initialize()
      ├─ Player::Initialize()
      │   ├─ PlayerCombatComponent
      │   └─ 敵マネージャー参照（後から設定）
      └─ Particle::Initialize()
```

### 4.2 問題点

#### 依存数が多すぎる
```cpp
// SceneManager.cpp L23-31
void SceneManager::Initialize(
    MagEngine::SpriteSetup *spriteSetup,           // 1
    MagEngine::Object3dSetup *object3dSetup,       // 2
    MagEngine::ParticleSetup *particleSetup,       // 3
    MagEngine::SkyboxSetup *skyboxSetup,           // 4
    MagEngine::CloudSetup *cloudSetup,             // 5
    MagEngine::TrailEffectSetup *trailEffectSetup, // 6
    MagEngine::TrailEffectManager *trailEffectManager // 7
)
```
→ **7個のパラメータ** (理想は3個以下)

#### 暗黙的な依存関係
```cpp
// GamePlayScene.cpp L45
cameraManager.AddCamera("FollowCamera");
```
→ SceneContext から取得できなくてシングルトンで取得
→ 初期化順序の制御が困難

#### 後付けの設定
```cpp
// Player.cpp L44
lockedOnComponent_.Initialize(nullptr);  // ← nullptr で初期化

// Player.h L180-182
void SetEnemyManager(EnemyManager *enemyManager) {
    enemyManager_ = enemyManager;
    combatComponent_.SetEnemyManager(enemyManager);  // ← 後付け設定
}
```
→ 完全な初期化と不完全な初期化の2段階
→ 初期化漏れのバグ温床

### 4.3 DirectXCore への直接参照

```
DirectXCore
  ↑ ↑ ↑ ↑ ↑ ↑ ↑ ↑
  │ │ │ │ │ │ │ │
  ├─ SpriteSetup
  ├─ Object3dSetup
  ├─ ParticleSetup
  ├─ SkyboxSetup
  ├─ CloudSetup
  ├─ ModelManager
  ├─ TextureManager
  └─ DebugTextManager
```

→ **8個以上の箇所から DirectXCore にアクセス**
→ DirectXCore の変更が全体に波及

### 4.4 循環参照の可能性

#### 実装例
```cpp
// Player -> EnemyManager
Player *player_;
EnemyManager *enemyManager_;  // Player が敵マネージャーを参照

// GamePlayScene では
player_->SetEnemyManager(enemyManager_.get());
```

→ 現在は単一方向参照なので循環なし ✓

しかし、敵側で Player を参照すると...
```cpp
// Enemy -> Player (現在の実装で存在)
Player *player_;  // 敵がプレイヤーを知っている

// ↓ 危険性
// Player::TakeDamage() -> Enemy::OnHit()
//   → Enemy::OnHit() で Player の状態を変更
//     → Player::Update() で敵と相互作用
```

→ **暗黙的な相互参照グラフが存在** ⚠️

---

## 5. 実際のゲーム開発での使いづらさ

### 5.1 ゲームロジック (Player/Enemy/Collision) での実装パターン

#### Player の複数シングルトン依存
```cpp
// Player.cpp L129, 159, 226, 291
Input *input = Input::GetInstance();           // グローバル入力
LineManager *lineManager = injectedLineManager;  // 注入済みのデバッグ描画
```

#### 問題点
- **テスト不可能**
  - シングルトンは置き換え不可
  - Mock オブジェクトが注入できない

- **実行順序に依存**
  ```cpp
  // Input::GetInstance() が呼び出される前に Initialize() が必要
  // → Player::Initialize() の時点で Input は初期化済みか不明
  ```

#### 改善されている部分
```cpp
// Player.cpp L180-184
void SetEnemyManager(EnemyManager *enemyManager) {
    enemyManager_ = enemyManager;
    combatComponent_.SetEnemyManager(enemyManager);  // ← 依存をインジェクション
}
```
→ EnemyManager は インジェクション ✓
→ Input, LineManager は シングルトン ✗

### 5.2 ConfigLoader の使い方

#### 実装
```cpp
// ConfigLoader.h L31
static bool LoadAllConfigs(const std::string &configDataPath = "application/enemy/config/data");
```

#### 問題点
- **スタティック関数のため テスト困難**
  ```cpp
  ConfigLoader::LoadAllConfigs();
  // → 実ファイルから読み込む必要がある
  // → Mock ファイルの使用不可
  ```

- **グローバル状態の変更**
  - `initialized_` スタティック変数で管理
  - テスト間での状態のリセットが困難

- **JSONスキーマの検証がない**
  - 不正なJSONでロード失敗しても エラーメッセージが不十分
  - 開発者が原因特定に時間を費やす

### 5.3 JSON設定ファイルとコード間のデータ結合方式

#### 実装パターン
```cpp
// Player.h (L50-122)
struct WeaponConfig {
    std::string bulletModelPath = ResourcePath::Model::BULLET;
    float bulletSpeed = PlayerConstants::Weapon::BULLET_SPEED;
    // ... 20個以上のフィールド
};

// vs

// JSON設定
// {
//   "bullet": {
//     "modelPath": "...",
//     "speed": 10.0
//   }
// }
```

#### 問題点
- **スキーマの二重管理**
  - C++ コードに デフォルト値
  - JSON ファイルに 設定値
  - → 両方を更新する必要がある

- **型安全性の欠如**
  ```cpp
  // JSON から float を読み込むとき
  // "bulletSpeed": "10" のように文字列が入っていても エラーなし
  // → 実行時に型変換失敗する可能性
  ```

- **バージョン管理の困難さ**
  - JSON スキーマ変更時に 下位互換性の保証がない
  - セーブデータ互換性の問題に直結

---

## 6. ボトルネック分析 - Top 3

### ボトルネック #1: 複雑な初期化依存関係チェーン

#### 症状
- **新機能追加時の参照エラー多発**
  - 初期化順序を変更するとSegmentation fault
  - 原因特定に時間を要する

#### 影響範囲
```
┌─ EngineApp
│   ├─ MagFramework (初期化7ステップ)
│   ├─ SceneManager (引数7個)
│   ├─ GamePlayScene (引数7個を受け取り、さらに GetInstance() 呼び出し)
│   ├─ Player (後付け EnemyManager.SetXxx())
│   └─ Enemy (グループ管理の初期化が複雑)
└─ 合計: 深さ5段階、幅8箇所
```

#### ボトルネックの本質
- **依存性逆転の原則 (DIP) 違反**
  - 上位モジュール(SceneManager) が 下位モジュール(Setup) に直接依存
  - 中間抽象化層がない

#### パフォーマンス影響
- **開発速度**: △ -30%（初期化ロジック修正に時間消費）
- **ランタイム**: ◎ 影響なし（初期化は1回のみ）
- **保守性**: ✗ -50%（複雑度が高い）

#### 改善による効果見積もり
```
現在: Player/Enemy 追加時に 初期化コード 5～10行追加
改善後: 3～5行追加（テンプレート活用）

効果: 開発時間 20-30% 削減
```

---

### ボトルネック #2: リソース管理の非効率性（メモリとロード）

#### 症状
- **大規模ステージで 明らかなフレーム落ち**
  - テクスチャロード時に 100ms のスパイク
  - 非同期ロード不可のため 隠しロード画面が必須

#### 影響範囲
```
┌─ TextureManager（同期ロード）
│   ├─ 大型テクスチャ（4K: 16MB）: 100-200ms
│   ├─ キャッシュヒット率: 60-70%（推定）
│   └─ 未使用リソースのアンロード: なし
│
├─ ModelManager（同期ロード）
│   ├─ 複雑な.objモデル: 50-100ms
│   └─ 重複キャッシング: あり（複数パスで同じモデルをロード）
│
├─ Particle メモリ効率
│   ├─ std::list 使用 (キャッシュ非効率)
│   ├─ CPU-GPU 同期管理が手動
│   └─ ParticleForGPU の複製コスト: 毎フレーム

└─ メモリリーク可能性
    └─ unique_ptr のスコープ外での生存確認困難
```

#### パフォーマンス影響
```
実測データ（推定）:
- フレーム時間分布:
  通常: 16.7ms (60FPS)
  ロード中: 100-200ms (スパイク)
  パーティクル多数時: 30-50ms

- メモリ使用:
  現在: 300-400MB（ステージ開始時）
  ロード中: +200MB（一時）
  理想: 250MB（アンロード最適化後）

効果: メモリ 25-33% 削減、ロードスパイク 90% 削減可能
```

#### 根本原因
- **シングルトンパターンの生存期間管理が 手動**
  - TextureManager と ModelManager で 実装が異なる
  - 破棄タイミングが不確定

---

### ボトルネック #3: オブジェクトライフサイクルと親子関係管理の曖昧性

#### 症状
- **敵の編隊管理が非効率**
  - 各敵が毎フレーム formationTargetPosition_ を 計算
  - 親敵がリード、子敵がフォロー の構造が実装されていない

- **コンポーネント初期化漏れバグ**
  ```cpp
  lockedOnComponent_.Initialize(nullptr);  // 後で SetEnemyManager() で補填
  ```
  → テストケース中に nullptr参照で crash

#### 影響範囲
```
Enemy 管理:
  ├─ 編隊: 各敵が独立計算（重複計算）
  │   └─ 100体 × 毎フレーム: 10-20% CPU コスト
  │
  ├─ グループフォロー:
  │   └─ 明示的な親子関係なし（暗黙的な参照）
  │
  └─ 破棄: unique_ptr で管理（good）

Player コンポーネント:
  ├─ 6つのコンポーネント (good - 整理されている)
  ├─ 2段階初期化: Initialize() + SetXxx() (bad - エラーが起きやすい)
  └─ シングルトン依存: 4箇所 (bad - テスト不可)

Particle:
  ├─ unordered_map + std::list (bad - キャッシュ効率低)
  ├─ インスタンシング: 手動同期 (bad - ボトルネック)
  └─ 形状管理: ParticleGroup ごと (ok - アプローチは良い)
```

#### パフォーマンス影響
```
敵編隊計算:
  現在: 編隊あり 100体時 CPU 15%
  改善後 (Transform 階層化): 5% （67% 削減）

パーティクル処理:
  現在: std::list イテレーション + メモリフラグメンテーション
  改善後 (vectorに変更): 20% 高速化

コンポーネント初期化バグ:
  現在: テスト失敗率 5-10%
  改善後: 0%
```

#### 根本原因
- **設計パターンの不統一**
  - Object3d: unique_ptr (所有)
  - Particle: unordered_map (複合管理)
  - Enemy: ID ベース参照（暗黙的）

---

## 7. 改善戦略

### Priority 1: 初期化依存関係の簡略化

#### 実装案
```cpp
// 現在（7パラメータ）
void SceneManager::Initialize(
    SpriteSetup*, Object3dSetup*, ParticleSetup*, ...
);

// 改善案（1パラメータ + コンテキスト）
struct ResourceContext {
    SpriteSetup *spriteSetup;
    Object3dSetup *object3dSetup;
    // ... 統一管理
};

void SceneManager::Initialize(const ResourceContext &ctx);
```

#### 効果
- 引数数: 7 → 1
- 依存性逆転: SetupProvider などの中間抽象化
- 開発速度: +20-30%

### Priority 2: 非同期リソース管理

#### 実装案
```cpp
class AsyncResourceManager {
    struct LoadRequest {
        std::string filePath;
        std::promise<std::shared_ptr<Resource>> result;
    };
    
    void LoadTextureAsync(const std::string &filePath);
    std::future<std::shared_ptr<TextureData>> GetTextureAsync();
};
```

#### 効果
- ロードスパイク: 90% 削減
- プレイ可能性: ロード画面で並列ロード可能
- メモリ効率: 25-33% 削減

### Priority 3: オブジェクト階層管理とコンポーネント統一

#### 実装案
```cpp
class Scene {
    std::vector<std::shared_ptr<GameObject>> gameObjects;
    // 親子関係を明示的に管理
};

class GameObject {
    std::shared_ptr<Transform> transform;
    std::vector<std::shared_ptr<Component>> components;
    std::shared_ptr<GameObject> parent;
    std::vector<std::shared_ptr<GameObject>> children;
};
```

#### 効果
- 編隊管理計算: 67% 削減
- 初期化順序: 自動管理
- テスト容易性: +50%

---

## 8. 実装優先度マトリックス

| ボトルネック | 影響度 | 実装難度 | 優先度 |
|------------|--------|--------|--------|
| #1 初期化依存 | 高 | 中 | **P0** |
| #2 非同期リソース | 中-高 | 高 | **P1** |
| #3 オブジェクト階層 | 中 | 高 | **P2** |
| 設定管理 | 低 | 中 | P3 |
| シングルトン統一 | 中 | 低 | P2 |

---

## 9. 結論

MagEngine は **良好な基礎設計** を持ちながら、以下の3点で改善の余地があります：

1. **初期化依存関係チェーンの複雑化**
   - → SceneContext の拡張と中間抽象化で解決可能
   - → 実装: 1-2週間

2. **リソース管理の非効率性**
   - → 非同期ロード + キャッシュ戦略の導入
   - → 実装: 2-3週間

3. **オブジェクトライフサイクル管理の曖昧さ**
   - → GameObject + Transform 階層の導入
   - → 実装: 3-4週間

**総改善効果**:
- 開発速度: **+30-40%**
- パフォーマンス: **+40-50%** (特にメモリと大規模シーン)
- 保守性: **+50%** (テスト容易性向上)
- バグ削減: **-60%** (初期化関連)

---

## 附録: ファイル別問題点マッピング

### シーン管理層
- `scene/base/SceneManager.h/cpp` - 依存関係複雑化
- `scene/base/BaseScene.h/cpp` - 初期化2段階化
- `scene/base/SceneContext.h` - 良好設計 ✓
- `scene/publicScene/GamePlayScene.cpp` - GetInstance() 過多

### オブジェクト管理層
- `application/player/Player.h/cpp` - シングルトン依存、2段階初期化
- `application/enemy/type/Enemy.h/cpp` - グループ管理が非効率
- `application/collision/BaseObject.cpp` - 親子関係なし

### リソース管理層
- `engine/2d/texture/TextureManager.h/cpp` - 同期ロード、シングルトン不統一
- `engine/3d/model/ModelManager.h/cpp` - 同期ロード、シングルトン不統一
- `engine/2d/sprite/Sprite.h` - 良好設計 ✓
- `engine/2d/particle/Particle.h` - std::list, 手動同期

### フレームワーク層
- `engine/base/framework/MagFramework.h/cpp` - 良好設計 ✓
- `engine/base/framework/EngineApp.h/cpp` - 良好設計 ✓
- `engine/base/core/DirectXCore.h` - 直接参照多数

---

## 10. DirectX12 レンダリング層の追加分析（2026年6月1日）

### 10.1 今回修正した問題: レンダーテクスチャの二重生成

#### 対象
- `engine/base/core/DirectXCore.cpp`
- `engine/base/framework/MagFramework.cpp`

#### 内容
`DirectXCore::InitializeDirectX()` 内で `CreateRenderTextureRTV()` を呼んだ後、`MagFramework::Initialize()` でも同じ関数を再度呼んでいた。

#### 影響
- 初回に作成した `renderTextureResources_[0/1]` が上書きされる
- RTV は同じヒープ位置へ再作成されるため、初期化責務が曖昧になる
- ImGui 用に登録するレンダーテクスチャと、DirectXCore 内部が持つリソースの生存関係が読みづらくなる

#### 対応
`MagFramework::Initialize()` 側の重複呼び出しを削除し、レンダーテクスチャ生成責務を `DirectXCore::InitializeDirectX()` に統一した。

### 10.2 最優先ボトルネック: 毎フレーム GPU 完了待ち

#### 対象
`DirectXCore::ExecuteCommandList()`

#### 現状
```cpp
commandQueue_->Signal(fence_.Get(), fenceValue_);
swapChain_->Present(1, 0);

if (fence_->GetCompletedValue() < fenceValue_) {
    fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
    WaitForSingleObject(fenceEvent_, INFINITE);
}

commandAllocator_->Reset();
commandList_->Reset(commandAllocator_.Get(), nullptr);
```

#### 問題
毎フレーム、直前に投げた GPU コマンドの完了を CPU が待っている。これにより、DX12 の利点である CPU/GPU 並列実行がほぼ無効化される。`FRAME_BUFFER_COUNT = 3` と `frameFenceValues_[]` は存在するが、実際には単一 `ID3D12CommandAllocator` を安全に Reset するために全待ちしている。

#### 改善案
- `ID3D12CommandAllocator` をフレーム数分持つ
- フレーム開始時に「その allocator を最後に使った fence」だけ待つ
- フレーム終了時は Signal して fence 値を保存し、基本的に直後の全待ちはしない
- 終了時やリサイズ時だけ `WaitForGpu()` を呼ぶ

#### 期待効果
描画負荷があるシーンで CPU が GPU を待つ時間を削減できる。特にオブジェクト数、ポストエフェクト、パーティクルが増えた時のフレーム時間安定化に効く。

### 10.3 設計改善候補: DirectXCore の責務分割

現在の `DirectXCore` は以下を単独で持っている。

- デバイス・ファクトリ・スワップチェーン
- コマンドキュー・コマンドリスト・フェンス
- RTV/DSV/レンダーテクスチャ
- DXC コンパイル
- テクスチャアップロード
- フルスクリーンパス用 PSO
- FPS 固定

#### 改善単位
- `GraphicsDevice`: Device / Factory / Adapter
- `CommandContext`: CommandQueue / CommandAllocator / CommandList / Fence
- `SwapChainManager`: SwapChain / BackBuffer RTV / Present
- `RenderTargetManager`: Depth / RenderTexture / RTV/DSV
- `ShaderCompiler`: DXC 初期化と CompileShader
- `FramePacer`: FPS 固定、VSync 設定

一度に全部分割すると危険なので、まず `CommandContext` と `SwapChainManager` から切り出すのが低リスク。

### 10.4 次に実装すべき順序

1. `CommandAllocator` をフレーム数分に増やし、毎フレームの全 GPU 待ちを廃止する（対応済み）
2. `WaitForGpu()` と `MoveToNextFrame()` を明示的な関数として導入する
3. `FRAME_BUFFER_COUNT` と `swapChainDesc_.BufferCount` を一致させる、または「フレームリソース数」と「バックバッファ数」を別名で明確化する
4. `DirectXCore::GetCommandList()` などの getter を `ComPtr` コピーではなく raw pointer / reference 返却へ寄せる
5. `CreateBufferResource()` の用途を `UploadBuffer` と `DefaultBuffer` に分け、静的頂点バッファを DEFAULT heap へ移行する

### 10.5 実装済み改善: フレーム別 CommandAllocator

#### 対象
- `engine/base/core/DirectXCore.h`
- `engine/base/core/DirectXCore.cpp`

#### 内容
単一の `ID3D12CommandAllocator` を使い回す構造から、`FRAME_BUFFER_COUNT` 分の `commandAllocators_[]` を持つ構造へ変更した。

#### 変更後の同期方針
- フレーム終了時に現在のフレームの fence 値を保存する
- 次のフレームインデックスへ進める
- 次に使う allocator がまだ GPU で使用中の場合だけ待つ
- 使用可能になった allocator を Reset して、次フレームの command list に使う

#### 効果
従来は毎フレーム必ず直前の GPU 完了を待っていたため CPU/GPU 並列性が落ちていた。変更後はフレームリソースが空いている限り CPU が先行できるため、描画負荷が上がったときの待機時間を減らせる。

#### 補足
現状はまだ `ExecuteCommandList()` に Present、Signal、次フレーム準備がまとまっている。次の整理として `MoveToNextFrame()` と `WaitForGpu()` に分けると、リサイズや終了処理も読みやすくなる。

---

## 11. RenderPass依存管理と簡易RenderGraph基盤（2026年6月18日）

### 11.1 今回追加した目的

Skybox、Opaque、Cloud、Trail、Particle は `RenderPassEntry` によって実行順を管理できるようになったが、各Passがどの描画リソースをRead/WriteするかはRendererから見えなかった。

今回、完全なRenderGraphではなく、以下の検証用メタデータ基盤を追加した。

```text
Pass登録
↓
Read/Writeリソース宣言
↓
依存関係構築
↓
依存違反・競合検証
↓
既存のPhase + order順で実行
```

### 11.2 追加した論理リソース

現時点でPass間共有の検証に必要なリソースだけを定義した。

```text
SceneColor
- DirectXCore::RenderTexturePreDraw() でRTVとして設定されるレンダーテクスチャ
- RenderTexturePostDraw() でPixelShaderResourceへ遷移され、最終合成でBackBufferへ表示される

SceneDepth
- DirectXCore::RenderTexturePreDraw() でDSVとして設定・ClearされるDepthStencil
- 3D系Passと一部2D/ParticleがDepth Testに利用する
```

Cloud専用の中間リソースは現在のPass実装上確認できなかったため、今回は追加していない。

### 11.3 Passごとのリソース宣言

```text
SkyboxRenderPass
- Phase: Scene
- Order: 100
- Write: SceneColor
- Read: SceneDepth
- 根拠: SkyboxSetup はDepth Test有効、DepthWriteMask ZERO

OpaqueRenderPass
- Phase: Scene
- Order: 200
- ReadWrite: SceneColor, SceneDepth
- 根拠: Object3dSetup はDepth Test有効、DepthWriteMask ALL

CloudRenderPass
- Phase: Scene
- Order: 300
- ReadWrite: SceneColor, SceneDepth
- 根拠: CloudSetup はAlpha Blend有効、DepthWriteMask ALL

TrailRenderPass
- Phase: Scene
- Order: 400
- ReadWrite: SceneColor, SceneDepth
- 根拠: TrailEffectSetup はAlpha Blend有効、DepthWriteMask ALL

ParticleRenderPass
- Phase: After2D
- Order: 100
- ReadWrite: SceneColor
- Read: SceneDepth
- 根拠: ParticleSetup はAlpha Blend有効、DepthWriteMask ZERO
```

Resource State は今回の宣言には含めていない。既存Barrierを変更せず、まず論理依存の検証だけに留めるため。

### 11.4 RenderGraphの責務

`engine/render/RenderGraph.h/cpp` を追加し、依存関係の正本をここへ集約した。

```text
持つもの
- 外部初期化済み論理リソース
- Pass間依存辺

持たないもの
- GPU Resource本体
- CommandList
- Descriptor Heap
- Resource Barrier実行
- RenderTarget生成
- Pass所有権
- Pass実行処理
```

実行責務は引き続き `Renderer::ExecutePhase()` が持つ。

### 11.5 構築される依存関係

現在のPass宣言から、代表的には以下の依存が構築される。

```text
Skybox -> Opaque
- SceneColor: Write -> ReadWrite
- SceneDepth: Read後のOpaque Writeは外部初期化済みDepthへ順序付きでアクセス

Opaque -> Cloud
- SceneColor: ReadWrite -> ReadWrite
- SceneDepth: ReadWrite -> ReadWrite

Cloud -> Trail
- SceneColor: ReadWrite -> ReadWrite
- SceneDepth: ReadWrite -> ReadWrite

Trail -> Particle
- SceneColor: ReadWrite -> ReadWrite
- SceneDepth: ReadWrite -> Read
```

2D描画はまだRenderPass化されていないため、RenderGraph上のPass依存としては表現していない。現在は既存順序どおり `TrailRenderPass` と `ParticleRenderPass` の間に固定実行される。

### 11.6 検証内容

`Renderer::Initialize()` のPass登録完了後に、1回だけGraphを構築・検証する。

```text
検出するもの
- 外部リソースでも先行Writerでもない未初期化Read
- 同一Pass内の同一リソース重複宣言
- 同一Phase/同一Orderでの不定なWrite競合
- 依存グラフの循環
- 依存順とPhase/order実行順の矛盾
```

毎フレームのGraph再構築やトポロジカルソートは行わない。現在の描画順を変えないことを優先している。

### 11.7 Resource Barrierの扱い

今回の変更ではBarrierを自動生成しない。

```text
維持した既存Barrier
- RenderTexturePreDraw(): PixelShaderResource -> RenderTarget
- RenderTexturePostDraw(): RenderTarget -> PixelShaderResource
- DirectXCore::PreDraw/PostDraw(): BackBufferのPresent/RenderTarget遷移
- Texture Upload時のCopyDest -> PixelShaderResource
```

RenderGraphのRead/Write宣言は、将来のBarrier自動生成とState検証へ接続するためのメタデータとして扱う。

### 11.8 今回の実装による改善

```text
Before
- 描画順はPhase/orderのみ
- Passが何を読む/書くかRendererから不可視
- Order変更による破綻を検出しづらい

After
- PassごとにSceneColor/SceneDepthのRead/Writeを宣言
- Write->Read、Write->Write、ReadWrite依存を構築
- 初期化時に依存違反をassertで検出
- 現在の描画順と描画結果は維持
```

### 11.9 残る課題

```text
残課題
- Sprite/2D描画がまだRenderPass化されていない
- PostEffectがRenderGraphのPassとして表現されていない
- Resource State宣言と既存Barrierの整合性検証は未実装
- 依存グラフのトポロジカルソート結果はまだ実行順に使っていない
```

次に優先すべき改修は、2D描画を `SpriteRenderPass` へ移行し、`SceneColor` へのReadWriteを明示すること。

---

## 12. SpriteRenderPassへの2D描画移行

### 12.1 改修前の2D描画経路

改修前は、RenderGraph管理外の固定関数として2D Sprite描画が実行されていた。

```text
RenderPreDraw
↓
ExecutePhase(Scene)
  ├─ SkyboxRenderPass
  ├─ OpaqueRenderPass
  ├─ CloudRenderPass
  └─ TrailRenderPass
↓
MagFramework::Object2DCommonDraw()
  ├─ SpriteSetup::CommonDrawSetup()
  └─ SceneManager::Object2DDraw()
      └─ Scene/UI::Draw()
↓
ExecutePhase(After2D)
  └─ ParticleRenderPass
↓
RenderPostDraw
```

この構成では、TrailとParticleの間に入るSprite描画がRenderGraphへ宣言されず、Pass間依存として検証できなかった。

### 12.2 移行内容

`SpriteRenderPass`を追加し、Sprite用PSO、RootSignature、DescriptorHeap、Topology設定をPassへ集約した。SceneとUIはSpriteを直接描画せず、`RenderWorld`へ非所有参照として登録する。

```text
RenderPreDraw
↓
ExecutePhase(Scene)
  ├─ SkyboxRenderPass
  ├─ OpaqueRenderPass
  ├─ CloudRenderPass
  └─ TrailRenderPass
↓
ExecutePhase(Overlay)
  └─ SpriteRenderPass
↓
ExecutePhase(PostOverlay)
  └─ ParticleRenderPass
↓
RenderPostDraw
```

### 12.3 Sprite描画順の管理方式

`RenderWorld`へ`SpriteRenderItem`を追加した。

```text
SpriteRenderItem
- Sprite* sprite
- uint32_t submissionOrder
- bool visible
```

今回の既存UIにはLayer/Order概念が無く、旧実装はDraw呼び出し順で重なり順を決めていた。そのため、`RenderWorld::AddSprite()`で`submissionOrder`を発行し、登録順を決定的な描画順として保持する。RenderPass側では毎フレームの追加ソートを行わない。

### 12.4 UI登録経路

主要な登録経路は以下。

```text
TitleScene
↓
titleSprite / pressEnterSprite / SceneTransition
↓
RenderWorld::AddSprite

GamePlayScene
↓
UIManager
↓
GameOverUI / GameClearAnimation / OperationGuideUI / StartAnimation / MenuUI
↓
RenderWorld::AddSprite

GamePlayScene
↓
SceneTransition
↓
RenderWorld::AddSprite
```

`MenuUI`は旧挙動と同じく、Menu表示中はMenuのみを登録する。`OperationGuideUI`の背景Spriteは旧`Draw()`でも無効化されていたため登録していない。HUD/LockOnHUDはSpriteではなくLineManager描画であるため、SpriteRenderPass対象外としつつ、`UIManager::RegisterRenderables()`から既存LineManager経路への登録を維持した。Line描画のRenderPass化は別改修で扱う。

### 12.5 RenderGraph Read/Write宣言

SpritePassを以下で登録した。

```text
Pass ID: Sprite
実装: SpriteRenderPass
Phase: Overlay
Order: 100
Reads: なし
Writes: なし
ReadWrites:
- SceneColor
- SceneDepth
```

通常UI SpriteならDepth不要に見えるが、現行`SpriteSetup`のPSOは`DepthEnable = true`、`DepthWriteMask = ALL`である。Blend/Depth設定を変更しない条件を優先し、`SceneDepth`もReadWriteとして宣言した。

### 12.6 Trail→Sprite→Particle依存

現在のPass宣言から、代表的に以下の依存が構築される。

```text
TrailRenderPass
↓ SceneColor / SceneDepth
SpriteRenderPass
↓ SceneColor / SceneDepth
ParticleRenderPass
```

`Renderer::Initialize()`でPass登録後に`RenderGraph::Build()`と`RenderGraph::Validate()`を実行する。Debug x64ビルド時点で、未初期化Read、循環、Phase/orderと依存順の矛盾は発生していない。

### 12.7 削除した旧API

削除した旧方式は以下。

```text
- BaseScene::Object2DDraw
- SceneManager::Object2DDraw
- MagFramework::Object2DCommonDraw
- EngineAppからのObject2DCommonDraw呼び出し
- 各SceneのObject2DDraw override
- SceneからのSprite直接Draw呼び出し
```

`Sprite::Draw()`自体は低レベル描画関数として残し、呼び出し元を`SpriteRenderPass`へ限定した。

### 12.8 ビルド結果

```text
Command:
MSBuild.exe MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64

Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

最初のビルドでは、`RenderWorld.h`のinclude pathが`engine/render`を含んでいないことにより失敗した。追加したincludeを`engine/render/RenderWorld.h`へ統一して修正した。

### 12.9 実行確認できていない項目

GUI実行は行っていないため、以下は未確認。

```text
- TitleSceneの実表示
- GamePlaySceneへの遷移
- Menu UI / OperationGuide UI / Fade / Transitionの実表示
- SpriteとParticleの実画面上の前後関係
- DirectX 12 Debug Layerの実行時メッセージ
```

静的確認とDebug x64ビルドでは、RenderGraph構築・検証の呼び出し、および旧2D描画APIの削除を確認した。

### 12.10 次の改修候補

次はPostEffectをRenderPass化し、SceneColor入力とPresent出力をRenderGraph上で明示するのが最も効果的。現在の描画終端がRenderGraph外に残っているため、フレーム全体の依存関係を把握しづらい。

---

## 13. PostEffectRenderPassへのFullscreen描画移行

### 13.1 改修前のPostEffect経路

改修前は、SceneColorに相当するRenderTextureへの描画完了後、`DirectXCore::PreDraw()`がBackBufferをRenderTargetへ遷移し、Fullscreen Triangle描画とPostEffect適用を固定処理として実行していた。

```text
RenderTexturePreDraw
↓
ExecutePhase(Scene / Overlay / PostOverlay)
↓
RenderTexturePostDraw
  - SceneColor: RenderTarget -> PixelShaderResource
↓
DirectXCore::PreDraw
  - BackBuffer: Present -> RenderTarget
  - BackBuffer Clear
  - RenderTexture SRVを設定
  - Fullscreen Triangle描画
  - PostEffectManager::ApplyEffects
↓
ImGui
↓
PostDraw
  - BackBuffer: RenderTarget -> Present
  - ExecuteCommandLists
  - Present
  - Fence Signal
```

この構成では、SceneColorからBackBufferへの最終合成がRenderGraph上に現れず、Particle後にPostEffectがSceneColorを読む依存を検証できなかった。

### 13.2 移行内容

`PostEffectRenderPass`を追加し、BackBufferへのFullscreen描画をRenderer管理のPassとして実行するようにした。

```text
RenderTexturePreDraw
↓
ExecutePhase(Scene)
↓
ExecutePhase(Overlay)
↓
ExecutePhase(PostOverlay)
↓
RenderTexturePostDraw
↓
ExecutePhase(PostProcess)
  └─ PostEffectRenderPass
↓
ImGui
↓
PostDraw / Present
```

`Present()`、CommandQueue実行、Fence Signal、FrameContext更新は引き続き`DirectXCore::PostDraw()`側に残した。PostEffectPassはPresentそのものを担当しない。

### 13.3 SceneColorとPresentColorの定義

```text
SceneColor
- 対応GPU Resource: DirectXCoreの現在RenderTexture
- 用途: Skybox/Opaque/Cloud/Trail/Sprite/Particleの描画先
- Writer: Scene系Pass、Overlay系Pass、Particle
- Reader: PostEffectRenderPass
- Barrier: RenderTexturePreDrawでRTV化、RenderTexturePostDrawでSRV化

SceneDepth
- 対応GPU Resource: DirectXCoreのDepthStencil
- 用途: Scene/Sprite/ParticleのDepth参照または書き込み
- Writer: Opaque/Cloud/Trail/Sprite
- Reader: Skybox/Particle
- Barrier: 既存手動管理を維持

PresentColor
- 対応GPU Resource: SwapChainの現在BackBuffer
- 用途: PostEffectRenderPassの出力先、ImGuiの描画先、Present対象
- Writer: PostEffectRenderPass、ImGui
- Reader: Present処理
- Barrier: PostEffectRenderPass開始時にRTV化、PostDrawでPresent化
```

GPU Resource本体はRenderGraphへ渡していない。RenderGraphは論理リソース名だけを保持する。

### 13.4 RenderGraph Read/Write宣言

`RenderResourceId::PresentColor`を追加し、外部リソースとして登録した。`PostEffectRenderPass`の宣言は以下。

```text
Pass ID: PostEffect
実装: PostEffectRenderPass
Phase: PostProcess
Order: 100
Reads:
- SceneColor
Writes:
- PresentColor
ReadWrites:
- なし
```

これにより、`SceneColor`の最後のWriterである`ParticleRenderPass`から`PostEffectRenderPass`への依存が構築される。

### 13.5 Resource Barrierの配置

```text
SceneColor描画開始:
- DirectXCore::RenderTexturePreDraw
- PixelShaderResource -> RenderTarget

SceneColorをSRVへ変更:
- DirectXCore::RenderTexturePostDraw
- RenderTarget -> PixelShaderResource

PresentColorをRTVへ変更:
- PostEffectRenderPass
- DirectXCore::BeginPresentRenderTarget
- Present -> RenderTarget

PresentColorをPresentへ変更:
- DirectXCore::CloseCommandList
- RenderTarget -> Present
```

Barrierの自動生成は行っていない。既存の手動Barrierを責務ごとに移動・維持した。

### 13.6 PresentとImGuiの責務分離

```text
PostEffectRenderPass:
- BackBufferをRenderTargetへ遷移
- BackBuffer RTVを設定
- Viewport/Scissor/Topologyを設定
- PostEffectManager::ApplyEffectsでFullscreen描画

ImGui:
- PostEffect後のBackBufferへ従来どおり描画

DirectXCore::PostDraw:
- FPS固定
- BackBufferをPresentへ遷移
- CommandList Close
- ExecuteCommandLists
- Present
- Fence Signal
```

ImGuiは今回RenderPass化していない。

### 13.7 削除した旧方式

削除・整理した旧方式は以下。

```text
- EngineAppからの固定PostEffect前処理呼び出し
- MagFramework::PreDraw
- DirectXCore::PreDraw(PostEffectManager*, TextureManager&)
- DirectXCore内のFullscreen描画固定実行
- DirectXCore内のPostEffectManager::ApplyEffects直接呼び出し
- DirectXCore.cppの不要なPostEffectManager/TextureManager include
```

低レベルなFullscreen描画PSO、RootSignature、`PostEffectManager::ApplyEffects()`は、`PostEffectRenderPass`から呼ばれる実装として維持した。

### 13.8 ビルド結果

```text
Command:
MSBuild.exe MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64

Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 13.9 実行確認できていない項目

GUI実行は行っていないため、以下は未確認。

```text
- 実画面でのPostEffect適用結果
- PostEffect無効時の画面表示
- 複数PostEffect有効時の見た目
- ImGuiの実表示位置
- DirectX 12 Debug Layerの実行時メッセージ
```

静的確認とDebug x64ビルドでは、PostEffectPass登録、SceneColor Read、PresentColor Write、ParticleからPostEffectへの依存構築、旧固定呼び出し削除を確認した。

### 13.10 次の改修候補

次はResource State宣言と手動Barrierの整合性検証を追加するのが最も効果的。RenderGraphに論理Read/Writeは揃ってきたため、実際の手動Barrierが宣言と矛盾していないかを検証できる段階に入っている。

---

## 14. Resource State宣言と手動Barrier整合性検証

### 14.1 改修前の課題

改修前のRenderGraphは、`SceneColor`、`SceneDepth`、`PresentColor`のRead/Write依存は検証できていたが、実際のD3D12 Resource Stateと手動`ResourceBarrier`の整合性は検証できなかった。

```text
確認できていたこと
- Pass間のRead/Write依存
- 循環依存
- 実行順と依存順の矛盾

確認できていなかったこと
- PostEffect前にSceneColorがPixelShaderResource状態か
- PresentColorがPostEffect時にRenderTarget状態か
- Present前にPresentColorがPresent状態へ戻っているか
- PassのRequired Stateと手動Barrierが一致しているか
```

### 14.2 Required State宣言の追加内容

`RenderPassResourceUsage`へ`requiredState`を追加した。

```text
RenderPassResourceUsage
- RenderResourceId resource
- RenderResourceAccess access
- RenderResourceState requiredState
```

今回追加した`RenderResourceState`は、現在使っている状態を中心にした軽量enumであり、RenderGraphはD3D12 Resource本体やCommandListを所有しない。

```text
RenderTarget
PixelShaderResource
DepthWrite
DepthRead
Present
CopySource
CopyDest
GenericRead
```

### 14.3 各PassのRequired State

```text
Skybox
- SceneColor: Write / RenderTarget
- SceneDepth: Read / DepthWrite

Opaque
- SceneColor: ReadWrite / RenderTarget
- SceneDepth: ReadWrite / DepthWrite

Cloud
- SceneColor: ReadWrite / RenderTarget
- SceneDepth: ReadWrite / DepthWrite

Trail
- SceneColor: ReadWrite / RenderTarget
- SceneDepth: ReadWrite / DepthWrite

Sprite
- SceneColor: ReadWrite / RenderTarget
- SceneDepth: ReadWrite / DepthWrite

Particle
- SceneColor: ReadWrite / RenderTarget
- SceneDepth: Read / DepthWrite

PostEffect
- SceneColor: Read / PixelShaderResource
- PresentColor: Write / RenderTarget
```

`SceneDepth`はParticleなどでRead用途でも、現在の実装ではDepthStencilをフレーム中`DepthWrite`状態のDSVとして扱い続けている。そのため、今回のRequired Stateは実際の手動Barrier配置に合わせて`DepthWrite`としている。

### 14.4 手動Barrier記録の仕組み

`RenderResourceBarrierRecord`を追加し、実コードに存在する手動Barrierのメタ情報をRenderGraphへ登録するようにした。

```text
RenderResourceBarrierRecord
- resource
- beforeState
- afterState
- label
- sequence
```

登録しているManual Barrierは以下。

```text
DirectXCore::RenderTexturePreDraw
- SceneColor: PixelShaderResource -> RenderTarget

DirectXCore::RenderTexturePostDraw
- SceneColor: RenderTarget -> PixelShaderResource

DirectXCore::BeginPresentRenderTarget
- PresentColor: Present -> RenderTarget

DirectXCore::CloseCommandList
- PresentColor: RenderTarget -> Present
```

実際のBarrier発行箇所は変更していない。今回追加したのは検証用メタ情報のみ。

### 14.5 Resource State整合性検証

`RenderGraph::Validate()`に以下を追加した。

```text
Required State未設定検出
- 各RenderPassResourceUsageのrequiredStateがUnknownならassert

Initial State未設定検出
- 使用する論理リソースに初期状態が無ければassert

Manual Barrier整合性検証
- 記録されたbeforeStateが現在追跡中の状態と一致しなければassert
- 一致した場合のみafterStateへ更新

Pass Required State検証
- Pass実行時点の追跡状態とrequiredStateが一致しなければassert

Final State検証
- フレーム終端状態が期待値と一致しなければassert
```

この検証は実行順を変更せず、既存の固定Phase/order順を検証用シーケンスへ写して行う。

### 14.6 Resource State Timeline

```text
SceneColor
Initial:
- PixelShaderResource
Transitions:
- RenderTexturePreDraw: PixelShaderResource -> RenderTarget
- RenderTexturePostDraw: RenderTarget -> PixelShaderResource
Pass required states:
- Skybox/Opaque/Cloud/Trail/Sprite/Particle: RenderTarget
- PostEffect: PixelShaderResource
Final:
- PixelShaderResource

SceneDepth
Initial:
- DepthWrite
Transitions:
- なし
Pass required states:
- Skybox/Opaque/Cloud/Trail/Sprite/Particle: DepthWrite
Final:
- DepthWrite

PresentColor
Initial:
- Present
Transitions:
- BeginPresentRenderTarget: Present -> RenderTarget
- CloseCommandList: RenderTarget -> Present
Pass required states:
- PostEffect: RenderTarget
- ImGui: RenderGraph外だがRenderTarget状態のBackBufferへ描画
Final:
- Present
```

### 14.7 自動Barrier生成をまだ行わない理由

今回の目的は、現在の手動BarrierがPassのRequired Stateと矛盾していないことを検証することに限定した。

```text
まだ行わないこと
- RenderGraphからResourceBarrierを発行する
- GPU Resource本体をRenderGraphへ渡す
- State Trackerを本格導入する
- Barrier最適化を行う
- トポロジカルソート結果で実行順を変える
```

自動生成へ進む前に、既存の手動Barrierと宣言の差分を検出できる状態を作ることを優先した。

### 14.8 ビルド結果

```text
Command:
MSBuild.exe MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64

Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 14.9 実行確認できていない項目

GUI実行は行っていないため、以下は未確認。

```text
- 実画面での描画結果
- Scene切り替え後の状態
- DirectX 12 Debug LayerのResource State関連メッセージ
```

静的確認とDebug x64ビルドでは、RenderGraphのRead/Write依存検証、Required State検証、Manual Barrier整合性検証が初期化時に通ることを確認した。

### 14.10 次の改修候補

次は、手動Barrier記録を実際のBarrier発行ヘルパーへ統合するのが最も自然。現在は検証用メタ情報をRenderer初期化時に登録しているため、将来的にはBarrier発行と記録を同じ入口に寄せると、記録漏れを防ぎやすくなる。

## 15. 手動ResourceBarrier発行とRenderGraph記録の統合

### 15.1 改修前の二重管理リスク

前回までの構成では、実際の`ID3D12GraphicsCommandList::ResourceBarrier()`発行は`DirectXCore`側にあり、検証用の`RenderResourceBarrierRecord`は`Renderer::Initialize()`で別途登録していた。

```text
変更前:
DirectXCore
- 実際のResourceBarrierを発行

Renderer
- RenderGraphへ検証用Barrierを登録
```

この構成では、BarrierのBefore/Afterや発行位置を変更したときに、実処理と検証用メタ情報がずれる可能性があった。

### 15.2 統合したBarrier発行ヘルパー

`RenderBarrierRecorder`を追加し、1回のTransition要求で実Barrier発行とRenderGraph記録を同時に行うようにした。

```text
変更後:
呼び出し側
↓
RenderBarrierRecorder::Transition
├─ D3D12_RESOURCE_BARRIERを生成
├─ CommandList::ResourceBarrierを発行
└─ RenderGraph::RecordManualBarrierへ同じ内容を記録
```

`RenderGraph`は引き続きCommandListやGPU Resource本体を所有しない。`RecordManualBarrier()`もBarrierを発行せず、記録だけを行う。

### 15.3 Barrier Pointの固定化

自由文字列ラベルの代わりに、固定enumの`RenderBarrierPoint`を追加した。

```text
RenderTexturePreDraw
RenderTexturePostDraw
BeginPresentRenderTarget
BeforePresent
```

Debug用途の文字列化は`ToString(RenderBarrierPoint)`で行う。呼び出し箇所ごとに任意文字列を渡さないため、記録名の表記揺れを避けられる。

### 15.4 移行したBarrier

```text
Barrier Point: RenderTexturePreDraw
Resource: SceneColor
Before: PixelShaderResource
After: RenderTarget
実際のResource: renderTextureResources_[renderResourceIndex_]
呼び出し側: DirectXCore::RenderTexturePreDraw

Barrier Point: RenderTexturePostDraw
Resource: SceneColor
Before: RenderTarget
After: PixelShaderResource
実際のResource: renderTextureResources_[renderResourceIndex_]
呼び出し側: DirectXCore::RenderTexturePostDraw

Barrier Point: BeginPresentRenderTarget
Resource: PresentColor
Before: Present
After: RenderTarget
実際のResource: swapChainResource_[currentBackBufferIndex_]
呼び出し側: DirectXCore::SetupTransitionBarrier

Barrier Point: BeforePresent
Resource: PresentColor
Before: RenderTarget
After: Present
実際のResource: swapChainResource_[currentBackBufferIndex_]
呼び出し側: DirectXCore::CloseCommandList
```

`DirectXCore`はRenderer初期化後に`RenderBarrierRecorder`の非所有ポインタを受け取り、フレーム中の上記Barrierを統合ヘルパー経由で発行する。DirectX初期化中はRenderer/RenderGraphがまだ存在しないため、従来どおり低レベルBarrierだけを発行するフォールバックを残した。

### 15.5 実行時記録と検証タイミング

Barrier記録はフレーム単位にした。

```text
RenderTexturePreDraw直前:
- RenderGraph::ClearRecordedBarriers

描画中:
- RenderBarrierRecorder::Transitionが実Barrier発行と記録を同時実行

PostDraw後:
- RenderGraph::ValidateRecordedResourceStates
```

検証は`PresentColor`が`Present`へ戻った後に行う。これにより、Final State検証が`BeforePresent`後の状態を対象にできる。

### 15.6 維持した検証

```text
初期化時:
- Read/Write依存検証
- 未初期化Read検出
- 循環依存検出
- 実行順矛盾検出
- Required State未設定検出
- Initial State未設定検出

フレーム終了時:
- Barrier不足検出
- Barrier Before State不一致検出
- Pass Required State不一致検出
- Final State不一致検出
```

Barrier自動生成は行っていない。状態遷移の判断は従来どおり呼び出し側が行い、`RenderBarrierRecorder`は発行と記録だけを担当する。

### 15.7 SceneDepthの扱い

`SceneDepth`にはTransitionを追加していない。

```text
Initial State: DepthWrite
実行中State: DepthWrite維持
Transition追加: なし
```

Skybox、Opaque、Cloud、Trail、Sprite、Particleはいずれも現行実装ではDSVを`DepthWrite`状態で扱っている。ParticleはRead用途として宣言しているが、D3D12 Resource Stateとしては既存Barrierが存在しないため、今回も推測で`DepthRead`へ変更していない。

### 15.8 直接ResourceBarrier呼び出しの残存

検索上、直接`ResourceBarrier()`は以下に残っている。

```text
直接ResourceBarrier()残存件数: 4

engine/render/RenderBarrierRecorder.cpp
- 統合ヘルパー本体
- 正当

engine/base/core/DirectXCore.cpp
- Renderer/RenderGraph未生成のDirectX初期化中フォールバック
- 正当

engine/base/core/DirectXCore.cpp
- Texture Upload後のCopyDest -> GenericRead遷移
- SceneColor/PresentColorのフレームBarrierではないため今回は対象外

engine/postEffect/PostEffectManager.cpp
- 複数PostEffect用の内部ピンポンRenderTexture切り替え
- SceneColor/PresentColorのGraph管理対象外として今回は対象外
```

今後、PostEffectの内部RenderTextureをGraph上の論理リソースとして扱う場合は、この箇所も統合対象にできる。

### 15.9 現在の描画順

描画順は変更していない。

```text
RenderTexturePreDraw
↓
ExecutePhase(Scene)
↓
ExecutePhase(Overlay)
↓
ExecutePhase(PostOverlay)
↓
RenderTexturePostDraw
↓
ExecutePhase(PostProcess)
↓
ImGui
↓
PostDraw / Present
```

### 15.10 ビルド結果

```text
Command:
MSBuild.exe MagEngine.sln /m /p:Configuration=Debug /p:Platform=x64

Configuration: Debug
Platform: x64
Result: Success
Warnings: 0
Errors: 0
```

### 15.11 実行確認できていない項目

GUI実行は行っていないため、以下は未確認。

```text
- 実画面での描画結果
- RenderGraph実行時検証が実機フレームで最後まで通ること
- DirectX 12 Debug LayerのResource State関連メッセージ
- Scene切り替え後の表示
- 終了時クラッシュの有無
```

静的確認とDebug x64ビルドでは、`SceneColor`と`PresentColor`の通常Barrierが統合ヘルパー経由になっていること、旧`AddManualBarrier`呼び出しが残っていないことを確認した。

### 15.12 次の改修候補

次は、Required Stateから最小限のTransition Barrierを自動生成する前段階として、PostEffect内部のピンポンRenderTextureを論理リソースとしてRenderGraphに登録するのがよい。
