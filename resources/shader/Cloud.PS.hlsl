/*********************************************************************
 * \file   Cloud.PS.hlsl
 * \brief
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note
 *********************************************************************/
#include "Cloud.hlsli"

float4 main(VertexShaderOutput input) : SV_TARGET {
    float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    float4 clipPos = float4(ndc, 0.0f, 1.0f);
    float4 worldPos = mul(clipPos, gInvViewProjection);
    worldPos.xyz /= worldPos.w;

    float3 viewDir = normalize(worldPos.xyz - gCameraPosition);
    
    // 雲のAABBとの交差判定
    float3 cloudMin = gCloudPosition - float3(gCloudRadius, gHeightRange * 0.5f, gCloudRadius) * gCloudScale;
    float3 cloudMax = gCloudPosition + float3(gCloudRadius, gHeightRange * 0.5f, gCloudRadius) * gCloudScale;
    
    // レイとAABBの交差計算
    float3 invDir = 1.0f / viewDir;
    float3 t0 = (cloudMin - gCameraPosition) * invDir;
    float3 t1 = (cloudMax - gCameraPosition) * invDir;
    float3 tmin = min(t0, t1);
    float3 tmax = max(t0, t1);
    
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);
    
    if (tFar <= tNear || tFar < 0.0f) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    tNear = max(tNear, 0.0f);
    tFar = min(tFar, gMaxDistance);

    float stepLength = max(gStepLength, 0.5f);
    int stepCount = clamp(int((tFar - tNear) / stepLength), 1, 96);

    float3 sunDir = normalize(gSunDirection);
    float3 sunColor = gSunColor * gSunIntensity;

    float3 accumulatedLight = 0.0f.xxx;
    float transmittance = 1.0f;

    float t = tNear;
    [loop]
    for (int i = 0; i < stepCount && transmittance > 0.02f; ++i) {
        float sampleT = t + 0.5f * stepLength;
        float3 position = gCameraPosition + viewDir * sampleT;
        
        // 雲の中心からの相対位置
        float3 relativePos = (position - gCloudPosition) / gCloudScale;
        float distFromCenter = length(relativePos);
        
        // 雲の範囲外は処理しない
        if (distFromCenter > gCloudRadius) {
            t += stepLength;
            continue;
        }

        float3 baseSample = relativePos * gBaseNoiseScale + gTime * 0.03f;
        float baseNoise = Fbm(baseSample);
        float detailNoise = Fbm(relativePos * gDetailNoiseScale + gTime * 0.09f);
        float weather = SampleWeather(relativePos.xz * gWeatherMapScale);

        float density = lerp(baseNoise, detailNoise, gDetailWeight);
        density = saturate(density + weather - gCoverage);
        density *= gDensity;
        
        // エッジのフェード
        float edgeFade = smoothstep(gCloudRadius, gCloudRadius * 0.8f, distFromCenter);
        density *= edgeFade;

        if (density > 0.0005f) {
            float visibility = SampleShadow(position, sunDir);
            float phase = PhaseFunction(dot(viewDir, sunDir), gAnisotropy);
            float3 lighting = gAmbient.xxx + sunColor * visibility * phase;
            float scatter = density * stepLength;
            accumulatedLight += lighting * scatter * transmittance;
            float attenuation = exp(-density * stepLength);
            transmittance *= attenuation;
        }

        t += stepLength;
    }

    float alpha = saturate(1.0f - transmittance);
    return float4(accumulatedLight, alpha);
}