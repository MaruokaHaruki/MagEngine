# 敵編隊システム実装ガイド

## 概要
本ドキュメントは、敵の団体行動・編隊行動システムの実装内容を説明します。

## 実装内容

### Phase 1: EnemyGroup クラスの基礎実装 ✅

#### ファイル追加
- `EnemyGroup.h` - 編隊管理クラスのヘッダ
- `EnemyGroup.cpp` - 編隊管理クラスの実装

#### 主要機能
1. **编隊フォーメーション管理**
   - FormationType 列挙型で5つのパターンを定義
   - V字・直線・円形・菱形・動的フォーメーション対応

2. **メンバ敵の管理**
   - リーダー敵とフォロワー敵を区別
   - 最大8敵までのメンバ管理

3. **群動作ロジック (Boid的)**
   - 分離処理（敵同士の衝突回避）
   - 結合処理（集団中心への寄与）
   - 整列処理（敵の向き統一）

### Phase 2: V字編隊の基本実装 ✅

#### Enemy クラスへの拡張
- `groupId_` メンバ追加（属するグループのID）
- `isFollowingFormation_` フラグ（編隊追尾中かどうか）
- `formationTargetPosition_` メンバ（編隊内の目標位置）

#### 新規ステート
```cpp
BehaviorState::FormationFollow
```
編隊内での相対位置追尾用の新しい行動ステート

#### 実装位置
- `Enemy.h:60-94` - グループ関連メソッド追加
- `Enemy.cpp:19-21` - グループメンバ初期化
- `Enemy.cpp:160-166` - FormationFollow ステート処理

### Phase 3: 他の編隊パターン追加 ✅

#### フォーメーション実装一覧
| パターン | クラス | 説明 |
|---------|------|------|
| V字 | FormationType::VFormation | 先頭のリーダーから左右に広がる配置 |
| 直線 | FormationType::LineFormation | 縦一列配置 |
| 円形 | FormationType::CircleFormation | リーダー中心の円形配置 |
| 菱形 | FormationType::DiamondFormation | 上下左右のバランス配置 |
| 動的 | FormationType::DynamicFormation | プレイヤー距離で自動切り替え |

#### 動的フォーメーション切り替え基準
- 距離 < 50m → 円形（全方位対応）
- 距離 50-100m → 菱形（安定性重視）
- 距離 > 100m → V字（機動性重視）

### Phase 4: ウェーブシステムへの統合 ✅

#### EnemyManager への統合
- `groups_` ベクタ追加（編隊管理用）
- `nextGroupId_` メンバ追加（グループID採番用）

#### 編隊生成ロジック（SpawnEnemy メソッド）
1. **Wave 3 以降で編隊生成が開始**
2. **メンバ追加ロジック**
   - 既存グループに追加: 30%の確率
   - グループサイズ: 最大5敵
3. **新規グループ作成ロジック**
   - 新規グループ作成: 20%の確率

#### 更新フロー（EnemyManager::Update）
```cpp
// グループ更新処理
for (auto &group : groups_) {
    if (group && group->IsActive()) {
        group->Update(playerPos);
        group->RemoveDeadMembers();
    }
}
```

## パラメータ設定

### EnemyGroup フォーメーション設定

#### V字フォーメーション（推奨）
```cpp
config.spacing = 30.0f;
config.cohesionStrength = 0.8f;      // 結集度
config.separationStrength = 0.5f;    // 分離強度
config.alignmentStrength = 0.3f;     // 方向整列強度
config.maxMemberCount = 5;
```

相対位置オフセット:
```cpp
offsets[0] = {0, 0, 0}           // リーダー
offsets[1] = {-30, 0, -30}       // 左前
offsets[2] = {30, 0, -30}        // 右前
offsets[3] = {-50, 0, -50}       // 左後ろ
offsets[4] = {50, 0, -50}        // 右後ろ
```

#### 直線フォーメーション
```cpp
config.spacing = 25.0f;
config.cohesionStrength = 0.7f;
config.separationStrength = 0.4f;
config.alignmentStrength = 0.4f;
config.maxMemberCount = 8;
```

#### 円形フォーメーション
```cpp
config.spacing = 35.0f;
config.cohesionStrength = 0.6f;
config.separationStrength = 0.6f;
config.alignmentStrength = 0.2f;
config.maxMemberCount = 6;
```

