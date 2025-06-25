#pragma once
#include "LineManager.h" // LineManagerを追加
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
	/// \param particle パーティクルシステムのポインタ
	/// \param particleSetup パーティクルセットアップのポインタ
	void Initialize(Particle *particle, ParticleSetup *particleSetup);

	/// \brief 更新
	/// \param playerPosition プレイヤーの位置（雲の生成範囲の基準）
	void Update(const Vector3 &playerPosition);

	/// \brief 描画
	void Draw();

	/// \brief ImGui描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							プライベートメンバ関数
private:
	/// \brief エミッターを範囲内にランダム配置
	void CreateEmitters();

	/// \brief 霧の効果パラメータを設定
	void SetupFogEffect();

	/// \brief デバッグ描画（霧のエリアと風向きの可視化）
	void DrawDebugVisualization();

	///--------------------------------------------------------------
	///							メンバ変数
private:
	// パーティクルシステムへのポインタ
	Particle *particle_ = nullptr;
	ParticleSetup *particleSetup_ = nullptr;

	// エミッター配列
	std::vector<std::unique_ptr<ParticleEmitter>> fogEmitters_;

	// 霧システムのパラメータ
	Vector3 fogCenter_ = {0.0f, 0.0f, 0.0f};	 // 霧の中心位置
	Vector3 fogSize_ = {80.0f, 15.0f, 80.0f};	 // 霧の範囲サイズを拡大 (X, Y, Z)
	Vector3 windDirection_ = {1.0f, 0.0f, 0.2f}; // 風の方向
	float windStrength_ = 3.0f;					 // 風の強さを増加
	int emitterCount_ = 128;					 // エミッターの数を増加
	float emitterFrequency_ = 0.05f;			 // エミッター発生頻度を上げる（より頻繁に）
	int particlesPerEmitter_ = 12;				 // エミッター1つあたりのパーティクル数を増加
	float fogDensity_ = 0.8f;					 // 霧の密度を上げる
	bool isActive_ = true;						 // 霧システムの有効/無効

	// デバッグ表示用
	bool showDebugVisualization_ = true;		   // デバッグ表示のON/OFF
	Vector4 areaColor_ = {0.0f, 1.0f, 1.0f, 0.7f}; // エリア境界の色（シアン）
	Vector4 windColor_ = {1.0f, 0.0f, 0.0f, 1.0f}; // 風向き矢印の色（赤）
	float windArrowLength_ = 5.0f;				   // 風向き矢印の長さ

	// 乱数生成器
	std::random_device randomDevice_;
	std::mt19937 randomEngine_;
};
