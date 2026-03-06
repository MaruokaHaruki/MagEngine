/*********************************************************************
 * \file   Trail.VS.hlsl
 * \brief  トレイルエフェクト頂点シェーダー
 *
 * \author MagEngine
 * \date   March 2026
 * \note   軌跡パーティクルの頂点変換
 *********************************************************************/
#include "Trail.hlsli"

///=============================================================================
///						コンスタントバッファ

// カメラ定数バッファ（b1）
ConstantBuffer<CameraConstant> gCamera : register(b1);

///=============================================================================
///						VertexShader
VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;
	
	//========================================
	// ワールド座標をそのまま使用（CPU側で既にワールド座標に変換済み）
	float3 worldPos = input.position;
	
	//========================================
	// ビュープロジェクション変換
	output.position = mul(float4(worldPos, 1.0f), gCamera.viewProj);
	
	//========================================
	// ワールド座標を設定
	output.worldPosition = worldPos;
	
	//========================================
	// 法線をそのまま使用
	output.normal = normalize(input.normal);
	
	//========================================
	// エイジを設定（0.0～1.0で正規化）
	output.age = input.age;
	
	return output;
}
