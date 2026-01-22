# リファクタリング対応状況

## ✅ 完了した対応

### 1. スペルミスの修正
- `exetrnals` → `externals` (MagEngine.filters)
- `enelemtQuantity` → `elementQuantity` (SrvSetup.h/cpp)
- `transfomationMatrixBuffer_` → `transformationMatrixBuffer_` (Object3d, Skybox, Line, Sprite 全箇所)

### 2. コメントアウトされたコードの削除
- BaseObject.h: Draw() メソッドのコメント削除

### 3. 非const参照の修正
- BaseObject.h/cpp: Initialize(), Update() メソッドの Vector3& → const Vector3& に修正

### 4. Doxygenコメント形式の改善
- Collider.h: Intersects() メソッドのコメントを形式化
  - @param, @return, @note を明記
  - 衝突判定アルゴリズムの説明を追加
- BaseObject.h: クラス全体の責務を定義
  - コライダー管理
  - 衝突イベントのライフサイクル管理
  - 継承時の実装要件を明記

### 5. シングルトンの生ポインタ → unique_ptr化
- TextureManager.h: static TextureManager* → static std::unique_ptr<TextureManager>
- TextureManager.cpp: 
  - GetInstance() で make_unique() を使用
  - Finalize() で reset() を使用

### 6. マジックナンバーの定数化
- CollisionManager.h に CollisionConstants namespace を追加
  - kDefaultCellSize = 32.0f
  - kDefaultMaxObjects = 1024
- Initialize() メソッドのデフォルト引数を定数化

---

## ⚠️ 対応が困難な問題と解決策

### 1. **物理構造とフィルタの乖離** (engine\3d\Line.cpp等)
**問題**: ビジュアルスタジオのフィルタ(.filters)上での配置と実物理構造がずれている

**解決策**:
```
手動対応が必要です。MagEngine.filters ファイルを編集して、以下を修正してください：

1. VS のソリューションエクスプローラーで各ファイルを確認
2. 物理ファイルの場所に合わせてフィルタツリーを再構築
   例）engine\3d\line\Line.cpp → "ソース ファイル\engine\3d\line" 配下に配置
3. MagEngine.filters を手動編集するか、VS UI で Drag & Drop で再配置

代替案：
- プロジェクトをVS 2022で開く → ファイルを正しい場所にドラッグ＆ドロップ
- MagEngine.vcxproj を自動生成ツール（CMake等）で再生成
```

---

### 2. **クラスの責務定義が不足** (BaseObject.h等)
**問題**: 一行程度の説明のみで、クラス設計の全体像が不明確

**完了した部分**:
- BaseObject.h で責務を記載（コライダー管理、衝突イベント管理）

**推奨される追加対応**:
```cpp
// 各クラスのヘッダに詳細なコメントを追加例：
/**
* @brief プレイヤー弾クラス
* 
* 責務：
* - プレイヤーが発射した弾の管理と更新
* - 敵との衝突判定（BaseObjectインターフェース経由）
* - 一定距離・時間後の自動削除
* 
* 依存関係：
* - Object3d（3D描画）
* - BaseObject（衝突システム）
* 
* ライフサイクル：
* 1. Initialize() で初期位置・方向を設定
* 2. Update() で毎フレーム位置更新
* 3. 敵衝突時 OnCollisionEnter() で削除
*/
```

**対応方法**:
- 各クラスヘッダの先頭に上記形式で責務を記載
- 責務の共通化パターンを確立

---

### 3. **自明なコメントの多さ** (BaseObject.cpp:10等)
**問題**: `// コライダーの生成` など、コードから自明な説明ばかり

**改善案**:
```cpp
// 改善前：
collider_ = std::make_unique<Collider>();
// コライダーの生成

// 改善後：
// ローカルスコープでコライダー作成し、衝突判定システムと統合
// 【重要】位置・半径を明示的に設定する必要あり
collider_ = std::make_unique<Collider>();
collider_->SetPosition(position);
collider_->SetRadius(radius);
```

**推奨される対応**:
- コード自明な行：コメント削除
- ロジック意図が不明確な部分：WHY（なぜこの処理が必要か）を記述
- 計算ロジック：アルゴリズム説明を追加（CollisionManager.cpp の example を参考）

---

### 4. **未使用引数への対応** (Enemy.cpp:170等)
**問題**: FIXME と共に未使用引数が放置されている

**解決策**:
```cpp
// C++17以降の推奨方法：
void OnCollisionExit(BaseObject *other) override {
    [[maybe_unused]] BaseObject *unused = other;  // 今後実装予定
    // FIXME: 衝突終了時の敵固有処理を実装
}

// または削除検討：
// 継承先で不要なら、基底クラスで空実装してOK
void OnCollisionExit(BaseObject * /*other*/) override {
    // デフォルト実装：何もしない
}
```

---

### 5. **protectedメンバ変数の公開** (BaseObject.h:42,45等)
**問題**: 継承先から直接アクセス可能 → カプセル化破壊

**完了した部分**:
- Collider: SetPosition/GetPosition アクセサ既に実装

