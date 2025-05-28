// Emit.h
#pragma once
#include "Particle.h"
#include "ParticleSetup.h"
#include <vector>

class ParticleEmitter {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief コンストラクタ
	/// \param particle パーティクルマネージャのインスタンス
	/// \param name パーティクルグループ名
	/// \param transform エミッターの位置・回転・スケール
	/// \param count 発生させるパーティクルの数
	/// \param frequency 発生頻度
	/// \param repeat 繰り返し発生させるかどうかのフラグ
	ParticleEmitter(Particle *particle, const std::string &name, const Transform &transform, uint32_t count, float frequency, bool repeat = false);

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// @brief Emit
	/// \details パーティクルを発生させる
	void Emit();

	/// \brief SetRepeat
	/// \param repeat 繰り返し発生させるかどうかのフラグ
	void SetRepeat(bool repeat);

	/// @brief SetScale
	/// @param translate エミッターの位置を設定
	void SetTranslate(const Vector3 &translate) {
		transform_.translate = translate;
	}
	/// @brief SetRotate
	/// @param size エミッターの回転を設定
	void SetCustomTextureSize(const Vector2 &size) {
		particle_->SetCustomTextureSize(size);
	}
	/// @brief SetParticleShape
	/// @details パーティクルの形状を設定
	/// @param shape パーティクルの形状
	void SetParticleShape(ParticleShape shape) {
		particle_->SetParticleShape(shape);
	}
	/// @brief SetRingRadius
	/// @param radius パーティクルの半径
	void SetRingRadius(float radius) {
		particle_->SetRingRadius(radius);
	}
	/// @brief SetCylinderParams
	/// @param height パーティクルの高さ
	/// @param radius パーティクルの半径
	void SetCylinderParams(float height, float radius) {
		particle_->SetCylinderParams(height, radius);
	}

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	Particle *particle_;  // ParticleManagerのインスタンスを保持
	std::string name_;	  // パーティクルグループ名
	Transform transform_; // エミッターの位置・回転・スケール
	uint32_t count_;	  // 発生させるパーティクルの数
	float frequency_;	  // 発生頻度
	float elapsedTime_;	  // 経過時間
	bool repeat_;		  // 繰り返し発生させるかどうかのフラグ
};
