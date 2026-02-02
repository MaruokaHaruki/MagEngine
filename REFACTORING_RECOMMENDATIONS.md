# リファクタリング推奨事項

このドキュメントは、将来的に大規模なリファクタリングが必要な設計課題をまとめたものです。

## 🔴 高優先度（設計上の大きな問題）

### 1. BaseScene::sceneNo のグローバル状態問題
**問題点:**
- `BaseScene::sceneNo` が static メンバとして実質的なグローバル遷移状態を持っている
- これはシーン間の疎結合を阻害し、テストやデバッグを困難にする

**推奨対応:**
```cpp
// 現在の設計（問題あり）
class BaseScene {
    static int sceneNo;  // グローバル状態
};

// 推奨される設計
class SceneManager {
    std::unique_ptr<BaseScene> currentScene_;
    int nextSceneNo_ = -1;
    
    void RequestSceneChange(int sceneNo);
    void Update();
};
```

**影響範囲:** scene/base/BaseScene.h, SceneFactory.cpp, 各シーンクラス

---

### 2. DirectXCore::InitializeDirectX の巨大関数問題
**問題点:**
- 初期化処理が1つの関数に集中しており、可読性・保守性が低い
- 各初期化ステップの依存関係が不明瞭

**推奨対応:**
各初期化ステップを private メソッドに分割:
```cpp
class DirectXCore {
private:
    void InitializeDevice();
    void InitializeCommandQueue();
    void InitializeSwapChain();
    void InitializeRenderTargets();
    void InitializeDepthStencil();
    void InitializeFence();
    void InitializeViewportAndScissor();
};
```

**影響範囲:** engine/base/core/DirectXCore.cpp (600行以上)

---

### 3. ハードコーディングされた設定の外部ファイル化
**問題点:**
- モデルパス (`"jet.obj"`)
- 敵のパラメータ（HP、速度、攻撃間隔等）
- UI位置やサイズ

すべて .cpp ファイル内にハードコードされており、調整に再コンパイルが必要

**推奨対応:**
```json
// resources/config/enemy_config.json
{
  "enemies": {
    "gunner": {
      "modelPath": "enemy_gunner.obj",
      "hp": 2,
      "speed": 15.0,
      "shootInterval": 1.5,
      "combatDuration": 15.0
    },
    "basic": {
      "modelPath": "enemy_basic.obj",
      "hp": 3,
      "speed": 4.0
    }
  }
}
```

**影響範囲:** application/enemy/*.cpp, application/player/*.cpp, application/ui/*.cpp

---

## 🟠 中優先度（保守性の問題）

### 4. ステートパターンへの移行
**問題点:**
- `Enemy` や `GameClearAnimation` が enum と switch-case で実装されている
- 状態が増えた際の保守性が低く、拡張が困難

**推奨対応:**
```cpp
// 現在の設計
class Enemy {
    enum class BehaviorState { Approach, Combat, Retreat };
    BehaviorState behaviorState_;
    
    void Update() {
        switch (behaviorState_) {
            case Approach: /* ... */ break;
            case Combat: /* ... */ break;
            case Retreat: /* ... */ break;
        }
    }
};

// 推奨される設計（ステートパターン）
class IEnemyState {
public:
    virtual ~IEnemyState() = default;
    virtual void Enter(Enemy* enemy) = 0;
    virtual void Update(Enemy* enemy) = 0;
    virtual void Exit(Enemy* enemy) = 0;
};

class ApproachState : public IEnemyState { /* ... */ };
class CombatState : public IEnemyState { /* ... */ };
class RetreatState : public IEnemyState { /* ... */ };

