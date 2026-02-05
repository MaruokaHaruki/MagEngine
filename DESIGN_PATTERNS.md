# MagEngine デザインパターン分析

## 概要
MagEngine プロジェクトに実装されているデザインパターンを分析・文書化しています。
State パターンとシングルトンパターン以外のパターンについて記載しています。

---

## 1. Factory パターン

### 採用箇所
- **クラス名**: `AbstractSceneFactory`, `SceneFactory`
- **ファイル**: 
  - [scene/base/AbstractSceneFactory.h](scene/base/AbstractSceneFactory.h)
  - [scene/base/SceneFactory.h](scene/base/SceneFactory.h)
  - [scene/base/SceneFactory.cpp](scene/base/SceneFactory.cpp)

### 実装内容
```cpp
// 抽象ファクトリクラス
class AbstractSceneFactory {
public:
    virtual ~AbstractSceneFactory() = default;
    virtual std::unique_ptr<BaseScene> CreateScene(int sceneNo) = 0;
};

// 具体的なファクトリ実装
class SceneFactory : public AbstractSceneFactory {
public:
    std::unique_ptr<BaseScene> CreateScene(int sceneNo) override;
};
```

### 目的
- **シーン生成の一元化**: 複数のシーン（TitleScene, GamePlayScene, DebugScene, ClearScene）の生成を一箇所で管理
- **シーン番号に基づく動的生成**: 整数値から対応するシーンオブジェクトを自動生成
- **疎結合性**: [SceneManager](scene/base/SceneManager.h) がシーン生成の詳細に依存しない設計を実現
- **拡張性**: 新しいシーンを追加する際、`CreateScene()` メソッドのみ修正すれば対応可能

### 使用例
```cpp
// SceneManager内での使用
nowScene_ = sceneFactory_->CreateScene(currentSceneNo_);
```

---

## 2. Component パターン（コンポーネント設計）

### 採用箇所
- **主クラス**: [Player.h](application/player/Player.h)
- **コンポーネント群**:
  - [PlayerMovementComponent.h](application/player/component/PlayerMovementComponent.h) - 移動処理
  - [PlayerHealthComponent.h](application/player/component/PlayerHealthComponent.h) - HP管理
  - [PlayerCombatComponent.h](application/player/component/PlayerCombatComponent.h) - 戦闘管理
  - [PlayerDefeatComponent.h](application/player/component/PlayerDefeatComponent.h) - 敗北処理

### 実装内容
Player クラスは複数のコンポーネントを所有し、各機能を分割

```cpp
class Player : public BaseObject {
private:
    PlayerMovementComponent movementComponent_;  // 移動関連
    PlayerHealthComponent healthComponent_;      // HP・ダメージ管理
    PlayerCombatComponent combatComponent_;      // 戦闘・射撃管理
    PlayerDefeatComponent defeatComponent_;      // 敗北処理
};
```

### 目的
- **責任分離**: プレイヤーの複雑な機能を独立した小さなコンポーネントに分割
- **関心の分離**: 移動、体力、戦闘、敗北処理などが独立して管理可能
- **再利用性**: 各コンポーネントを他のキャラクターにも適用可能
- **テストの容易性**: 個別のコンポーネントを単体テスト可能
- **保守性向上**: 各機能の変更が他の部分に影響を与えにくい設計

### 利点
- Player クラスの複雑性を低減
- 各コンポーネントが単一の責任を持つ（単一責任の原則）
- Initialize(), Update(), Draw() の処理を各コンポーネントで個別管理

---

## 3. Observer パターン（Callback/イベント駆動）

### 採用箇所
- **実装クラス**:
  - [SceneTransition.h](application/SceneTransition.h) - シーン遷移完了時
  - [GameOverUI.h](application/ui/GameOverUI.h) - ゲームオーバーUI完了時
  - [StartAnimation.h](application/StartAnimation.h) - 開始アニメーション完了時
  - [GameClearAnimation.h](application/GameClearAnimation.h) - クリアアニメーション完了時

### 実装内容
`std::function` を使用したコールバック機能

```cpp
class SceneTransition {
public:
    void SetOnCompleteCallback(std::function<void()> callback) {
        onCompleteCallback_ = callback;
    }
    
private:
    std::function<void()> onCompleteCallback_ = nullptr;
    
    // 完了時に呼び出し
    if (onCompleteCallback_) {
        onCompleteCallback_();
    }
};
```

### 使用例
```cpp
// TitleScene.cpp での使用
sceneTransition_->SetOnCompleteCallback([this]() {
    currentSceneNo_ = 1;  // GamePlayScene へ遷移
});
```

### 目的
- **疎結合性**: シーン遷移やアニメーション完了後の処理を呼び出し側に委譲
- **イベント駆動設計**: 特定の状態変化に応じて、任意の処理を実行可能
- **柔軟性**: 完了後の処理を動的に定義できる
- **責任分離**: アニメーション側は「完了を通知する」のみで、その後の処理は呼び出し側で決定

---

## 4. Strategy パターン（戦略パターン）