## ウェーブ設定

### Wave ごとの編隊割合（推奨）

| Wave | 編隊敵数 | 単独敵数 | 構成 |
|------|--------|--------|------|
| Wave 1-2 | 0% | 100% | 既存の単独敵 |
| Wave 3 | 20-30% | 70-80% | 編隊導入 |
| Wave 4 | 40-50% | 50-60% | 編隊増加 |
| Wave 5 | 60-70% | 30-40% | 編隊主体 |

## ファイル修正一覧

### 既存ファイルへの変更

#### Enemy.h
- L60-68: グループID関連メソッド追加
- L69-73: 編隊追尾関連メソッド追加
- L74-76: グループメンバ追加
- L77-84: FormationFollow ステート追加

#### Enemy.cpp
- L19-21: groupId_, isFollowingFormation_, formationTargetPosition_ 初期化
- L160-166: FormationFollow ステート処理追加

#### EnemyGunner.h / EnemyGunner.cpp
- groupId_ メンバ追加
- SetGroupId/GetGroupId メソッド追加

#### EnemyManager.h
- 前方宣言に `class EnemyGroup` 追加
- `groups_` ベクタ追加
- `nextGroupId_` メンバ追加

#### EnemyManager.cpp
- L6: `#include "EnemyGroup.h"` 追加
- L36: nextGroupId_ 初期化
- L77: グループ更新処理追加
- L154-157: Clear メソッドにグループクリア追加
- L223-265: SpawnEnemy メソッド拡張（編隊生成ロジック）

### プロジェクトファイルへの追加

#### MagEngine.vcxproj
- L121: `<ClCompile Include="application\enemy\EnemyGroup.cpp" />`
- L214: `<ClInclude Include="application\enemy\EnemyGroup.h" />`

#### MagEngine.vcxproj.filters
- 編隊管理用フィルタ追加

## 使用方法

### 編隊の自動生成
Wave 3 以降をプレイすると、敵のスポーン時に自動的に編隊が生成されます。

生成確率:
- 既存グループへのメンバ追加: 30%
- 新規グループ作成: 20%
- 単独敵: 50%

### 編隊の動作
1. リーダー敵がプレイヤーに接近
2. フォロワー敵がリーダーを中心とした編隊フォーメーションに配置
3. 群動作ロジックで位置を調整
4. リーダー敵が Combat → Dash → Retreat と遷移
5. フォロワーも同期して行動

## パラメータ調整ガイド

### 編隊の結集度を上げたい場合
```cpp
cohesionStrength の値を上げる (0.6 → 0.8など)
separationStrength の値を下げる (0.5 → 0.3など)
```

### 敵同士の距離を広げたい場合
```cpp
spacing の値を増加させる (30 → 40など)
config.offsets[] の値を調整
```

### 編隊内での敵の独立性を高めたい場合
```cpp
cohesionStrength を下げる
separationStrength を上げる
alignmentStrength を下げる
```

## 今後の拡張案

### 追加可能な機能
1. **編隊内での役割分け**
   - リーダー: 前方探索
   - フランカー: 側面
   - リアガード: 後方支援

2. **複雑な編隊戦術**
   - 二段階編成（複数グループの連携）
   - 囲い込み戦術

3. **敵タイプ別の編隊**
   - ガンナー編隊（射撃を中心）
   - 近接敵編隊（体当たりを中心）
   - 混成編隊

4. **視覚効果**
   - 編隊内の接続線表示
   - 編隊フォーメーション変更時のエフェクト

5. **音声設計**
   - 編隊の展開/撤退音
   - 敵同士の連携音

## トラブルシューティング

### 編隊が生成されない
- Wave 3 以降をプレイしているか確認
- nextGroupId_ が正しく初期化されているか確認

### 敵が変な位置に移動する
- formationTargetPosition_ が正しく設定されているか確認
- MoveToward() の速度が適切か確認

### 敵同士が衝突する
- separationStrength を上げる
- spacing パラメータを調整

## まとめ

本実装により、敵システムに以下の改善がもたらされます:

✅ **戦術的な深さ**: 単独敵vs編隊敵で異なる対策が必要
✅ **視覚的な興味**: 複数敵の協調行動
✅ **難易度調整**: Wave を通じた段階的な難易度上昇
✅ **拡張性**: 新しいフォーメーションパターンの追加が容易

