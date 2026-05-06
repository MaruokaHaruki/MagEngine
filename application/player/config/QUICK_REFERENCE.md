# PlayerConfigLoader クイックリファレンス

## 📌 ファイル場所

| ファイル | パス |
|---------|------|
| **設定ファイル** | `resources/config/player/player_config.json` |
| **ヘッダ** | `application/player/config/PlayerConfigLoader.h` |
| **実装** | `application/player/config/PlayerConfigLoader.cpp` |
| **ドキュメント** | `resources/config/player/README.md` |

## 🚀 使い始める

### 1. ゲーム初期化時
```cpp
#include "PlayerConfigLoader.h"

void Game::Initialize() {
    // 設定ファイルを読み込む
    if (!PlayerConfigLoader::LoadConfig()) {
        std::cerr << "Failed to load player config, using defaults" << std::endl;
    }
    
    // 初期化成功確認
    if (PlayerConfigLoader::IsInitialized()) {
        std::cout << "Player config loaded successfully" << std::endl;
    }
}
```

### 2. コンポーネント内での使用
```cpp
#include "PlayerConfigLoader.h"

void PlayerMovementComponent::Initialize() {
    moveSpeed_ = PlayerConfigLoader::GetDefaultMoveSpeed();
    maxRollAngle_ = PlayerConfigLoader::GetMaxRollAngle();
    boostGauge_ = PlayerConfigLoader::GetBoostMaxGauge();
}
```

## 📋 主要ゲッターメソッド一覧

### Timing / Input
```cpp
float frameTime = PlayerConfigLoader::GetFrameTime();
float frameRate = PlayerConfigLoader::GetFrameRate();
float stickDeadzone = PlayerConfigLoader::GetStickDeadzone();
float triggerThreshold = PlayerConfigLoader::GetTriggerThreshold();
```

### Weapon (Bullet)
```cpp
float bulletSpeed = PlayerConfigLoader::GetBulletSpeed();           // 128.0
float bulletLifetime = PlayerConfigLoader::GetBulletLifetime();     // 3.0
float bulletRadius = PlayerConfigLoader::GetBulletRadius();         // 0.5
float shootCooldown = PlayerConfigLoader::GetShootCooldown();       // 0.1
```

### Weapon (Missile)
```cpp
float missileSpeed = PlayerConfigLoader::GetMissileSpeed();         // 50.0
float missileTurnRate = PlayerConfigLoader::GetMissileTurnRate();   // 120.0
float missileLifetime = PlayerConfigLoader::GetMissileLifetime();   // 15.0
int missileMaxAmmo = PlayerConfigLoader::GetMissileMaxAmmo();       // 3
float missileRecoveryTime = PlayerConfigLoader::GetMissileRecoveryTime(); // 3.0
```

### Movement
```cpp
float moveSpeed = PlayerConfigLoader::GetDefaultMoveSpeed();               // 5.0
float acceleration = PlayerConfigLoader::GetDefaultAcceleration();         // 0.1
float rotationSmoothing = PlayerConfigLoader::GetDefaultRotationSmoothing(); // 0.1
float maxRollAngle = PlayerConfigLoader::GetMaxRollAngle();               // 30.0
float maxPitchAngle = PlayerConfigLoader::GetMaxPitchAngle();             // 15.0
```

### Boost
```cpp
float maxGauge = PlayerConfigLoader::GetBoostMaxGauge();                   // 100.0
float speedMultiplier = PlayerConfigLoader::GetBoostSpeedMultiplier();     // 2.0
float consumptionRate = PlayerConfigLoader::GetBoostConsumptionRate();     // 30.0
float recoveryRate = PlayerConfigLoader::GetBoostRecoveryRate();           // 15.0
```

### BarrelRoll
```cpp
float duration = PlayerConfigLoader::GetBarrelRollDuration();              // 0.6
float cooldown = PlayerConfigLoader::GetBarrelRollCooldown();              // 1.2
float cost = PlayerConfigLoader::GetBarrelRollCost();                      // 30.0
float accelMult = PlayerConfigLoader::GetBarrelRollAccelerationMultiplier(); // 2.0
float rotationRad = PlayerConfigLoader::GetBarrelRollRotationAngleRadians(); // 6.28
```

### LockOn
```cpp
float range = PlayerConfigLoader::GetLockOnRange();                        // 50.0
float fov = PlayerConfigLoader::GetLockOnFOVDegrees();                     // 180.0
float acqInterval = PlayerConfigLoader::GetLockOnAcquisitionInterval();    // 0.35
int maxTargets = PlayerConfigLoader::GetLockOnMaxTargets();               // 3
float retentionTime = PlayerConfigLoader::GetLockOnRetentionTime();        // 0.5
```

### JustAvoidance
```cpp
float windowSize = PlayerConfigLoader::GetJustAvoidanceWindowSize();           // 0.3
float boostReward = PlayerConfigLoader::GetJustAvoidanceBoostReward();         // 30.0
float damageTimeout = PlayerConfigLoader::GetJustAvoidanceDamageTimeout();     // 1.0
float perfectThreshold = PlayerConfigLoader::GetJustAvoidancePerfectTimingThreshold(); // 0.8
```

