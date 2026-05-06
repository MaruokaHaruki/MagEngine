# プレイヤー設定ファイル移動ガイド

## 概要
プレイヤーシステムの設定値を`PlayerConstants.h`からJSON設定ファイルに移行しました。
これにより、コンパイルなしにゲームバランスを調整できるようになります。

## 📁 ディレクトリ構造

### 移動前
```
application/player/
├── PlayerConstants.h (全設定値がここに定義)
├── ResourcePaths.h
└── ...
```

### 移動後
```
resources/config/player/
└── player_config.json ← ここに全設定値を移動

application/player/
├── PlayerConstants.h (デフォルト値・互換性維持)
├── ResourcePaths.h (Updated: Config namespace追加)
└── config/
    ├── PlayerConfigLoader.h (新規)
    └── PlayerConfigLoader.cpp (新規)
```

## 📋 作成されたファイル

### 1. `resources/config/player/player_config.json`
全プレイヤー設定値をJSON形式で管理

**構成セクション:**
- `timing`: フレームレート設定
- `input`: コントローラ入力閾値
- `weapon`: 弾・ミサイル設定
  - `bullet`: 弾速度、生存時間、当たり判定など
  - `missile`: ミサイル速度、旋回速度、弾数など
- `movement`: 移動速度、加速度、ロール/ピッチ制限
- `boost`: ブーストゲージ、消費率、回復率
- `barrelRoll`: 実行時間、クールダウン、コスト
- `lockOn`: ロック範囲、視野角、最大ターゲット数
- `justAvoidance`: ジャスト回避ウィンドウ、報酬値
- `health`: HP、無敵時間、ダメージ量
- `defeat`: 敗北演出パラメータ

### 2. `application/player/config/PlayerConfigLoader.h`
JSON設定ファイルを読み込むインターフェース

**主な機能:**
- `LoadConfig(configPath)`: JSON設定ファイルを読み込む
- `IsInitialized()`: 初期化状態を確認
- 60個以上のゲッターメソッドで各パラメータにアクセス可能

**使用例:**
```cpp
// 初期化時
PlayerConfigLoader::LoadConfig("resources/config/player/player_config.json");

// ゲーム実行中にアクセス
float bulletSpeed = PlayerConfigLoader::GetBulletSpeed();
float boostGauge = PlayerConfigLoader::GetBoostMaxGauge();
```

### 3. `application/player/config/PlayerConfigLoader.cpp`
ConfigLoaderの実装

**設計ポイント:**
- `PlayerConstants.h`の値をデフォルト値として使用（互換性維持）
- JSON読み込み失敗時は自動的にデフォルト値にフォールバック
- nlohmann/json ライブラリを使用
- 全パラメータをグローバル変数でキャッシュ（性能最適化）

## 🔄 統合パターン

EnemyConfigLoaderと同じパターンで実装しているため、既存コードベースとの一貫性を保ちます：

```cpp
// 敵設定ローダーと同じ基本構造
ConfigLoader::LoadConfig(path);          // JSON読み込み
ConfigLoader::IsInitialized();            // 初期化確認
ConfigLoader::GetParameterName();         // 値の取得
```

## 🔧 使用方法

### ステップ1: ゲーム初期化時に設定をロード

```cpp
// Main.cpp または GameScene::Initialize()
#include "PlayerConfigLoader.h"

// ゲーム起動時
if (!PlayerConfigLoader::LoadConfig()) {
    std::cerr << "Failed to load player config" << std::endl;
    // デフォルト値で実行を継続
}
```

### ステップ2: コンポーネントでConfigLoaderを使用

```cpp
// 例: PlayerMovementComponent.cpp
#include "PlayerConfigLoader.h"

void PlayerMovementComponent::Initialize() {
    moveSpeed_ = PlayerConfigLoader::GetDefaultMoveSpeed();
    maxRollAngle_ = PlayerConfigLoader::GetMaxRollAngle();
    boostMaxGauge_ = PlayerConfigLoader::GetBoostMaxGauge();
}
```

