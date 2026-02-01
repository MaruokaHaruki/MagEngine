# 動的雲システム実装ガイド

## 概要
弾丸が雲を突き抜け、穴が空いたり変形したりする動的効果を実装しました。

## 実装済み機能

### 1. Cloud クラスの拡張
- **ImpactPoint 構造体** - 影響ポイント情報の管理
  - `position`: ワールド座標
  - `radius`: 影響半径
  - `strength`: 影響強度 (0.0～1.0)
  - `elapsedTime`: 経過時間
  - `lifeTime`: 存在時間（秒）
  
- **Cloud::AddImpact()** メソッド
  ```cpp
  void AddImpact(const MagMath::Vector3 &position, 
                 float radius, 
                 float strength, 
                 float lifeTime = 1.0f);
  ```
  弾丸や爆風が雲に衝突したときに呼び出す

### 2. シェーダー側の対応
- **Cloud.hlsli** - 影響ポイント定数バッファ追加
  - `gImpactPointCount`: アクティブな影響ポイント数
  - `gImpactInfluence`: 全体の強度倍率
  
- **Cloud.PS.hlsl** - レイマーチング時に影響を適用
  - `CalculateImpactEffect()` でポイント効果を計算
  - 密度減少処理で雲が薄くなる

## 使用方法

### 弾丸衝突時に雲に影響を追加

**PlayerBullet.cpp** の `OnCollisionEnter()` に以下を追加:

```cpp
void PlayerBullet::OnCollisionEnter(BaseObject *other) {
    // 他のオブジェクトと衝突した場合
    SetDead();
    
    // ★ 雲に影響を追加 ★
    // GamePlaySceneの雲インスタンスへアクセスする必要があります
    // 方法：GamePlaySceneをシングルトンなどで参照するか、
    //      ポインタを渡す設計に変更
    
    // 例：
    // if (Cloud *cloud = GetGlobalCloud()) {
    //     cloud->AddImpact(
    //         GetPosition(),      // 衝撃の位置
    //         30.0f,              // 影響半径
    //         0.8f,               // 影響強度
    //         1.0f                // 1秒かけて復帰
    //     );
    // }
}
```

## 実装ステップ

### ステップ 1: Cloud へのアクセス設定
GamePlayScene で cloud_ をシングルトンまたは静的にアクセス可能にします：

```cpp
// GamePlayScene.h に追加
static Cloud* GetInstance() { return instancePtr_; }
static Cloud* instancePtr_ = nullptr;

// GamePlayScene::Initialize() に追加
GamePlayScene::instancePtr_ = cloud_.get();
```

### ステップ 2: 弾丸衝突時に影響を追加

**PlayerBullet.cpp** の `OnCollisionEnter()` を修正：

```cpp
void PlayerBullet::OnCollisionEnter(BaseObject *other) {
    SetDead();
    
    // 雲に影響を追加
    if (Cloud *cloud = GamePlayScene::GetInstance()) {
        cloud->AddImpact(
            GetPosition(),
            30.0f,      // 弾丸サイズに応じて調整
            0.7f,       // 0.0～1.0
            1.2f        // 復帰時間（秒）
        );
    }
}
```

### ステップ 3: 敵の弾も同様に対応

**EnemyBullet.cpp** の `OnCollisionEnter()` を修正：

```cpp
void EnemyBullet::OnCollisionEnter(BaseObject *other) {
    SetDead();
    
    if (Cloud *cloud = GamePlayScene::GetInstance()) {
        cloud->AddImpact(
            GetPosition(),
            25.0f,
            0.6f,
            0.8f
        );
    }
}
```

## パラメータ調整ガイド

### radius (影響半径)
- **20～30**: 小さい弾丸、細い空洞
- **40～60**: 通常の弾丸
- **80～150**: 大きな爆発

### strength (影響強度)
- **0.3～0.5**: 薄い効果、すぐに戻る
- **0.6～0.8**: 標準的な効果
- **0.9～1.0**: 完全に消失

### lifeTime (復帰時間)
- **0.5～0.8**: 素早く復帰（爽快感）
- **1.0～1.5**: 自然な復帰
- **2.0～3.0**: ゆっくり復帰（dramatic効果）

## パフォーマンス考慮事項

- **MAX_IMPACT_POINTS = 16** - 最大同時影響ポイント数
  - CPU側で自動的に古いものを削除
  - シェーダー側で効率的に処理

- 影響ポイントがアクティブな間のみシェーダーで計算
- GPU側で快速な計算で高速処理

## トラブルシューティング

### 雲の穴が見えない
1. `gImpactInfluence` が 0.0 になっていないか確認
2. 弾丸の `lifeTime` が短すぎないか確認
3. クラウドパラメータで `density` が高すぎないか確認

### パフォーマンス低下
1. `MAX_IMPACT_POINTS` を減らす（8～12程度）
2. 弾丸の `radius` を小さくする
3. クラウドの `stepSize` を大きくする

## 今後の拡張案

1. **流体シミュレーション**: StructuredBuffer を使用した高度な物理計算
2. **ウェザーマップ連携**: 影響ポイント情報をテクスチャに焼き込み
3. **VFX連携**: 雲の穴の位置にパーティクルを生成
4. **音声連携**: 穴の大きさに応じた効果音
