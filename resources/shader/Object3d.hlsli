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
    float4 color;       // ライトの色
    float3 direction;   // ライトの方向
    float intensity;    // ライトの強度
};
// PointLight
struct PointLight
{
    float4 color;       // ライトの色
    float3 position;    // ライトの位置
    float intensity;    // ライトの強度
    float radius;       // ライトの影響範囲
    float decay;        // ライトの減衰率
};
// SpotLight
struct SpotLight
{
    float4 color;       // ライトの色
    float3 position;    // ライトの位置
    float intensity;    // ライトの強度
    float3 direction;   // ライトの方向
    float distance;     // ライトの影響範囲
    float decay;        // ライトの減衰率
    float cosAngle;     // スポットライトの角度
};