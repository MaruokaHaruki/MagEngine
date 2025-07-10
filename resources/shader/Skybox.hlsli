///=============================================================================
///						Skybox専用構造体
//========================================
// SkyboxVertexShaderの入力
struct SkyboxVertexInput {
    float4 position : POSITION0;
};

//========================================
// SkyboxVertexShaderの出力
struct SkyboxVertexOutput {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;  // キューブマップ用の3D座標
};

//========================================
// SkyboxPixelShaderの出力
struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

//========================================
// Skybox ViewProjection行列
struct SkyboxViewProjection {
    float4x4 viewProjection;
};