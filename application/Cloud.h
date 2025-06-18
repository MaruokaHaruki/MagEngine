#pragma once
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Vector3.h"
#include <memory>
#include <vector>

class Cloud {
	///--------------------------------------------------------------
	///							メンバ関数
public:

	/// \brief 初期化
	void Initialize();

	/// \brief 更新
	void Update();

	/// \brief 描画 
	void Draw();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:

	///--------------------------------------------------------------
	///							入出力関数
public:


	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// パーティクル
	std::unique_ptr<Particle> particle_;
	// パーティクルエミッター
	std::unique_ptr<ParticleEmitter> particleEmitter_;
};
