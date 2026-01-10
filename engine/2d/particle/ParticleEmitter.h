// Emit.h
#pragma once
#include "MagMath.h"
#include "Particle.h"
#include "ParticlePreset.h"

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
	ParticleEmitter(Particle *particle, const std::string &name, const MagMath::Transform &transform, uint32_t count, float frequency, bool repeat = false);

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
	void SetTranslate(const MagMath::Vector3 &translate) {
		transform_.translate = translate;
	}
	/// @brief SetRotate
	/// @param size エミッターの回転を設定
	void SetCustomTextureSize(const MagMath::Vector2 &size) {
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

	/// @brief SetBillboard
	/// @param enable ビルボードを使用するかどうか
	void SetBillboard(bool enable) {
		particle_->SetBillboard(enable);
	}

	/// @brief SetTranslateRange
	/// @param min 最小移動量
	/// @param max 最大移動量
	void SetTranslateRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetTranslateRange(min, max);
	}

	/// @brief SetVelocityRange
	/// @param min 最小初速度
	/// @param max 最大初速度
	void SetVelocityRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetVelocityRange(min, max);
	}

	/// @brief SetColorRange
	/// @param min 最小色 (RGBA)
	/// @param max 最大色 (RGBA)
	void SetColorRange(const MagMath::Vector4 &min, const MagMath::Vector4 &max) {
		particle_->SetColorRange(min, max);
	}

	/// @brief SetLifetimeRange
	/// @param min 最小生存時間
	/// @param max 最大生存時間
	void SetLifetimeRange(float min, float max) {
		particle_->SetLifetimeRange(min, max);
	}

	/// @brief SetInitialScaleRange
	/// @param min 最小初期スケール
	/// @param max 最大初期スケール
	void SetInitialScaleRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetInitialScaleRange(min, max);
	}

	/// @brief SetEndScaleRange
	/// @param min 最小終了スケール
	/// @param max 最大終了スケール
	void SetEndScaleRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetEndScaleRange(min, max);
	}

	/// @brief SetInitialRotationRange
	/// @param min 最小初期回転 (ラジアン)
	/// @param max 最大初期回転 (ラジアン)
	void SetInitialRotationRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetInitialRotationRange(min, max);
	}

	/// @brief SetEndRotationRange
	/// @param min 最小終了回転 (ラジアン)
	/// @param max 最大終了回転 (ラジアン)
	void SetEndRotationRange(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		particle_->SetEndRotationRange(min, max);
	}

	/// @brief SetGravity
	/// @param gravity 重力ベクトル
	void SetGravity(const MagMath::Vector3 &gravity) {
		particle_->SetGravity(gravity);
	}

	/// @brief SetFadeInOut
	/// @param fadeInRatio フェードイン比率
	/// @param fadeOutRatio フェードアウト比率
	void SetFadeInOut(float fadeInRatio, float fadeOutRatio) {
		particle_->SetFadeInOut(fadeInRatio, fadeOutRatio);
	}

	/// \brief プリセットから設定を適用
	/// \param config プリセット設定
	ParticleEmitter &ApplyConfig(const ParticleConfig &config);

	/// \brief ビルダーパターン - 移動範囲
	ParticleEmitter &SetTranslate(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		SetTranslateRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - 速度範囲
	ParticleEmitter &SetVelocity(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		SetVelocityRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - 色範囲
	ParticleEmitter &SetColor(const MagMath::Vector4 &min, const MagMath::Vector4 &max) {
		SetColorRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - 生存時間
	ParticleEmitter &SetLifetime(float min, float max) {
		SetLifetimeRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - 初期スケール
	ParticleEmitter &SetInitialScale(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		SetInitialScaleRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - 終了スケール
	ParticleEmitter &SetEndScale(const MagMath::Vector3 &min, const MagMath::Vector3 &max) {
		SetEndScaleRange(min, max);
		return *this;
	}

	/// \brief ビルダーパターン - ビルボード設定
	ParticleEmitter &Billboard(bool enable) {
		SetBillboard(enable);
		return *this;
	}

	/// \brief ビルダーパターン - 重力設定
	ParticleEmitter &Gravity(const MagMath::Vector3 &gravity) {
		SetGravity(gravity);
		return *this;
	}

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							メンバ変数
private:
	Particle *particle_;  // Particleのインスタンスを保持
	std::string name_;	  // パーティクルグループ名
	MagMath::Transform transform_; // エミッターの位置・回転・スケール
	uint32_t count_;	  // 発生させるパーティクルの数
	float frequency_;	  // 発生頻度
	float elapsedTime_;	  // 経過時間
	bool repeat_;		  // 繰り返し発生させるかどうかのフラグ
};