### 採用箇所
- **ファイル**: [ENGINE_DESIGN_DOCUMENT.md](ENGINE_DESIGN_DOCUMENT.md#L1569)
- **対象**: PostEffect システム（ポストエフェクト管理）
- **実装クラス群**:
  - [GrayscaleEffect.h](engine/postEffect/grayscale/GrayscaleEffect.h)
  - [Vignetting.h](engine/postEffect/Vignetting.h)
  - [PostEffectManager.h](engine/postEffect/PostEffectManager.h)

### 実装内容
異なるエフェクト実装を抽象化した戦略パターン

```cpp
// エフェクトタイプの列挙
enum class EffectType {
    Grayscale,
    Vignetting,
    // その他のエフェクト...
};

// PostEffectManager内でエフェクト戦略を切り替え
std::vector<EffectType> effectChain_;  // 適用するエフェクトのチェーン
```

### 目的
- **エフェクトの動的切り替え**: 実行時にエフェクトを選択・組み合わせ可能
- **拡張性**: 新しいエフェクトを追加する際、既存コードへの変更を最小化
- **再利用性**: 同じエフェクトを異なるシーンで再利用可能
- **複合エフェクト**: 複数のエフェクトを組み合わせてエフェクトチェーンを構築

---

## 5. Template Method パターン

### 採用箇所
- **ベースクラス**: [BaseScene.h](scene/base/BaseScene.h)
- **実装クラス群**:
  - [TitleScene.h](scene/publicScene/TitleScene.h)
  - [GamePlayScene.h](scene/publicScene/GamePlayScene.h)
  - [DebugScene.h](scene/privateScene/DebugScene.h)

### 実装内容
```cpp
class BaseScene {
public:
    // テンプレートメソッド
    virtual void Initialize() = 0;
    virtual void Finalize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
    virtual void DrawImGui() = 0;
};
```

### 目的
- **標準化**: すべてのシーンが同じ初期化→更新→描画のライフサイクルを持つ
- **一貫性**: シーン遷移時の処理フロー（初期化→更新→描画→終了化）を統一
- **フレームワーク設計**: SceneManager がシーンの詳細を知らずに、統一インタフェースで操作
- **ポリモーフィズム**: 各シーンが固有の処理を実装しつつ、全体構造を保持

---

## 6. Composite パターン（可能性）

### 採用箇所
- **実装**: UI要素の階層構造管理
- **管理クラス**: [UIManager.h](application/ui/UIManager.h)
- **関連クラス群**:
  - [GameOverUI.h](application/ui/GameOverUI.h)
  - [HUD.h](application/ui/HUD.h)
  - [MenuUI.h](application/ui/MenuUI.h)

### 実装内容
複数のUI要素を一括管理する構造

```cpp
class UIManager {
private:
    std::unique_ptr<GameOverUI> gameOverUI_;
    std::unique_ptr<HUD> hud_;
    std::unique_ptr<MenuUI> menuUI_;
    // その他のUI要素...
};
```

### 目的
- **UI要素の一元管理**: すべてのUI要素の生成、更新、描画を集中管理
- **階層的構造**: UI要素を親子関係で管理可能
- **統一インタフェース**: 個別のUIと複数UIの操作を同じインタフェースで処理

---

## 7. Object Pool パターン（潜在的実装）

### 採用箇所
- **実装クラス**: [EnemyManager.h](application/enemy/EnemyManager.h)
- **関連クラス**: [EnemyGunner.h](application/enemy/EnemyGunner.h)

### 実装内容
敵オブジェクトのプール管理

```cpp
class EnemyManager {
private:
    std::vector<std::unique_ptr<EnemyBase>> enemies_;
    // 敵オブジェクトのプール
};
```

### 目的
- **パフォーマンス最適化**: メモリ割当解放のオーバーヘッドを削減
- **敵管理の一元化**: 複数の敵の生成・更新・描画を効率的に管理
- **ガベージコレクション削減**: C++の std::unique_ptr で自動メモリ管理

---

## デザインパターン適用の利点

| パターン | 主な利点 |
|---------|---------|
| **Factory** | シーン生成の一元化、拡張性向上 |
| **Component** | 責任分離、再利用性、テスト容易性 |
| **Observer** | イベント駆動設計、疎結合性 |
| **Strategy** | エフェクト管理の柔軟性、拡張性 |
| **Template Method** | 処理フロー の統一化、ポリモーフィズム |
| **Composite** | UI要素の統一管理 |
| **Object Pool** | メモリ効率化、パフォーマンス最適化 |

---

## まとめ

MagEngine は以下のデザインパターンを効果的に組み合わせて実装されています：

1. **構造的パターン**: Component, Composite
2. **生成的パターン**: Factory, Object Pool
3. **振る舞いパターン**: Observer (Callback), Strategy, Template Method

これらのパターンにより、以下を実現しています：
- ✅ 高い保守性と拡張性
- ✅ コンポーネント間の低い結合度
- ✅ 責任の明確な分離
- ✅ 新機能の追加が容易な設計
