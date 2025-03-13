///=============================================================================
///						VertexShader
//========================================
// VertexShaderの入力
struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0;
};
//========================================
//変換行列
struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
};
//========================================
//定数バッファ
struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

///=============================================================================
///						PixelShader
//========================================
// PixelShaderの出力
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};
//========================================
//マテリアル
struct Material
{
    float4 color;
    int enableLighting;
    float4x4 uvTransform;
    float shininess;
};
//========================================
//カメラ
struct Camera
{
    float3 worldPosition;
};
//========================================
// Light
// DirectionalLight
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};
// PointLight
struct PointLight
{
    float4 color;
    float3 position;
    float intensity;
};
// SpotLight
struct SpotLight
{
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float distance;
    float decay;
    float cosAngle;
};