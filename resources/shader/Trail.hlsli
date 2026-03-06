///=============================================================================
///						VertexShader
//========================================
// VertexShaderの入力
struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float3 normal : NORMAL0;
	float3 worldPosition : POSITION0;
	float age : TEXCOORD0;
};

//========================================
// 頂点シェーダー入力
struct VertexShaderInput {
	float3 position : POSITION0;
	float age : TEXCOORD0;
	float3 normal : NORMAL0;
};

///=============================================================================
///						PixelShader
//========================================
// PixelShaderの出力
struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

//========================================
// トレイルレンダリングパラメータ
struct TrailRenderParams {
	float3 color;				// エフェクト色
	float opacity;				// 透明度 (0.0-1.0)
	float width;				// 軌跡幅
	float lifeTime;				// ライフタイム（秒）
	float time;					// 経過時間
	float velocityDamping;		// 速度減衰
	float gravityInfluence;		// 重力影響度
	float padding;				// パディング
};

//========================================
// カメラ定数バッファ
struct CameraConstant {
	float4x4 viewProj;			// ビュープロジェクション行列
	float3 worldPosition;		// カメラのワールド座標
	float time;					// 経過時間
};