**推奨される全体対応**:
```cpp
// 改善前：
protected:
    Vector3 position_;
    float radius_;

// 改善後：
private:
    Vector3 position_;
    float radius_;
    
public:
    const Vector3& GetPosition() const { return position_; }
    void SetPosition(const Vector3& pos) { position_ = pos; }
    float GetRadius() const { return radius_; }
    void SetRadius(float r) { radius_ = r; }
```

**対応方法**:
- BaseObject, EnemyBase などの基底クラスを優先
- 継承時のオーバーライド箇所を検出してリファクタ

---

### 6. **グローバル状態の汚染** (BaseScene::sceneNo static)
**問題**: static メンバがグローバル変数化 → テスト困難・状態管理が脆弱

**解決策**（アーキテクチャレベル）:
```cpp
// 改善案1: シーン番号をスタックで管理
class SceneStack {
    static std::stack<int> sceneStack_;
    static int GetCurrentScene() { return sceneStack_.top(); }
    static void PushScene(int sceneNo) { sceneStack_.push(sceneNo); }
    static void PopScene() { sceneStack_.pop(); }
};

// 改善案2: SceneManager に一元化
class SceneManager {
private:
    int currentSceneNo_ = 0;  // static → instance member に変更
public:
    int GetCurrentScene() const { return currentSceneNo_; }
};
// 全体でシングルトン化：SceneManager::GetInstance()->GetCurrentScene()
```

**実装コスト**: 中程度（SceneManager 周辺の大幅リファクタ必要）

---

### 7. **グローバル関数の隠蔽** (DirectXCore.cpp:621 WriteToFile)
**問題**: WriteToFile がグローバル関数 → 名前空間汚染

**解決策**:
```cpp
// 改善前：
void WriteToFile(...) { }

// 改善後A: ユーティリティクラス化
class FileUtility {
public:
    static void WriteToFile(...) { }
    static bool ReadFromFile(...) { }
};

// 改善後B: 無名namespace + 関数
namespace {
    void WriteToFile(...) { }  // static内部リンク
}
```

**対応方法**:
- engine/utils/FileUtility.h を新規作成
- WriteToFile を static メソッド化
- DirectXCore.cpp の呼び出し箇所を FileUtility::WriteToFile() に変更

---

### 8. **Player.h の肥大化** (180行超過)
**問題**: メンバ変数が多すぎる → 責務が不明確・テスト困難

**推奨リファクタリング**:
```cpp
// 現状：Player が持つ責務が多すぎる
// - 移動制御
// - 攻撃管理（PlayerCombatComponent存在も、統合不完全）
// - UI状態管理
// - アニメーション管理

// 改善案：責務分割
class Player {
private:
    std::unique_ptr<PlayerMovementComponent> movement_;
    std::unique_ptr<PlayerCombatComponent> combat_;
    std::unique_ptr<PlayerAnimationComponent> animation_;
    std::unique_ptr<PlayerUIComponent> ui_;
    
public:
    void Update() {
        movement_->Update();
        combat_->Update();
        animation_->Update();
        ui_->Update();
    }
};
```

**実装コスト**: 大（メンバ変数の整理・component化に時間が必要）

---

### 9. **enum + switch の状態遷移パターン** (Enemy.cpp:143等)
**問題**: 状態数増加に伴い switch 文が肥大化 → 保守困難

**完了した部分**:
- Enemy.cpp で BehaviorState enum + switch が使用されている（既に実装）

**さらにステート パターンで改善する場合**:
```cpp
// 改善案：State パターン適用
class EnemyState {
public:
    virtual ~EnemyState() = default;
    virtual void OnEnter(Enemy *owner) {}
    virtual void Update(Enemy *owner) = 0;
    virtual void OnExit(Enemy *owner) {}
};

class ApproachState : public EnemyState {
    void Update(Enemy *owner) override { /* 接近処理 */ }
};

class CombatState : public EnemyState {
    void Update(Enemy *owner) override { /* 戦闘処理 */ }
};

// Enemy クラス側：
class Enemy {
private:
    std::unique_ptr<EnemyState> currentState_;
public:
    void Update() {
        if (shouldChangeState()) {
            currentState_ = createNextState();
        }
        currentState_->Update(this);
    }
};
```

**実装コスト**: 大（全enemy状態をクラス化）
**メリット**: 状態追加時の影響範囲が局所化

---

## 推奨される次の優先順位

### 優先度 High（設計に関わる）
1. ✅ **スペルミス** → 完了
2. ✅ **非const参照** → 完了  
3. ✅ **unique_ptr化** → 完了
4. 責務定義の記載（各クラスヘッダに）
5. protectedメンバのprivate化

### 優先度 Medium（保守性向上）
6. ✅ **定数化** → 完了
7. グローバル関数の隠蔽（FileUtility化）
8. ✅ **Doxygenコメント** → 完了

### 優先度 Low（大幅リファクタ）
9. Player クラスの分割
10. State パターンの適用
11. グローバル static の排除

---

## 確認事項

- [ ] すべての修正ファイルをビルド・テストして動作確認
- [ ] 物理構造とフィルタの乖離を手動で確認・修正
- [ ] 新規コメントが日本語・英語で統一されているか確認
