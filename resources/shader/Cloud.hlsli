#pragma once

///=============================================================================
///						VertexShader
//========================================
//定数バッファ
struct VertexShaderInput {
    float3 position : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// PixelShader出力構造体を追加
struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

cbuffer CloudCameraCB : register(b0)
{
    float4x4 gInvViewProjection;
    float3 gCameraPosition;
    float  gCameraPadding;
    
    // カメラパラメータは既に定義されています
    float gNearPlane;
    float gFarPlane;
    float gPadding4;
    float gPadding5;
    
    // ビュープロジェクション行列を追加
    float4x4 gViewProjection;
};

cbuffer CloudParamsCB : register(b1)
{
    // 雲の位置とサイズ
    float3 gCloudCenter;
    float gCloudSizeX;
    float3 gCloudSize;
    float gPadding0;
    
    // ライティング
    float3 gSunDirection;
    float gSunIntensity;
    float3 gSunColor;
    float gAmbient;
    
    // 雲の密度とノイズ
    float gDensity;
    float gCoverage;
    float gBaseNoiseScale;
    float gDetailNoiseScale;
    
    // レイマーチング設定
    float gStepSize;
    float gMaxDistance;
    float gLightStepSize;
    float gShadowDensityMultiplier;
    
    // アニメーション
    float gTime;
    float gNoiseSpeed;
    float gDetailWeight;
    float gAnisotropy;
    
    // デバッグ
    float gDebugFlag;
    float gPadding1;
    float gPadding2;
    float gPadding3;
};

Texture2D<float4> gWeatherMap : register(t0);
SamplerState gLinearSampler : register(s0);

static const float PI = 3.14159265f;
static const int MAX_STEPS = 128;
static const int MAX_LIGHT_STEPS = 6;

// ハッシュ関数
float Hash(float3 p) {
    p = frac(p * 0.3183099f + 0.1f);
    p *= 17.0f;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// 3Dノイズ
float Noise3D(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0f - 2.0f * f);

    return lerp(
        lerp(lerp(Hash(i + float3(0, 0, 0)), Hash(i + float3(1, 0, 0)), f.x),
             lerp(Hash(i + float3(0, 1, 0)), Hash(i + float3(1, 1, 0)), f.x), f.y),
        lerp(lerp(Hash(i + float3(0, 0, 1)), Hash(i + float3(1, 0, 1)), f.x),
             lerp(Hash(i + float3(0, 1, 1)), Hash(i + float3(1, 1, 1)), f.x), f.y),
        f.z);
}

// FBMノイズ
float FBM(float3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * Noise3D(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

// レイとAABBの交差判定
bool IntersectAABB(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax, out float tNear, out float tFar) {
    float3 invDir = 1.0f / (rayDir + 1e-6f);
    float3 t0 = (boxMin - rayOrigin) * invDir;
    float3 t1 = (boxMax - rayOrigin) * invDir;
    
    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);
    
    tNear = max(max(tMin.x, tMin.y), tMin.z);
    tFar = min(min(tMax.x, tMax.y), tMax.z);
    
    return tFar > tNear && tFar > 0.0f;
}

// 雲の密度サンプリング
float SampleCloudDensity(float3 position) {
    float3 uvw = (position - gCloudCenter) / gCloudSize;
    
    // バウンディングボックス外は0
    if (any(abs(uvw) > 0.5f)) {
        return 0.0f;
    }
    
    // ベースノイズ - Z軸方向に流れるように時間オフセットを追加（符号を反転してZ-からZ+へ）
    float3 baseCoord = position * gBaseNoiseScale + float3(0.0f, 0.0f, gTime * gNoiseSpeed);
    float baseNoise = FBM(baseCoord, 4);
    
    // ディテールノイズ - Z軸方向に異なる速度で流れる（符号を反転してZ-からZ+へ）
    float3 detailCoord = position * gDetailNoiseScale + float3(0.0f, 0.0f, gTime * gNoiseSpeed * 0.7f);
    float detailNoise = FBM(detailCoord, 3);
    
    // ノイズ合成
    float density = lerp(baseNoise, detailNoise, gDetailWeight);
    density = saturate(density - gCoverage);
    
    // エッジフェード
    float3 edgeDist = 0.5f - abs(uvw);
    float edgeFade = min(min(edgeDist.x, edgeDist.y), edgeDist.z);
    edgeFade = smoothstep(0.0f, 0.1f, edgeFade);
    
    return density * edgeFade * gDensity;
}

// ライトマーチング
float LightMarch(float3 position) {
    float3 lightDir = normalize(gSunDirection);
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
    float transmittance = 1.0f;
    float3 rayPos = position;
    
    for (int i = 0; i < MAX_LIGHT_STEPS; i++) {
        rayPos += lightDir * gLightStepSize;
        
        // バウンディングボックス外に出たら終了
        if (any(rayPos < boxMin) || any(rayPos > boxMax)) {
            break;
        }
        
        float density = SampleCloudDensity(rayPos);
        if (density > 0.001f) {
            transmittance *= exp(-density * gLightStepSize * gShadowDensityMultiplier);
            if (transmittance < 0.01f) break;
        }
    }
    
    return transmittance;
}

// Henyey-Greenstein位相関数
float PhaseHG(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0f - g2) / (4.0f * PI * pow(abs(1.0f + g2 - 2.0f * g * cosTheta), 1.5f));
}

// 線形深度を計算する関数を追加
float LinearizeDepth(float depth, float near, float far)
{
    return (2.0f * near * far) / (far + near - depth * (far - near));
}