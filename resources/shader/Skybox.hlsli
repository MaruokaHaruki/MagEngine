///=============================================================================
///						Skybox専用構造体
//========================================
// SkyboxVertexShaderの出力
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD0;  // キューブマップ用の3D座標
};
//========================================
//定数バッファ
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};