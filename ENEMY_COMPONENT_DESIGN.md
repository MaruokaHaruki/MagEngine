# MagEngine エネミーシステム - コンポーネントベース設計ドキュメント

## 概要

MagEngine の敵（Enemy）システムを、**継承ベース → コンポーネントベース**に完全に置き換えます。

新設計により：
- ✅ 新敵追加が容易（JSON + Factory）
- ✅ 難易度調整が外部ファイルで可能
- ✅ 部隊AI が独立した専門システムに
- ✅ 個体AI と部隊AI が完全分離
- ✅ 責務が明確に分割される

---

## アーキテクチャ全体図

```
┌─────────────────────────────────────────────────────────┐
│                 EnemyWaveManager                         │
│  (ウェーブ制御・敵スポーン管理)                            │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   EnemyFactory                           │
│  JSON設定から敵生成 (コンポーネント組み立て)                │
└─────────────────────────────────────────────────────────┘
                          ↓
┌────────────────────┬─ Enemy ─────────────────────────────┐
│                    (コンテナ・統合)                        │
│  ┌──────────────────────────────────────────────────┐   │
│  │      IEnemyComponent (基底インターフェース)       │   │
│  │ ・Initialize(config, owner)                      │   │
│  │ ・Update(deltaTime)                              │   │
│  │ ・Draw(), DrawImGui()                            │   │
│  └──────────────────────────────────────────────────┘   │
│           ↓                                               │
│  ┌─────────────────────────────────────┐                │
│  │  7種類のコンポーネント実装             │                │
│  │                                     │                │
│  │ 1. TransformComponent               │                │
│  │    - 位置、回転、スケール            │                │
│  │                                     │                │
│  │ 2. HealthComponent                  │                │
│  │    - HP管理、ダメージ処理            │                │
│  │                                     │                │
│  │ 3. MovementComponent                │                │
│  │    - 移動制御、速度管理              │                │
│  │                                     │                │
│  │ 4. AIComponent                      │                │
│  │    - 個体AI戦略の実行                │                │
│  │                                     │                │
│  │ 5. CombatComponent                  │                │
│  │    - 射撃・攻撃制御                  │                │
│  │                                     │                │
│  │ 6. GroupComponent                   │                │
│  │    - 部隊参加情報・命令受信          │                │
│  │                                     │                │
│  │ 7. VisualComponent                  │                │
│  │    - 描画・アニメーション            │                │
│  └─────────────────────────────────────┘                │
└────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                    SquadAI                               │
│  (部隊管理・協力戦術・リーダー制御)                        │
│  ・V字フォーメーション                                    │
│  ・包囲戦術                                              │
│  ・支援射撃                                              │
└─────────────────────────────────────────────────────────┘

                     Config Files
┌──────────────────┬──────────────────┬──────────────────┐
│  enemies.json    │  waves.json      │ ai_strategies.json
│  (敵定義)        │  (ウェーブ定義)   │  (AI戦略定義)
│                  │                  │
│ - fighter        │ Wave 1: Spawn    │ - Approach
│ - scout          │   Time, Pos,     │ - Orbit
│ - gunner         │   EnemyType      │ - Shooting
│ - ...            │                  │ - Retreat
│                  │ Wave 2: ...      │
└──────────────────┴──────────────────┴──────────────────┘
```

---

## クラス設計詳細

### 1. IEnemyComponent（基底インターフェース）

```cpp
class IEnemyComponent {
    virtual void Initialize(const ComponentConfig& config, Enemy* owner) = 0;
    virtual void Update(float deltaTime) {}
    virtual void Draw() {}
    virtual void DrawImGui() {}
    virtual std::string GetComponentName() const = 0;
protected:
    Enemy* owner_;
};
```

**責務:**
- コンポーネントのライフサイクル管理
- 設定値の初期化
- フレーム更新と描画

---

### 2. 具体的なコンポーネント実装（7種類）

#### TransformComponent
```cpp
class TransformComponent : public IEnemyComponent {
private:
    Vector3 position_;
    Vector3 velocity_;
    float radius_;  // 当たり判定半径
public:
    void MoveTo(const Vector3& target, float speed);
    void UpdatePosition(float deltaTime);
};
```

