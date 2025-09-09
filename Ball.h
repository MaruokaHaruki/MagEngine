#pragma once
#include "Vector3.h"
#include "Vector4.h"

// 前方宣言
class LineManager;

///=============================================================================
///
class Ball {

	///--------------------------------------------------------------
	///							メンバ関数
public:
	/// \brief 初期化
	void Initialize();

	/// \brief 更新
	void Update();

	/// \brief 描画
	void Draw();

	/// \brief 境界範囲を設定
	void SetBounds(float minX, float maxX, float minY, float maxY);

	/// \brief 線描画による2D表示
	void DrawWireframe();

	/// \brief 境界の壁を線で描画
	void DrawBounds();

	/// \brief ImGUI描画
	void DrawImGui();

	///--------------------------------------------------------------
	///							静的メンバ関数
private:
	///--------------------------------------------------------------
	///							入出力関数
public:
	// 位置の取得・設定
	const Vector3 &GetPosition() const {
		return position_;
	}
	void SetPosition(const Vector3 &position) {
		position_ = position;
	}

	// 速度の取得・設定
	const Vector3 &GetVelocity() const {
		return velocity_;
	}
	void SetVelocity(const Vector3 &velocity) {
		velocity_ = velocity;
	}

	// 半径の取得・設定
	float GetRadius() const {
		return radius_;
	}
	void SetRadius(float radius) {
		radius_ = radius;
	}

	///--------------------------------------------------------------
	///							メンバ変数
private:
	//========================================
	// 位置
	Vector3 position_ = {0.0f, 0.0f, 0.0f};
	// 速度
	Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
	// 加速度
	Vector3 acceleration_ = {0.0f, 0.0f, 0.0f};

	//========================================
	// 半径
	float radius_ = 1.0f;
	// 質量
	float mass_ = 1.0f;
	// 反発係数
	float restitution_ = 1.0f;
	// 摩擦係数
	float friction_ = 0.0f;

	//========================================
	// 境界範囲
	float boundsMinX_ = -10.0f;
	float boundsMaxX_ = 10.0f;
	float boundsMinY_ = -10.0f;
	float boundsMaxY_ = 10.0f;

	// 重力
	float gravity_ = 9.8f;
	// デルタタイム
	float deltaTime_ = 1.0f / 60.0f;

	//========================================
	// 描画設定
	Vector4 ballColor_ = {1.0f, 0.5f, 0.0f, 1.0f};	 // オレンジ色
	Vector4 boundsColor_ = {0.0f, 1.0f, 0.0f, 1.0f}; // 緑色
	int circleSegments_ = 24;						 // 円の分割数
};
