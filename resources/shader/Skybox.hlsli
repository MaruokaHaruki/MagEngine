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

///=============================================================================
///                      ライト構造体（Object3dと同じ）
// 並行光源
struct DirectionalLight {
    float4 color;      // ライトの色
    float3 direction;  // ライトの向き
    float intensity;   // ライトの強度
};

// ポイントライト
struct PointLight {
    float4 color;      // ライトの色(16 bytes)
    float3 position;   // ライトの位置(12 bytes) + padding(4 bytes) = 16 bytes
    float intensity;   // ライトの強度(4 bytes)
    float radius;      // ライトの範囲(4 bytes)
    float decay;       // ライトの減衰(4 bytes)
    float padding;     // パディング(4 bytes)
};

// スポットライト
struct SpotLight {
    float4 color;           // ライトの色(16 bytes)
    float3 position;        // ライトの位置(12 bytes) + padding(4 bytes) = 16 bytes
    float intensity;        // ライトの強度(4 bytes)
    float3 direction;       // ライトの向き(12 bytes) + padding(4 bytes) = 16 bytes
    float distance;         // ライトの距離(4 bytes)
    float decay;            // ライトの減衰(4 bytes)
    float cosFalloffStart;  // フォールオフ開始(4 bytes)
    float cosFalloffEnd;    // フォールオフ終了(4 bytes)
};

// ライト定数バッファ
cbuffer DirectionalLightCB : register(b1)
{
    DirectionalLight gDirectionalLight;
};

cbuffer PointLightCB : register(b2)
{
    PointLight gPointLight;
};

cbuffer SpotLightCB : register(b3)
{
    SpotLight gSpotLight;
};