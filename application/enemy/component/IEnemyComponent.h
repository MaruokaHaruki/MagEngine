#pragma once
#include <string>
#include <memory>
#include <typeindex>
#include <unordered_map>

// 前方宣言
class Enemy;

/**
 * @brief 敵コンポーネント設定データ
 * 
 * JSON の代わりにシンプルなKey-Value構造で設定を管理
 */
struct ComponentConfig {
	std::unordered_map<std::string, float> floats;
	std::unordered_map<std::string, int> integers;
	std::unordered_map<std::string, std::string> strings;
	std::unordered_map<std::string, bool> booleans;

	float GetFloat(const std::string& key, float defaultValue = 0.0f) const {
		auto it = floats.find(key);
		return (it != floats.end()) ? it->second : defaultValue;
	}

	int GetInt(const std::string& key, int defaultValue = 0) const {
		auto it = integers.find(key);
		return (it != integers.end()) ? it->second : defaultValue;
	}

	std::string GetString(const std::string& key, const std::string& defaultValue = "") const {
		auto it = strings.find(key);
		return (it != strings.end()) ? it->second : defaultValue;
	}

	bool GetBool(const std::string& key, bool defaultValue = false) const {
		auto it = booleans.find(key);
		return (it != booleans.end()) ? it->second : defaultValue;
	}

	void SetFloat(const std::string& key, float value) {
		floats[key] = value;
	}

	void SetInt(const std::string& key, int value) {
		integers[key] = value;
	}

	void SetString(const std::string& key, const std::string& value) {
		strings[key] = value;
	}

	void SetBool(const std::string& key, bool value) {
		booleans[key] = value;
	}
};

/**
 * @brief 敵コンポーネント基底インターフェース
 *
 * すべての敵コンポーネントがこのインターフェースを実装します。
 */
class IEnemyComponent {
public:
	virtual ~IEnemyComponent() = default;

	/**
	 * @brief コンポーネント初期化
	 * @param config コンポーネント設定
	 * @param owner このコンポーネントを所有するEnemyオブジェクト
	 */
	virtual void Initialize(const ComponentConfig& config, Enemy* owner) = 0;

	/**
	 * @brief 毎フレーム更新
	 * @param deltaTime フレーム経過時間（秒）
	 */
	virtual void Update(float deltaTime) {}

	/**
	 * @brief 描画処理（必要に応じてオーバーライド）
	 */
	virtual void Draw() {}

	/**
	 * @brief ImGui デバッグ表示
	 */
	virtual void DrawImGui() {}

	/**
	 * @brief コンポーネント名取得
	 */
	virtual std::string GetComponentName() const = 0;

	/**
	 * @brief 初期化完了時に呼び出されるコールバック
	 */
	virtual void OnInitializeComplete() {}

	/**
	 * @brief オーナーEnemyへのアクセス
	 */
	Enemy* GetOwner() const { return owner_; }

	/**
	 * @brief オーナーEnemyを設定（内部使用）
	 */
	void SetOwner(Enemy* owner) { owner_ = owner; }

protected:
	Enemy* owner_ = nullptr;
};
