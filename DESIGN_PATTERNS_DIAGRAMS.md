# MagEngine デザインパターン クラス図

このドキュメントは、MagEngine に実装されているデザインパターンをマーメイド記法で表現したクラス図をまとめたものです。

---

## 1. Factory パターン - シーン生成

```mermaid
classDiagram
    class AbstractSceneFactory {
        <<abstract>>
        +CreateScene(int sceneNo) BaseScene*
    }
    
    class SceneFactory {
        +CreateScene(int sceneNo) BaseScene*
    }
    
    class BaseScene {
        <<abstract>>
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
    }
    
    class TitleScene {
        +Initialize()
        +Update()
        +Draw()
    }
    
    class GamePlayScene {
        +Initialize()
        +Update()
        +Draw()
    }
    
    class DebugScene {
        +Initialize()
        +Update()
        +Draw()
    }
    
    class SceneManager {
        -sceneFactory_: AbstractSceneFactory*
        -nowScene_: unique_ptr~BaseScene~
        +SetSceneFactory(factory: AbstractSceneFactory*)
        +ChangeScene(sceneNo: int)
    }
    
    AbstractSceneFactory <|-- SceneFactory
    BaseScene <|-- TitleScene
    BaseScene <|-- GamePlayScene
    BaseScene <|-- DebugScene
    SceneFactory --> BaseScene : creates
    SceneManager --> AbstractSceneFactory : uses
    SceneManager --> BaseScene : manages
```

**目的**:
- シーン生成の一元化
- 新しいシーンスの追加が容易
- SceneManager がシーン生成の詳細に依存しない設計

---

## 2. Component パターン - プレイヤー機能分割

```mermaid
classDiagram
    class Player {
        -movementComponent_: PlayerMovementComponent
        -healthComponent_: PlayerHealthComponent
        -combatComponent_: PlayerCombatComponent
        -defeatComponent_: PlayerDefeatComponent
        +Initialize()
        +Update()
        +Draw()
        +TakeDamage(damage: int)
        +GetHealth(): int
    }
    
    class PlayerMovementComponent {
        -moveSpeed_: float
        -boostGauge_: float
        -isBarrelRolling_: bool
        +Initialize()
        +Update(transform: Transform*)
        +ProcessInput(x: float, y: float)
        +StartBarrelRoll(isRight: bool)
        +ProcessBoost(input: bool)
    }
    
    class PlayerHealthComponent {
        -currentHP_: int
        -maxHP_: int
        -isInvincible_: bool
        +Initialize(maxHP: int)
        +Update(deltaTime: float)
        +TakeDamage(damage: int)
        +Heal(amount: int)
        +IsAlive(): bool
    }
    
    class PlayerCombatComponent {
        -bullets_: vector~unique_ptr~PlayerBullet~~
        -missiles_: vector~unique_ptr~PlayerMissile~~
        -shootCoolTime_: float
        +Initialize(object3dSetup)
        +Update(deltaTime: float)
        +ShootBullet(pos: Vector3, dir: Vector3)
        +ShootMissile(pos: Vector3, dir: Vector3, target: Enemy*)
    }
    
    class PlayerDefeatComponent {
        -isDefeated_: bool
        +Initialize()
        +OnDefeat()
    }
    
    Player *-- PlayerMovementComponent
    Player *-- PlayerHealthComponent
    Player *-- PlayerCombatComponent
    Player *-- PlayerDefeatComponent
```

**目的**:
- 複雑な Player クラスの責任分離
- 各機能の独立管理と再利用性向上
- 個別コンポーネントの単体テスト容易化
- 単一責任の原則に従った設計

---

## 3. Observer パターン - コールバック/イベント駆動