#### HealthComponent
```cpp
class HealthComponent : public IEnemyComponent {
private:
    int maxHP_;
    int currentHP_;
    float damageCooldown_;
public:
    void TakeDamage(int damage);
    bool IsAlive() const { return currentHP_ > 0; }
};
```

#### MovementComponent
```cpp
class MovementComponent : public IEnemyComponent {
private:
    float baseSpeed_;
    float acceleration_;
public:
    void MoveTo(const Vector3& target, float speedMultiplier = 1.0f);
    void Orbit(const Vector3& center, float radius);
};
```

#### AIComponent
```cpp
class AIComponent : public IEnemyComponent {
private:
    std::unique_ptr<IAIBehavior> behavior_;
public:
    void SetBehavior(std::unique_ptr<IAIBehavior> newBehavior);
    void Update(float deltaTime) override;
};
```

#### CombatComponent
```cpp
class CombatComponent : public IEnemyComponent {
private:
    float shootInterval_;
    float shootCooldown_;
    std::string bulletType_;
public:
    void Fire(const Vector3& direction);
};
```

#### GroupComponent
```cpp
class GroupComponent : public IEnemyComponent {
private:
    int groupId_;
    int roleInGroup_;  // 0=Leader, 1=Support, 2=Flanker
public:
    void ReceiveSquadCommand(const SquadCommand& cmd);
};
```

#### VisualComponent
```cpp
class VisualComponent : public IEnemyComponent {
private:
    std::string modelPath_;
    MagEngine::Object3d* object3d_;
public:
    void Draw() override;
    void PlayAnimation(const std::string& name);
};
```

---

### 3. Enemy（コンテナクラス）

```cpp
class Enemy {
private:
    std::unordered_map<std::type_index, std::unique_ptr<IEnemyComponent>> components_;
    std::string enemyTypeId_;
    int enemyId_;
    int groupId_;
    int groupRole_;
    Player* player_;

public:
    template<typename T>
    T* AddComponent() { /* ... */ }

    template<typename T>
    T* GetComponent() const { /* ... */ }

    void Initialize(const std::string& enemyTypeId, const Vector3& position);
    void Update(float deltaTime);
    void Draw();

    // 簡便アクセス
    Vector3 GetPosition() const;
    void SetPosition(const Vector3& pos);
    bool IsAlive() const;
    void TakeDamage(int damage);
};
```

**責務:**
- コンポーネント管理（追加、取得、削除）
- ライフサイクル統合（初期化→更新→描画）
- 敵ID・タイプ管理
- 部隊参加情報管理

---

### 4. AI戦略パターン（IAIBehavior）

```cpp
class IAIBehavior {
    virtual void Update(float deltaTime, Enemy& enemy, const Vector3& playerPos) = 0;
};

// 具体的な戦略
class ApproachBehavior : public IAIBehavior { /* 接近 */ };
class OrbitBehavior : public IAIBehavior { /* 周回戦闘 */ };
class ShootingBehavior : public IAIBehavior { /* 射撃戦闘 */ };
class RetreatBehavior : public IAIBehavior { /* 退却 */ };
class PatrolBehavior : public IAIBehavior { /* パトロール */ };
class ChaseBehavior : public IAIBehavior { /* 追撃 */ };
```

**責務:**
- 敵の個体AI 実装
- 状態遷移ロジック
- プレイヤー相対位置の判定と行動決定

---

### 5. EnemyFactory（ファクトリ）

```cpp
class EnemyFactory {
private:
    std::map<std::string, EnemyDefinition> definitions_;  // enemies.json から読み込み

public:
    void LoadDefinitions(const std::string& jsonPath);
    Enemy* CreateEnemy(const std::string& typeId, const Vector3& pos);
};

struct EnemyDefinition {
    std::string id;
    std::vector<std::string> componentNames;
    std::map<std::string, ComponentConfig> componentConfigs;
};
```

**責務:**
- JSON設定から敵を生成
- コンポーネント組み立て
- 難易度スケーリング適用

---

### 6. SquadAI（部隊AI）

```cpp
class SquadAI {
private:
    Enemy* leader_;
    std::vector<Enemy*> members_;
    SquadFormation formation_;
    SquadStrategy strategy_;

public:
    void ExecuteStrategy(const Vector3& targetPos);
    void UpdateFormation();
    void RemoveMember(Enemy* enemy);
};

enum class SquadStrategy {
    VFormation,      // V字
    Encircle,        // 両包囲
    SuppressionFire, // 支援射撃
    Pincer           // 挟み撃ち
};
```

