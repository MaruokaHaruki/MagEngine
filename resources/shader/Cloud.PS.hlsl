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
    float topHeight = gBaseHeight + gHeightRange;

    if ((abs(viewDir.y) < 0.0001f) || (viewDir.y >= 0.0f && gCameraPosition.y > topHeight)) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float t0 = (gBaseHeight - gCameraPosition.y) / viewDir.y;
    float t1 = (topHeight - gCameraPosition.y) / viewDir.y;
    if (t0 > t1) {
        float tmp = t0;
        t0 = t1;
        t1 = tmp;
    }

    float tNear = max(t0, 0.0f);
    float tFar = min(t1, gMaxDistance);
    if (tFar <= tNear) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

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
        float heightFactor = saturate((position.y - gBaseHeight) / gHeightRange);

        float3 baseSample = position * gBaseNoiseScale + gTime * 0.03f;
        float baseNoise = Fbm(baseSample);
        float detailNoise = Fbm(position * gDetailNoiseScale + gTime * 0.09f);
        float weather = SampleWeather(position.xz * gWeatherMapScale);

        float density = lerp(baseNoise, detailNoise, gDetailWeight);
        density = saturate(density + weather - gCoverage);
        density *= gDensity;
        density *= smoothstep(0.0f, 0.08f, heightFactor) * smoothstep(1.0f, 0.92f, heightFactor);

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