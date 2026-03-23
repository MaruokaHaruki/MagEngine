# ジャスト回避（Just Avoidance）実装ガイド

## 概要

ジャスト回避は、敵の攻撃が迫ってくる直前の一瞬のタイミングでバレルロールを成功させると、
ダメージを受けずにボーストゲージが回復される高度な回避メカニクスです。

正確なタイミングでの回避を報酬する仕組みで、プレイヤーのスキルに応じた爽快感を提供します。

## 実装の流れ

### 1. コンポーネント構成

```
PlayerJustAvoidanceComponent
├─ RegisterIncomingDamage()      敵弾衝突時に呼び出し
├─ OnBarrelRollStarted()          バレルロール開始時に呼び出し
└─ CheckJustAvoidanceSuccess()    ジャスト回避成功判定
```

### 2. コア機能

#### 敵弾衝突の登録
```cpp
// Player.cpp の OnCollisionEnter() で敵弾との衝突時に呼び出し
justAvoidanceComponent_.RegisterIncomingDamage();
```

敵の弾がプレイヤーに衝突したことを JustAvoidanceComponent に通知します。

#### バレルロール開始の通知
```cpp
// Player.cpp の UpdateBarrelRollAndBoost() でバレルロール開始時に呼び出し
justAvoidanceComponent_.OnBarrelRollStarted();
```

プレイヤーがバレルロールを開始したことを通知します。

#### ジャスト回避の判定
```cpp
bool wasJustAvoidanceSuccess = false;
float boostReward = justAvoidanceComponent_.CheckJustAvoidanceSuccess(wasJustAvoidanceSuccess);

if (wasJustAvoidanceSuccess) {
    // ジャスト回避成功：ボーストゲージを回復
    movementComponent_.AddBoostGaugeReward(boostReward);
} else {
    // ジャスト回避失敗：通常通りダメージ
    TakeDamage(15);
}
```

### 3. パラメータ調整

各パラメータは Player クラスを通じて設定できます：

```cpp
// ジャスト回避ウィンドウサイズを設定（秒）
player->SetJustAvoidanceWindowSize(0.3f);

// 敵の攻撃通知タイムアウトを設定（秒）
player->SetJustAvoidanceTimeout(0.5f);

// ジャスト回避ボーストゲージ報酬を設定
player->SetJustAvoidanceBoostReward(30.0f);
```

### 4. デフォルト値

| パラメータ | デフォルト値 | 説明 |
|-----------|----------|------|
| ウィンドウサイズ | 0.3秒 | バレルロール開始のタイムウィンドウ |
| タイムアウト | 0.5秒 | 敵の攻撃通知の有効期限 |
| ボーストゲージ報酬 | 30.0f | ジャスト回避成功時のゲージ回復量 |

## 仕組みの詳細

### 判定ロジック

1. **敵弾が衝突** → `RegisterIncomingDamage()` で時刻を記録
2. **プレイヤーがバレルロール開始** → `OnBarrelRollStarted()` で時刻を記録
3. **ジャスト回避成功判定**：
   - バレルロール開始時刻と敵弾衝突時刻の差が `justAvoidanceWindowSize_` 以内
   - → **成功**：ダメージなし、ボーストゲージ回復
   - → **失敗**：通常ダメージ

### タイミング

```
敵弾発射 ──→ 敵弾が移動 ──→ [ジャスト回避ウィンドウ (0.3秒)] ──→ 敵弾衝突
                            ↑
                    この間にバレルロール開始で成功
```

## HUD/UI への統合

プレイヤーにジャスト回避の成功状態をフィードバックするために、以下の情報を使用できます：

```cpp
// ジャスト回避ウィンドウ内かどうか
bool inWindow = player->IsInJustAvoidanceWindow();

// ウィンドウの残り時間（秒）
float timeRemaining = player->GetJustAvoidanceWindowTimeRemaining();

// 直近のジャスト回避成功率（0-1、1.0 = 完璧）
float successRate = player->GetJustAvoidanceSuccessRate();
```

これらの情報を使用して：
- UI上にジャスト回避ウィンドウのインジケーター表示
- 成功時のエフェクト再生（パーティクル、フラッシュなど）
- スコア加算やコンボシステムの連携

## バランス調整のヒント

### ウィンドウを広くしたい場合
```cpp
player->SetJustAvoidanceWindowSize(0.4f); // 0.4秒に延長
```
→ より簡単になるが、爽快感が減少

### ウィンドウを狭くしたい場合
```cpp
player->SetJustAvoidanceWindowSize(0.2f); // 0.2秒に短縮
```
→ より難しくなるが、成功時の満足感が増加

### ボーナスゲージを調整したい場合
```cpp
player->SetJustAvoidanceBoostReward(50.0f); // より多く回復
```
→ ジャスト回避の価値をより高める

## 既知の制限事項

- 現在、ジャスト回避はバレルロール中の敵弾衝突のみを対象
- 敵本体との衝突には適用されない
- ImGui表示は未実装（DrawImGui関数で追加可能）

## 将来の拡張案

1. **ビジュアルフィードバック**
   - ジャスト回避成功時のパーティクル演出
   - スクリーンフラッシュエフェクト

2. **サウンド**
   - ジャスト回避成功時のSE

3. **スコアシステム**
   - ジャスト回避成功時のボーナスポイント
   - コンボシステムの実装

4. **他の敵攻撃への対応**
   - ミサイルへのジャスト回避
   - 敵本体衝突への対応

## トラブルシューティング

### ジャスト回避が発動しない場合

1. `RegisterIncomingDamage()` が敵弾衝突時に呼ばれているか確認
2. `OnBarrelRollStarted()` がバレルロール開始時に呼ばれているか確認
3. ウィンドウサイズが適切に設定されているか確認
4. `CheckJustAvoidanceSuccess()` の戻り値を確認

### タイミングが合わせづらい場合

ウィンドウサイズを広げてからプレイして、徐々に狭くしていくことをお勧めします。

```cpp
// 最初は広めで練習
player->SetJustAvoidanceWindowSize(0.5f);

// 慣れてきたら狭める
player->SetJustAvoidanceWindowSize(0.3f);

// さらに上級者向けに
player->SetJustAvoidanceWindowSize(0.2f);
```

## 参考実装ファイル

- `PlayerJustAvoidanceComponent.h` - コンポーネントのヘッダー
- `PlayerJustAvoidanceComponent.cpp` - コンポーネントの実装
- `Player.h` - Player クラスの統合インターフェース
- `Player.cpp` - コンポーネントの統合実装
