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

cbuffer CloudCameraCB : register(b0)
{
    float4x4 gInvViewProjection;
    float3 gCameraPosition;
    float  gCameraPadding;
};

cbuffer CloudParamsCB : register(b1)
{
    float3 gSunDirection;
    float  gTime;
    float3 gSunColor;
    float  gDensity;
    float3 gCloudPosition;    // 雲の中心位置
    float  gCloudScale;       // 雲のスケール
    float  gBaseHeight;
    float  gHeightRange;
    float  gStepLength;
    float  gMaxDistance;
    float  gBaseNoiseScale;
    float  gDetailNoiseScale;
    float  gDetailWeight;
    float  gWeatherMapScale;
    float  gCoverage;
    float  gAmbient;
    float  gLightStepLength;
    float  gShadowDensity;
    float  gAnisotropy;
    float  gSunIntensity;
    float  gCloudRadius;      // 雲の範囲
    float  gPadCloud;
};

Texture2D<float4> gWeatherMap : register(t0);
SamplerState gLinearSampler : register(s0);

static const float PI = 3.14159265f;

float HashFloat3(float3 p) {
    return frac(sin(dot(p, float3(12.9898, 78.233, 37.719))) * 43758.5453f);
}

float Noise3D(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    float3 u = f * f * (3.0f - 2.0f * f);

    float n000 = HashFloat3(i + float3(0.0f, 0.0f, 0.0f));
    float n100 = HashFloat3(i + float3(1.0f, 0.0f, 0.0f));
    float n010 = HashFloat3(i + float3(0.0f, 1.0f, 0.0f));
    float n110 = HashFloat3(i + float3(1.0f, 1.0f, 0.0f));
    float n001 = HashFloat3(i + float3(0.0f, 0.0f, 1.0f));
    float n101 = HashFloat3(i + float3(1.0f, 0.0f, 1.0f));
    float n011 = HashFloat3(i + float3(0.0f, 1.0f, 1.0f));
    float n111 = HashFloat3(i + float3(1.0f, 1.0f, 1.0f));

    float nx00 = lerp(n000, n100, u.x);
    float nx10 = lerp(n010, n110, u.x);
    float nx01 = lerp(n001, n101, u.x);
    float nx11 = lerp(n011, n111, u.x);

    float nxy0 = lerp(nx00, nx10, u.y);
    float nxy1 = lerp(nx01, nx11, u.y);

    return lerp(nxy0, nxy1, u.z);
}

float Fbm(float3 p) {
    float sum = 0.0f;
    float amp = 0.5f;
    float freq = 1.0f;
    [unroll]
    for (int i = 0; i < 5; ++i) {
        sum += amp * Noise3D(p * freq);
        freq *= 2.02f;
        amp *= 0.5f;
    }
    return sum;
}

float SampleWeather(float2 uv) {
    if (gWeatherMapScale <= 0.0f) {
        return Fbm(float3(uv * 0.3f, gTime * 0.04f));
    }
    return gWeatherMap.SampleLevel(gLinearSampler, uv, 0.0f).r;
}

float PhaseFunction(float cosTheta, float g) {
    float g2 = g * g;
    float denom = pow(max(1.0f + g2 - 2.0f * g * cosTheta, 0.001f), 1.5f);
    return (1.0f - g2) / (4.0f * PI * denom);
}

float SampleShadow(float3 position, float3 lightDir) {
    float transmittance = 1.0f;
    float step = gLightStepLength;
    float3 cloudMin = gCloudPosition - gCloudRadius * gCloudScale;
    float3 cloudMax = gCloudPosition + gCloudRadius * gCloudScale;
    
    [unroll]
    for (int i = 0; i < 6 && transmittance > 0.05f; ++i) {
        position += lightDir * step;
        
        // 雲の範囲外判定
        if (any(position < cloudMin) || any(position > cloudMax)) {
            break;
        }
        
        // 雲の中心からの相対位置を計算
        float3 relativePos = (position - gCloudPosition) / gCloudScale;
        float3 p = relativePos * gBaseNoiseScale + gTime * 0.02f;
        float baseNoise = Fbm(p);
        float detailNoise = Fbm(relativePos * gDetailNoiseScale + gTime * 0.07f);
        float density = saturate(lerp(baseNoise, detailNoise, gDetailWeight) + SampleWeather(relativePos.xz * gWeatherMapScale) - gCoverage);
        density *= gDensity;
        transmittance *= exp(-density * step * gShadowDensity);
    }
    return transmittance;
}