#pragma once
#include "Vector3.h"
#include "Vector4.h"

/// @brief 並行光源
struct DirectionalLight {
	Vector4 color;		//ライトの色
	Vector3 direction;	//ライトの向き
	float intensity;	//光度
};

/// @brief 点光源
struct PointLight {
	Vector4 color;		//ライトの色
	Vector3 position;	//ライトの位置
	float intensity;	//光度
	float radius;		//半径
	float decay;		//減衰
};

/// @brief スポットライト
struct SpotLight {
	Vector4 color;		//ライトの色
	Vector3 position;	//ライトの位置
	float intensity;	//光度
	Vector3 direction;	//ライトの向き
	float distance;		//距離
	float decay;		//減衰
	float cosAngle;		//角度
	float padding[2];		//パディング
};