### Health
```cpp
int maxHP = PlayerConfigLoader::GetDefaultMaxHP();                         // 100
float invincibility = PlayerConfigLoader::GetInvincibilityDuration();      // 1.0
int enemyBulletDamage = PlayerConfigLoader::GetEnemyBulletDamage();       // 15
int collisionDamage = PlayerConfigLoader::GetCollisionDamage();            // 10
```

### Defeat
```cpp
float animDuration = PlayerConfigLoader::GetDefeatAnimationDuration();     // 3.0
float phase1Ratio = PlayerConfigLoader::GetDefeatPhase1Ratio();            // 0.6
float gravity = PlayerConfigLoader::GetDefeatGravityAcceleration();        // 15.0
float groundY = PlayerConfigLoader::GetDefeatGroundYThreshold();           // -50.0
float noseDive = PlayerConfigLoader::GetDefeatNoseDiveAngle();             // -90.0
```

## 📝 JSON設定ファイルの編集

### ファイルパス
```
resources/config/player/player_config.json
```

### 基本構造
```json
{
  "timing": { "frameTime": 0.016666666667, "frameRate": 60.0 },
  "input": { "stickDeadzone": 0.1, "triggerThreshold": 0.3 },
  "weapon": {
    "bullet": {
      "speed": 128.0,
      "lifetime": 3.0,
      "radius": 0.5,
      "shootCooldown": 0.1
    },
    "missile": {
      "speed": 50.0,
      "turnRate": 120.0,
      "lifetime": 15.0,
      "maxAmmo": 3,
      "recoveryTime": 3.0
    }
  },
  // ... 他のセクション
}
```

### 編集例: 弾の速度を変更
```json
"weapon": {
  "bullet": {
    "speed": 200.0  // ← 128.0 から 200.0 に変更
  }
}
```

### 編集例: HPを増加
```json
"health": {
  "defaultMaxHP": 150  // ← 100 から 150 に変更
}
```

## ⚙️ 設定のカスタマイズ戦略

### パターン1: 難易度別設定
複数のJSON設定ファイルを用意：
- `player_config_easy.json`
- `player_config_normal.json`
- `player_config_hard.json`

```cpp
// 難易度に応じて読み込み
std::string configPath = "resources/config/player/player_config_" + difficulty + ".json";
PlayerConfigLoader::LoadConfig(configPath);
```

### パターン2: ステージ別設定
ステージごとに設定を微調整：
```cpp
// ステージ読み込み時
std::string stagePath = "resources/config/player/stage_" + stageId + ".json";
PlayerConfigLoader::LoadConfig(stagePath);
```

### パターン3: テスト用設定
デバッグ・テスト専用設定を用意：
```cpp
#ifdef _DEBUG
PlayerConfigLoader::LoadConfig("resources/config/player/player_config_debug.json");
#else
PlayerConfigLoader::LoadConfig("resources/config/player/player_config.json");
#endif
```

## 🔍 トラブルシューティング

### JSONファイルが見つからない
→ デフォルト値（PlayerConstants.h）を使用して実行継続
→ コンソール出力: `Failed to open player config: ...`

### JSON形式が正しくない
→ JSON バリデータを使用して検証
→ 推奨: VSCode の JSON 拡張機能

### 値が反映されない
1. PlayerConfigLoader::LoadConfig() がゲーム起動時に呼ばれているか確認
2. JSON のスペルミスを確認 (camelCase で記述)
3. 値の型が正しいか確認 (数値か文字列か)

## 📊 パフォーマンス特性

| 項目 | 説明 |
|------|------|
| **読み込み時間** | 初回のみ（LoadConfig時） |
| **アクセス時間** | O(1) グローバル変数直接参照 |
| **メモリオーバーヘッド** | 約 1KB（全キャッシュ値） |
| **JSON解析ライブラリ** | nlohmann/json （ヘッダのみ） |

## 📌 ベストプラクティス

✅ **DO:**
- LoadConfig() をゲーム初期化時に1度呼ぶ
- 定期的に IsInitialized() で状態確認
- JSON値を複数回読む場合はローカル変数にキャッシュ
- 異なる設定シナリオをJSON別ファイルで管理

❌ **DON'T:**
- ゲーム実行中に LoadConfig() を呼ぶ（キャッシュが上書きされます）
- PlayerConstants.h の値を直接使用（将来的に削除予定）
- JSON解析後に手動で値変更（キャッシュと不整合）

## 🔗 関連ドキュメント

- `resources/config/player/README.md` - 詳細ガイド
- `application/player/PlayerConstants.h` - デフォルト値定義
- `application/enemy/config/ConfigLoader.*` - 敵設定ローダー（参考）

---

**最終更新**: 2026-05-06
**バージョン**: 1.0