```mermaid
classDiagram
    class SceneTransition {
        -state_: TransitionState
        -onCompleteCallback_: function~void()~
        +Initialize(spriteSetup: SpriteSetup*)
        +Update()
        +Draw()
        +StartClosing(type: TransitionType)
        +StartOpening(type: TransitionType)
        +SetOnCompleteCallback(callback: function)
    }
    
    class GameOverUI {
        -state_: GameOverState
        -onCompleteCallback_: function~void()~
        +Initialize()
        +Update()
        +Draw()
        +StartGameOver(fadeDuration: float)
        +SetOnCompleteCallback(callback: function)
    }
    
    class StartAnimation {
        -state_: StartAnimationState
        -onCompleteCallback_: function~void()~
        +Initialize()
        +Update()
        +Draw()
        +StartAnimation()
        +SetOnCompleteCallback(callback: function)
    }
    
    class GameClearAnimation {
        -state_: GameClearAnimationState
        -onCompleteCallback_: function~void()~
        +Initialize()
        +Update()
        +Draw()
        +StartClearAnimation()
        +SetOnCompleteCallback(callback: function)
    }
    
    class TitleScene {
        +Initialize()
        +Update()
        +OnStartAnimationComplete()
    }
    
    class GamePlayScene {
        +Initialize()
        +Update()
        +OnGameOverComplete()
        +OnStartAnimationComplete()
    }
    
    TitleScene --> SceneTransition : sets callback
    TitleScene --> StartAnimation : sets callback
    GamePlayScene --> GameOverUI : sets callback
    GamePlayScene --> GameClearAnimation : sets callback
    GamePlayScene --> SceneTransition : sets callback
```

**目的**:
- イベント駆動の設計実現
- イベント発生者とリスナーの疎結合化
- 完了後の処理を動的に定義可能
- シーン遷移やアニメーション完了時の処理を委譲

---

## 4. Template Method パターン - シーンのライフサイクル

```mermaid
classDiagram
    class BaseScene {
        <<abstract>>
        #sceneNo_: int
        +Initialize()*
        +Update()*
        +Draw()*
        +Finalize()*
        +DrawImGui()*
    }
    
    class TitleScene {
        -player_: unique_ptr~Player~
        -titleCamera_: unique_ptr~TitleCamera~
        -titleSprite_: unique_ptr~Sprite~
        -sceneTransition_: unique_ptr~SceneTransition~
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
        +DrawImGui()
    }
    
    class GamePlayScene {
        -player_: unique_ptr~Player~
        -enemyManager_: unique_ptr~EnemyManager~
        -collisionManager_: unique_ptr~CollisionManager~
        -followCamera_: unique_ptr~FollowCamera~
        -uiManager_: unique_ptr~UIManager~
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
        +DrawImGui()
    }
    
    class DebugScene {
        -objMonsterBall_: unique_ptr~Object3d~
        -objTerrain_: unique_ptr~Object3d~
        -particle_: unique_ptr~Particle~
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
        +DrawImGui()
    }
    
    class ClearScene {
        -player_: unique_ptr~Player~
        -clearAnimation_: unique_ptr~GameClearAnimation~
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
        +DrawImGui()
    }
    
    class SceneManager {
        -sceneFactory_: AbstractSceneFactory*
        -nowScene_: unique_ptr~BaseScene~
        +Update()
        +Draw()
        -CallLifecycleMethods()
    }
    
    BaseScene <|-- TitleScene
    BaseScene <|-- GamePlayScene
    BaseScene <|-- DebugScene
    BaseScene <|-- ClearScene
    SceneManager --> BaseScene : uses template methods
    SceneManager --> AbstractSceneFactory : creates with
```

**目的**:
- すべてのシーンに統一したライフサイクルを適用
- シーン遷移時の処理フロー（初期化→更新→描画→終了化）を標準化
- SceneManager がシーン詳細を知らずに統一インタフェースで操作可能
- ポリモーフィズムを活用した柔軟な設計

---

## 5. Composite パターン - UI要素の一元管理

