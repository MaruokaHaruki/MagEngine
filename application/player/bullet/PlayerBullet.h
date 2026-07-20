#pragma once
#include "BaseObject.h" // BaseObjectを継承
#include "Object3d.h"
#include "TrailEffect.h"
using Transform = MagMath::Transform;
#include "Transform.h"
#include "Vector3.h"
#include <memory>
#include <string>

// Forward declarations
class Object3dSetup;
namespace MagEngine {
	class TrailEffectManager;
	class RenderWorld;
}

class PlayerBullet : public BaseObject {
public:
	/// \brief 弾の3D表示、移動方向、寿命を初期化する
	/// \param object3dSetup 弾モデル生成に使用するセットアップ
	/// \param trailEffectManager 弾道トレイル生成に使用する管理クラス
	/// \param modelPath 弾モデルのファイルパス
	/// \param position 発射時のワールド座標
	/// \param direction 発射方向
	void Initialize(MagEngine::Object3dSetup *object3dSetup,
					MagEngine::TrailEffectManager *trailEffectManager,
					const std::string &modelPath, const Vector3 &position, const Vector3 &direction);

	/// \brief 速度に基づく移動と寿命切れ判定を更新する
	/// \param deltaTime 前フレームからの経過時間（秒）
	void Update(float deltaTime);

	/// \brief 弾モデルとトレイルをフレームの描画対象へ登録する
	/// \param renderWorld 現フレームの描画対象を集約するRenderWorld
	void RegisterRenderables(MagEngine::RenderWorld &renderWorld);

	/// \brief 弾が寿命切れまたは衝突で削除対象になっていないかを取得する
	/// \return 生存中の場合はtrue、削除対象の場合はfalse
	bool IsAlive() const {
		return isAlive_;
	}

	/// \brief 弾を削除対象としてマークする
	void SetDead();

	/// \brief 現在の弾のワールド座標を取得する
	/// \return 弾の現在位置
	Vector3 GetPosition() const;

	/// @brief 弾道方向の算出に使用する速度を取得
	/// @return 現在の移動速度への参照
	const Vector3 &GetVelocity() const {
		return velocity_;
	}

	/// \brief 当たり判定に使用する半径を取得する
	/// \return 弾の衝突半径
	float GetRadius() const {
		return radius_;
	}

	/// @brief 他オブジェクトとの衝突開始を処理する
	/// @param other 衝突した相手オブジェクト
	/// @note 相手の種類によらず弾を削除対象にする。
	void OnCollisionEnter(BaseObject *other) override;
	/// @brief 他オブジェクトとの接触継続を通知する
	/// @param other 接触を継続している相手オブジェクト
	void OnCollisionStay(BaseObject *other) override;
	/// @brief 他オブジェクトとの接触終了を通知する
	/// @param other 接触を終了した相手オブジェクト
	void OnCollisionExit(BaseObject *other) override;

private:
	//========================================
	//  3Dオブジェクト
	std::unique_ptr<MagEngine::Object3d> obj_;

	//========================================
	//  トレイルエフェクト
	std::unique_ptr<MagEngine::TrailEffect> trailEffect_;

	//========================================
	//  位置情報
	Transform transform_;

	Vector3 velocity_;
	float speed_;
	float lifeTime_;
	float maxLifeTime_;
	bool isAlive_;
	float radius_; // 当たり判定用の半径
};