### ステップ3: JSON設定値を編集

`resources/config/player/player_config.json`を任意のテキストエディタで編集してバランス調整

```json
{
  "weapon": {
    "bullet": {
      "speed": 128.0,    // ← これを変更するだけ！
      "lifetime": 3.0,
      "radius": 0.5,
      "shootCooldown": 0.1
    }
  }
}
```

## ⚠️ 重要な互換性情報

### PlayerConstants.hは残す
- **理由**: デフォルト値として機能し、JSON読み込み失敗時のフォールバックになる
- **用途**: 
  1. コンパイル時の定数参照
  2. 設定ファイルがない場合の初期値
  3. ドキュメント的な役割

### 移行パス
1. **段階1（現状）**: ConfigLoader導入、PlayerConstantsはまだ使用
2. **段階2（オプション）**: 各コンポーネントをPlayerConfigLoaderに切り替え
3. **段階3（オプション）**: 完全移行後、PlayerConstants.hは削除可能

## 📊 アーキテクチャ図

```
┌─────────────────────────────────────────────────────────┐
│                    Game Initialization                   │
│              PlayerConfigLoader::LoadConfig()            │
└────────────────────────┬────────────────────────────────┘
                         │
                         ▼
        ┌────────────────────────────────┐
        │  resources/config/player/      │
        │  player_config.json ◄── Edit   │
        │  (JSON Configuration File)     │
        └────────────────────────────────┘
                         │
                         ▼
        ┌────────────────────────────────┐
        │  PlayerConfigLoader (Static)   │
        │  - LoadConfig()                │
        │  - Get*() methods              │
        │  - Static caching              │
        └────────────────┬───────────────┘
                         │
            ┌────────────┼────────────┬──────────────────┐
            ▼            ▼            ▼                  ▼
        ┌──────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
        │Combat│  │Movement  │  │LockedOn  │  │ Health   │
        │Comp. │  │Component │  │Component │  │Component │
        └──────┘  └──────────┘  └──────────┘  └──────────┘
```

## 🎯 次のステップ

### 推奨される実装順序
1. ✅ PlayerConfigLoader統合（現在ここ）
2. ⏳ 各コンポーネントをConfigLoaderに更新
3. ⏳ PlayerConstants.hの段階的廃止
4. ⏳ RuntimeパラメータUI追加（ImGui連携）

### 拡張可能性
- **複数プレイヤータイプ**: JSON内に複数構成を追加
- **難易度別設定**: 難易度ごとのJSONファイル
- **動的バランス調整**: ゲーム中にパラメータ動的変更
- **設定ファイルバージョニング**: 構成の更新管理

## 📝 コードレビューポイント

このリファクタリングで達成した設計目標：

| 原則 | 実装 |
|------|------|
| **関心の分離** | 設定読み込みをConfigLoaderに専任 |
| **中央集約管理** | resources/configに統一 |
| **テスト性** | ConfigLoaderを独立してテスト可能 |
| **保守性** | JSON形式で編集・管理が容易 |
| **拡張性** | セクション追加で新パラメータ対応 |
| **後方互換性** | PlayerConstants.hデフォルト値維持 |
| **パフォーマンス** | グローバルキャッシュで高速アクセス |

## ❓ FAQ

**Q: JSONファイルが見つからない場合はどうなる?**
A: デフォルト値（PlayerConstants.h）を使用してゲーム継続

**Q: 実行時にパラメータを変更できる?**
A: PlayerConfigLoaderはゲッターのみで、現在は読み込み時の値を使用

**Q: PlayerConstants.hはいつ削除する?**
A: 完全に移行後。互換性維持のため段階的に進める推奨

**Q: 複数の設定ファイルをサポートしたい場合は?**
A: ConfigLoaderに`LoadConfig(variant)`メソッドを追加

---

**最終確認日**: 2026-05-06
**EnemyConfigLoaderとの一貫性**: ✓ 確認済み