```mermaid
classDiagram
    class UIManager {
        -gameOverUI_: unique_ptr~GameOverUI~
        -hud_: unique_ptr~HUD~
        -menuUI_: unique_ptr~MenuUI~
        -operationGuideUI_: unique_ptr~OperationGuideUI~
        +Initialize(spriteSetup)
        +Update()
        +Draw()
        +Finalize()
    }
    
    class GameOverUI {
        -state_: GameOverState
        -backgroundSprite_: unique_ptr~Sprite~
        -textSprite_: unique_ptr~Sprite~
        +Initialize()
        +Update()
        +Draw()
        +StartGameOver()
    }
    
    class HUD {
        -followCamera_: FollowCamera*
        -currentPlayer_: Player*
        +Initialize()
        +Update(player: Player*)
        +Draw()
        -DrawBoresight()
        -DrawRadarAltitude()
    }
    
    class MenuUI {
        -menuButtons_: vector~MenuButton~
        +Initialize()
        +Update()
        +Draw()
        -DrawButtons()
    }
    
    class OperationGuideUI {
        -controllerButtons_: vector~ControllerButton~
        +Initialize()
        +Update()
        +Draw()
        -DrawGuide()
    }
    
    class Sprite {
        +Initialize(setup)
        +Draw()
    }
    
    UIManager *-- GameOverUI
    UIManager *-- HUD
    UIManager *-- MenuUI
    UIManager *-- OperationGuideUI
    GameOverUI *-- Sprite
    HUD *-- Sprite
    MenuUI *-- Sprite
    OperationGuideUI *-- Sprite
```

**目的**:
- UI要素の一元管理
- UI要素の階層的構造管理
- 個別UIと複数UIの操作を同じインタフェースで処理可能
- UI更新・描画の統一化

---

## 6. Strategy パターン - ポストエフェクト管理

```mermaid
classDiagram
    class PostEffectManager {
        -grayscaleEffect_: unique_ptr~GrayscaleEffect~
        -vignetting_: unique_ptr~Vignetting~
        -effectChain_: vector~EffectType~
        +Initialize()
        +Update()
        +Draw()
        +ApplyEffect(type: EffectType)
        +RemoveEffect(type: EffectType)
    }
    
    class PostEffect {
        <<abstract>>
        +Initialize()*
        +Update()*
        +Draw()*
        +Apply()*
    }
    
    class GrayscaleEffect {
        -intensity_: float
        +Initialize()
        +Update()
        +Draw()
        +Apply()
        +SetIntensity(value: float)
    }
    
    class Vignetting {
        -intensity_: float
        -radius_: float
        +Initialize()
        +Update()
        +Draw()
        +Apply()
        +SetIntensity(value: float)
    }
    
    enum EffectType {
        Grayscale
        Vignetting
    }
    
    PostEffect <|-- GrayscaleEffect
    PostEffect <|-- Vignetting
    PostEffectManager --> PostEffect : manages
    PostEffectManager --> EffectType : uses
```

**目的**:
- エフェクトの動的切り替え
- 新しいエフェクト追加時に既存コード変更を最小化
- 複数のエフェクトを組み合わせて使用可能
- エフェクトの再利用性向上

---

## 7. Object Pool パターン - 敵管理システム

```mermaid
classDiagram
    class EnemyManager {
        -enemies_: vector~unique_ptr~EnemyBase~~
        -levelDataLoader_: unique_ptr~LevelDataLoader~
        +Initialize()
        +Update()
        +Draw()
        +Finalize()
        +SpawnEnemy(type: int, pos: Vector3)
        +GetEnemies(): vector~EnemyBase*~
    }
    
    class EnemyBase {
        <<abstract>>
        #transform_: Transform
        #health_: int
        +Initialize()*
        +Update()*
        +Draw()*
        +TakeDamage(damage: int)*
    }
    
    class EnemyGunner {
        -shootCoolTime_: float
        -bulletSpeed_: float
        +Initialize()
        +Update()
        +Draw()
        +TakeDamage(damage: int)
        +ShootBullet()
    }
    
    class EnemyBullet {
        -velocity_: Vector3
        -lifetime_: float
        +Initialize()
        +Update()
        +Draw()
        +IsAlive(): bool
    }
    
    class LevelDataLoader {
        +Initialize()
        +LoadLevelData(filePath: string)
        +GetEnemyData(): vector~EnemySpawnData~
    }
    
    EnemyBase <|-- EnemyGunner
    EnemyManager *-- EnemyBase
    EnemyManager --> EnemyBullet : manages bullets
    EnemyManager *-- LevelDataLoader : uses for loading
```

**目的**:
- メモリ割当解放のオーバーヘッド削減
- 敵オブジェクトの効率的な一括管理
- 敵の生成・更新・描画を集中管理
- パフォーマンス最適化

---

## 8. MagEngine デザインパターン全体構成