**責務:**
- 部隊メンバ管理
- フォーメーション制御
- 部隊戦術実行
- リーダー再選出

---

### 7. EnemyWaveManager（改修版）

```cpp
class EnemyWaveManager {
private:
    std::vector<WaveDefinition> waves_;  // waves.json から読み込み
    int currentWaveIndex_;
    float waveTimer_;

public:
    void LoadWaves(const std::string& jsonPath);
    void Update(float deltaTime);
    bool ShouldSpawnNext() const;
    EnemySpawnDefinition GetNextSpawn();
};

struct WaveDefinition {
    int waveId;
    std::vector<EnemySpawnDefinition> spawns;
    float waveDuration;
    float betweenWavePause;
};

struct EnemySpawnDefinition {
    std::string enemyTypeId;
    float spawnTime;
    Vector3 spawnPosition;
    float difficultyMultiplier;
    int groupId;
    int groupRole;
};
```

**責務:**
- ウェーブスケジュール管理
- 敵スポーンタイミング制御
- 次ウェーブへの遷移

---

## JSON設定ファイル形式

### enemies.json
```json
{
  "enemies": {
    "fighter": {
      "id": "fighter",
      "displayName": "戦闘機",
      "components": ["transform", "health", "movement", "ai", "group", "visual"],
      
      "transform": {
        "radius": 1.0
      },
      
      "health": {
        "maxHP": 3,
        "damageCooldown": 0.1
      },
      
      "movement": {
        "baseSpeed": 18.0,
        "acceleration": 5.0
      },
      
      "visual": {
        "modelPath": "assets/models/jet.obj"
      },
      
      "ai": {
        "strategy": "fighter_ai"
      }
    },
    ...
  }
}
```

### waves.json
```json
{
  "waves": [
    {
      "waveId": 1,
      "spawns": [
        {
          "enemyTypeId": "fighter",
          "spawnTime": 0.0,
          "spawnPosition": [-50, 0, 0],
          "difficultyMultiplier": 0.8
        }
      ],
      "waveDuration": 5.0,
      "betweenWavePause": 3.0
    },
    ...
  ]
}
```

### ai_strategies.json
```json
{
  "strategies": {
    "fighter_ai": {
      "behaviors": [
        {
          "name": "Approach",
          "triggerDistance": 80.0,
          "parameters": {
            "speed": 20.0
          }
        },
        {
          "name": "Orbit",
          "triggerDistance": 40.0,
          "parameters": {
            "radius": 40.0,
            "duration": 20.0
          }
        },
        {
          "name": "Retreat",
          "triggerDistance": 10.0,
          "parameters": {
            "speed": 25.0
          }
        }
      ]
    },
    ...
  }
}
```

---

## 実装フェーズ

### Phase 1: 基盤システム（現在地）
- [x] IEnemyComponent インターフェース定義
- [ ] 7種類のコンポーネント実装
- [ ] Enemy コンテナ実装

### Phase 2: AI・Factory
- [ ] IAIBehavior インターフェース定義
- [ ] 具体的なAI戦略実装（Approach, Orbit, Shooting, Retreat）
- [ ] EnemyFactory 実装

### Phase 3: 部隊・管理
- [ ] SquadAI 実装
- [ ] EnemyWaveManager 改修
- [ ] EnemyManager（上位管理）改修

### Phase 4: 設定・テスト
- [ ] JSON設定ファイル作成
- [ ] ビルド・動作確認
- [ ] 既存コードとの統合確認

---

## 期待される効果

| 項目 | 従来（継承） | 新設計（コンポーネント） |
|------|-----------|----------------------|
| **新敵追加** | クラス追加＋コンパイル | JSON＋Factory |
| **難易度調整** | コード修正 | JSON編集 |
| **機能追加** | 基底クラス変更 | コンポーネント追加 |
| **AI切り替え** | コードハード化 | Strategy Pattern |
| **テスト可能性** | 全体テスト必須 | コンポーネント単体テスト |
| **コード再利用** | 限定的 | 高い（コンポーネント共有） |

---

## 今後の段階

本ドキュメントに基づいて、Phase 1 → Phase 4 の順で実装を進めます。

質問やご指示があれば、お聞かせください。
