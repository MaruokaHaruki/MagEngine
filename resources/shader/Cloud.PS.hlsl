/*********************************************************************
 * \file   Cloud.PS.hlsl
 * \brief
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note
 *********************************************************************/
#include "Cloud.hlsli"

struct PixelOutput {
    float4 color : SV_TARGET0;
    float depth : SV_Depth;
};

PixelOutput main(VertexShaderOutput input) {
    PixelOutput output;
    output.depth = 1.0f; // デフォルトは最遠
    
    // スクリーン座標からレイの方向を計算（改善版）
    float2 ndc = float2(input.uv.x       * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    
    // 近平面と遠平面の2点でワールド座標を復元
    float4 nearPoint = float4(ndc, 0.0f, 1.0f);  // 近平面
    float4 farPoint = float4(ndc, 1.0f, 1.0f);   // 遠平面
    
    float4 nearWorld = mul(nearPoint, gInvViewProjection);
    nearWorld.xyz /= nearWorld.w;
    
    float4 farWorld = mul(farPoint, gInvViewProjection);
    farWorld.xyz /= farWorld.w;
    
    // レイの原点と方向を計算
    float3 rayOrigin = gCameraPosition;
    float3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);
    
    // 雲のAABB（ワールド空間固定）
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
    // レイとAABBの交差判定
    float tNear, tFar;
    if (!IntersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }
    
    tNear = max(tNear, 0.0f);
    tFar = min(tFar, gMaxDistance);
    
    if (tFar <= tNear) {
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }
    
    // レイマーチング
    float3 sunDir = normalize(gSunDirection);
    float3 accumulatedLight = 0.0f;
    float transmittance = 1.0f;
    
    float t = tNear;
    float marchDistance = tFar - tNear;
    int numSteps = min(int(marchDistance / gStepSize), MAX_STEPS);
    float actualStepSize = marchDistance / float(max(numSteps, 1));
    
    int denseSampleCount = 0;
    float firstHitDepth = tFar; // 最初に密度が検出された位置
    bool hasHit = false;
    
    for (int i = 0; i < numSteps && transmittance > 0.01f; i++) {
        float3 position = rayOrigin + rayDir * (t + actualStepSize * 0.5f);
        
        float density = SampleCloudDensity(position);
        
        if (density > 0.001f) {
            if (!hasHit) {
                firstHitDepth = t;
                hasHit = true;
            }
            
            denseSampleCount++;
            
            float lightEnergy = LightMarch(position);
            float phase = PhaseHG(dot(rayDir, sunDir), gAnisotropy);
            float3 lighting = gAmbient + gSunColor * gSunIntensity * lightEnergy * phase;
            
            float scatterAmount = density * actualStepSize;
            accumulatedLight += lighting * scatterAmount * transmittance;
            transmittance *= exp(-density * actualStepSize);
        }
        
        t += actualStepSize;
    }
    
    float alpha = 1.0f - transmittance;
    
    // 深度値を計算（修正版）
    if (hasHit) {
        float3 hitPos = rayOrigin + rayDir * firstHitDepth;
        float4 clipPos = mul(float4(hitPos, 1.0f), gViewProjection);
        output.depth = clipPos.z / clipPos.w;
    }
    
    // デバッグモード
    if (gDebugFlag > 0.5f) {
        if (denseSampleCount == 0) {
            output.color = float4(0.0f, 0.0f, 1.0f, 0.5f); // 青
        } else if (alpha < 0.01f) {
            output.color = float4(1.0f, 1.0f, 0.0f, 0.5f); // 黄色
        } else {
            output.color = float4(accumulatedLight + float3(0.2f, 0.5f, 0.2f), alpha); // 緑
        }
        return output;
    }
    
    output.color = float4(accumulatedLight, alpha);
    return output;
}