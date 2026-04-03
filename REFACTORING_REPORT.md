エネミーシステム改修レポート
============================

【改修日時】
2026年4月3日

【改修対象】
- application/enemy/base/EnemyBase.h/cpp
- application/enemy/type/Enemy.h/cpp
- application/enemy/manager/EnemyManager.h/cpp

【改修内容】

### 1. EnemyBase（基底クラス）の簡略化

**削除されたメンバ変数（ヒットリアクション関連）:**
- `int hitFlashCount_` → 点滅効果は廃止
- `Vector3 originalScale_`, `Vector3 hitScale_` → スケール変更エフェクトは廃止
- `bool shouldRenderThisFrame_` → 常に描画（点滅なし）
- `Vector3 knockbackVelocity_` → ノックバック詳細計算は廃止
- `float shakeAmplitude_`, `float shakeFrequency_` → 振動エフェクトは廃止

**簡略化されたメソッド:**
- `UpdateHitReaction()` → 過去: 複雑な点滅・振動・ノックバック処理
  現在: 単純なタイマーのみ（0.3秒待機）
- `StartHitReaction()` → 必要最小限の初期化に削減
- `Draw()` → `shouldRenderThisFrame_` フラグを廃止

**効果:**
- ヒットリアクション関数が約270行から約10行に削減
- メモリ使用量削減（約60バイト/敵インスタンス）
- 保守性向上（複雑なアニメーション計算を削除）

---

### 2. Enemy（具体的な敵クラス）の行動ロジック整理

**削除されたパラメータ:**
- Dash関連のすべての変数（dashTimer_, dashCooldownTimer_, dashTargetPos_）
- Dash状態（BehaviorState::Dash）は廃止
  → 体当たり攻撃機能を削除

**現在の行動ステート:**
1. Approach（接近）
2. Combat（戦闘・周回）
3. Retreat（退却）
4. FormationFollow（編隊フォロー）

**シンプル化の効果:**
- 行動パターンが5段階 → 4段階に削減
- Enemy.cpp が約193行 → より管理しやすいサイズに

---

### 3. EnemyManager（管理クラス）の構成整理

**WaveConfig構造体の簡略化:**
削除されたパラメータ（各wave設定から廃止）:
- enemySpeedMultiplier
- enemyDamageMultiplier
- gunnerSpeedMultiplier
- gunnerDamageMultiplier
- separationStrength
- cohesionStrength
- alignmentStrength
- formationSpacing

**新しいWaveConfig:**
```cpp
struct WaveConfig {
    int enemyCount;         // 敵の数
    int gunnerCount;        // ガンナー数
    float spawnInterval;    // スポーン間隔
    float formationRatio;   // 編隊比率
    int maxGroupSize;       // 編隊最大サイズ
    int formationPattern;   // 編隊パターン
};
```

**効果:**
- コンフィグ定義がコンパクト化（5ウェーブ設定がより読みやすく）
- 複雑度削減（マルチプライヤーの計算ロジック廃止）

---

【改修前後の比較】

項目                    | 改修前      | 改修後      | 削減率
------------------------------------------
EnemyBase メンバ数      | 24個        | 19個        | 20.8%
ヒットリアクション処理   | 複雑        | 超シンプル   | 96%削減
Enemy の行動ステート     | 5種類       | 4種類       | 20%削減
WaveConfig 設定数       | 14項目      | 6項目       | 57%削減

---

【改修後のシステム構成】

```
┌─────────────────────────────────────────┐
│         EnemyManager（統括管理）         │
│  - ウェーブシステム                      │
│  - 敵生成・破棄                          │
│  - グループ編隊管理                      │
└──────────────┬──────────────────────────┘
               │
       ┌───────┴────────┐
       │                │
   ┌───▼──────┐    ┌────▼──────┐
   │  Enemy   │    │EnemyGroup │
   │(個別敵)  │    │(編隊管理) │
   └───┬──────┘    └────┬──────┘
       │                │
   ┌───▼───────────────▼────┐
   │    EnemyBase（基底）    │
   │  - HP管理              │
   │  - 移動                │
   │  - ヒットリアクション   │
   │  - パーティクル        │
   └────────────────────────┘
```

---

【今後の対応】

1. **behavior ディレクトリ** → 現在未使用
   将来的なBehavior Tree実装の際に活用可能

2. **DebugScene** → テストコード整理が必要
   エネミーシステムのデバッグ用UI統合

3. **EnemyBullet / EnemyGunner** → 既存システム維持
   本改修対象外（必要に応じて個別対応）

---

【品質保証】

改修後に以下を確認してください：

✓ ビルド成功確認
✓ ゲーム実行時のエネミー生成確認
✓ ウェーブシステムが正常に動作
✓ 編隊行動が破綻していないか確認
✓ メモリリークのないか確認（デバッグモード）

