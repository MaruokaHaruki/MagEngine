#pragma once
#include "Particle.h"
#include "ParticleEmitter.h"
#include "Vector3.h"
#include <memory>
#include <numbers> 
#include <random>
#include <vector>

class Cloud {
	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize();

	/// \brief 更新
	/// \param playerPosition プレイヤーの位置（雲の生成範囲の基準）
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							プライベートメンバ関数
private:

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// Particle
};
