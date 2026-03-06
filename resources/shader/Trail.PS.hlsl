/*********************************************************************
 * \file   Trail.PS.hlsl
 * \brief  トレイルエフェクトピクセルシェーダー
 *
 * \author MagEngine
 * \date   March 2026
 * \note   軌跡パーティクルのカラーと透明度を制御
 *********************************************************************/
#include "Trail.hlsli"

///=============================================================================
///						コンスタントバッファ

// トレイルレンダリングパラメータ（b0）
ConstantBuffer<TrailRenderParams> gTrailParams : register(b0);

///=============================================================================
///						PixelShader
PixelShaderOutput main(VertexShaderOutput input) {
	PixelShaderOutput output;
	
	//========================================
	// エイジ（0.0～1.0）に基づくフェードアウト計算
	// 0.0: 完全に不透明
	// 1.0: 完全に透明
	float fadeFactor = input.age;
	
	//========================================
	// Ease-out効果でより自然なフェードアウト（Quadratic）
	// 最初は半透明、終わり間際に急速に消える
	float easeOutAlpha = 1.0f - (fadeFactor * fadeFactor);
	
	//========================================
	// 最終的な不透明度を計算
	float finalAlpha = gTrailParams.opacity * easeOutAlpha;
	
	//========================================
	// 基本色を設定（シンプルな無ライティング）
	output.color = float4(gTrailParams.color, finalAlpha);
	
	return output;
}