class Enemy {
    std::unique_ptr<IEnemyState> currentState_;
    void ChangeState(std::unique_ptr<IEnemyState> newState);
};
```

**影響範囲:** application/enemy/Enemy.cpp, application/GameClearAnimation.cpp

---

### 5. Singleton パターンの改善
**問題点:**
- `TextureManager` 等の Singleton 実装が生ポインタ管理で不完全
- メモリリークのリスクあり

**推奨対応:**
```cpp
// 現在の設計（問題あり）
class TextureManager {
    static TextureManager* instance_;
public:
    static TextureManager* GetInstance() {
        if (instance_ == nullptr) {
            instance_ = new TextureManager();  // メモリリーク
        }
        return instance_;
    }
};

// 推奨される設計
class TextureManager {
public:
    static TextureManager& GetInstance() {
        static TextureManager instance;  // Meyers' Singleton
        return instance;
    }
    
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    
private:
    TextureManager() = default;
    ~TextureManager() = default;
};
```

**影響範囲:** engine/2d/texture/TextureManager.cpp 他、Singleton パターンを使用する全クラス

---

### 6. EnemyBase のカプセル化問題
**問題点:**
- EnemyBase のメンバが `protected` で公開されすぎている
- 派生クラスからの不正操作を許容する設計

**推奨対応:**
```cpp
// 現在の設計（問題あり）
class EnemyBase {
protected:
    int hp_;
    float speed_;
    Vector3 position_;
    // 派生クラスから直接アクセス可能
};

// 推奨される設計
class EnemyBase {
private:
    int hp_;
    float speed_;
    Vector3 position_;
    
protected:
    // Getter/Setter で制御
    int GetHP() const { return hp_; }
    void SetHP(int hp) { hp_ = std::max(0, hp); }  // バリデーション付き
    
    float GetSpeed() const { return speed_; }
    void SetSpeed(float speed);
    
    const Vector3& GetPosition() const { return position_; }
    void SetPosition(const Vector3& pos);
};
```

**影響範囲:** application/enemy/EnemyBase.h, Enemy.cpp, EnemyGunner.cpp

---

### 7. DirectXCore.cpp のグローバル関数問題
**問題点:**
- `WriteToFile` (609行目) がグローバル関数として定義されている
- ユーティリティクラスへ集約すべき

**推奨対応:**
```cpp
// engine/utils/FileUtility.h
namespace MagEngine {
namespace Utility {
    class FileUtility {
    public:
        static void WriteToFile(const std::string& path, const std::string& content);
        static std::string ReadFromFile(const std::string& path);
    };
}
}
```

**影響範囲:** engine/base/core/DirectXCore.cpp, engine/utils/

---

## 🟢 低優先度（最適化・クリーンアップ）

### 8. 未使用コードの削除
**問題点:**
- `Model::InstancingDraw` など、定義されているが呼び出し箇所が確認できないコードがある

**推奨対応:**
- コードカバレッジツールで未使用コードを特定
- 使用されていないメソッドは削除または `[[deprecated]]` でマーク

---

### 9. 重複コードの統合
**問題点:**
- `Enemy` と `EnemyGunner` で類似の移動計算が重複している
- `PlayerCombatComponent` の重複ファイル

**推奨対応:**
- 共通の移動ロジックを `EnemyBase` または独立した `MovementSystem` クラスに抽出
- 重複ファイルの整理と削除

---

## 📊 優先度の判断基準

| 優先度 | 基準 |
|--------|------|
| 🔴 高 | 設計上の根本的な問題。バグや拡張性の深刻な阻害要因 |
| 🟠 中 | 保守性に影響。将来的な機能追加で問題になる可能性 |
| 🟢 低 | コード品質の向上。動作には影響しない |

---

## 📝 実施時の注意事項

1. **段階的な実施**: すべてを一度に行わず、優先度順に1つずつ対応
2. **テストの追加**: リファクタリング前に既存動作のテストを追加
3. **ブランチ戦略**: feature ブランチで作業し、PR でレビュー
4. **パフォーマンス計測**: 大規模変更前後でベンチマークを取る
5. **ドキュメント更新**: 設計変更時は本ファイルと README.md を更新

---

**最終更新:** 2026年2月2日
**作成者:** GitHub Copilot (自動生成)
