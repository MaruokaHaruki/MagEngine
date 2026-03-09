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
	// グラデーション色を計算（startColor から endColor へ）
	// age = 0.0 の時は startColor
	// age = 1.0 の時は endColor
	float3 gradientColor = lerp(gTrailParams.startColor, gTrailParams.endColor, fadeFactor);
	
	//========================================
	// 基本色とグラデーション色を合成
	// gradientColor が設定されていない場合は基本色を使用
	float3 finalColor = length(gTrailParams.startColor + gTrailParams.endColor) > 0.001f 
		? gradientColor 
		: gTrailParams.color;
	
	output.color = float4(finalColor, finalAlpha);
	
	return output;
}
