///=============================================================================
///						変換行列
struct TransformationMatrix {
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInvTranspose;
};

///=============================================================================
///						Skybox専用構造体
//========================================
// 頂点シェーダー入力
struct VertexShaderInput {
    float4 position : POSITION0;
};

//========================================
// 頂点シェーダー出力（ピクセルシェーダー入力）
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;  // キューブマップ用の3D座標
};

//========================================
// ピクセルシェーダー出力
struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

//========================================
// サンプラー
SamplerState gSampler : register(s0);