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
    // NDC座標からワールド座標を復元
    float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    float4 clipPos = float4(ndc, 0.0f, 1.0f);
    float4 worldPos = mul(clipPos, gInvViewProjection);
    worldPos.xyz /= worldPos.w;

    float3 rayOrigin = gCameraPosition;
    float3 rayDir = normalize(worldPos.xyz - rayOrigin);
    
    // 雲のAABB
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
    // レイとAABBの交差判定
    float tNear, tFar;
    if (!IntersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        // AABBと交差しない
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    tNear = max(tNear, 0.0f);
    tFar = min(tFar, gMaxDistance);
    
    if (tFar <= tNear) {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    
    // レイマーチング
    float3 sunDir = normalize(gSunDirection);
    float3 accumulatedLight = 0.0f;
    float transmittance = 1.0f;
    
    float t = tNear;
    float marchDistance = tFar - tNear;
    int numSteps = min(int(marchDistance / gStepSize), MAX_STEPS);
    float actualStepSize = marchDistance / float(max(numSteps, 1));
    
    // デバッグ用: サンプル数カウント
    int denseSampleCount = 0;
    
    for (int i = 0; i < numSteps && transmittance > 0.01f; i++) {
        float3 position = rayOrigin + rayDir * (t + actualStepSize * 0.5f);
        
        float density = SampleCloudDensity(position);
        
        if (density > 0.001f) {
            denseSampleCount++;
            
            // ライトマーチング
            float lightEnergy = LightMarch(position);
            
            // 位相関数
            float phase = PhaseHG(dot(rayDir, sunDir), gAnisotropy);
            
            // ライティング計算
            float3 lighting = gAmbient + gSunColor * gSunIntensity * lightEnergy * phase;
            
            // 散乱と吸収
            float scatterAmount = density * actualStepSize;
            accumulatedLight += lighting * scatterAmount * transmittance;
            transmittance *= exp(-density * actualStepSize);
        }
        
        t += actualStepSize;
    }
    
    float alpha = 1.0f - transmittance;
    
    // デバッグモード
    if (gDebugFlag > 0.5f) {
        if (denseSampleCount == 0) {
            // AABBに交差したが密度サンプルが0 → 青
            return float4(0.0f, 0.0f, 1.0f, 0.5f);
        } else if (alpha < 0.01f) {
            // 密度はあったがアルファが低い → 黄色
            return float4(1.0f, 1.0f, 0.0f, 0.5f);
        } else {
            // 正常に描画できている → 緑のアウトライン
            return float4(accumulatedLight + float3(0.2f, 0.5f, 0.2f), alpha);
        }
    }
    
    return float4(accumulatedLight, alpha);
}