```mermaid
graph TB
    subgraph Factory["🏭 Factory パターン"]
        direction LR
        AF["AbstractSceneFactory"]
        SF["SceneFactory"]
        AF -->|implements| SF
    end
    
    subgraph Component["🧩 Component パターン"]
        direction LR
        P["Player"]
        PMC["PlayerMovementComponent"]
        PHC["PlayerHealthComponent"]
        PCC["PlayerCombatComponent"]
        P -->|aggregates| PMC
        P -->|aggregates| PHC
        P -->|aggregates| PCC
    end
    
    subgraph Observer["📢 Observer パターン"]
        direction LR
        CB["SetOnCompleteCallback"]
        ST["SceneTransition"]
        GUI["GameOverUI"]
        SA["StartAnimation"]
        GCA["GameClearAnimation"]
        CB -->|used by| ST
        CB -->|used by| GUI
        CB -->|used by| SA
        CB -->|used by| GCA
    end
    
    subgraph TemplateMethod["📋 Template Method パターン"]
        direction LR
        BS["BaseScene"]
        TS["TitleScene"]
        GPS["GamePlayScene"]
        DS["DebugScene"]
        BS -->|abstracts| TS
        BS -->|abstracts| GPS
        BS -->|abstracts| DS
    end
    
    subgraph Composite["🎨 Composite パターン"]
        direction LR
        UIM["UIManager"]
        HUD["HUD"]
        GUI2["GameOverUI"]
        MU["MenuUI"]
        UIM -->|manages| HUD
        UIM -->|manages| GUI2
        UIM -->|manages| MU
    end
    
    subgraph Strategy["⚙️ Strategy パターン"]
        direction LR
        PEM["PostEffectManager"]
        GE["GrayscaleEffect"]
        VG["Vignetting"]
        PEM -->|applies| GE
        PEM -->|applies| VG
    end
    
    subgraph ObjectPool["🏊 Object Pool パターン"]
        direction LR
        EM["EnemyManager"]
        EB["EnemyBase"]
        EG["EnemyGunner"]
        EM -->|manages| EB
        EB -->|inherits| EG
    end
    
    SF -.->|creates| BS
    P -.->|uses| UIM
    ST -.->|triggers| GPS
    EM -.->|part of| GPS
```

---

## デザインパターン適用効果

| パターン | 効果 | 実装箇所 |
|---------|------|--------|
| **Factory** | シーン生成の一元化、拡張性向上 | `scene/base/SceneFactory` |
| **Component** | 責任分離、再利用性、テスト容易性 | `application/player/component/` |
| **Observer** | イベント駆動設計、疎結合性 | `application/SceneTransition.h` など |
| **Strategy** | エフェクト管理の柔軟性、拡張性 | `engine/postEffect/` |
| **Template Method** | 処理フローの統一化、ポリモーフィズム | `scene/base/BaseScene.h` |
| **Composite** | UI要素の統一管理、階層構造 | `application/ui/UIManager.h` |
| **Object Pool** | メモリ効率化、パフォーマンス最適化 | `application/enemy/EnemyManager.h` |

---

## 利点と特徴

### 🎯 構造的メリット
- **低結合度**: 各パターンが独立して機能
- **高凝集度**: 関連する処理が集約されている
- **拡張性**: 新機能追加時の既存コード変更を最小化

### 🔧 保守性向上
- **責任分離**: 各クラスが明確な責任を持つ
- **理解容易性**: パターンに基づいた設計で理解が容易
- **テスト容易性**: 個別コンポーネントの単体テスト可能

### 🚀 開発効率性
- **再利用性**: コンポーネント・パターンの再利用
- **スケーラビリティ**: 新しいシーン・敵・UI の追加が容易
- **保守コスト削減**: 設計パターンの明確化により修正が局所化

---

## 参考資料

詳細な実装については、以下のドキュメントを参照してください：
- [DESIGN_PATTERNS.md](DESIGN_PATTERNS.md) - パターンの詳細説明
- [ENGINE_DESIGN_DOCUMENT.md](ENGINE_DESIGN_DOCUMENT.md) - エンジン設計ドキュメント
- [CLASS_DESIGN_DOCUMENT.md](CLASS_DESIGN_DOCUMENT.md) - クラス設計ドキュメント
