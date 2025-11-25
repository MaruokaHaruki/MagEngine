/*********************************************************************
 * \file   Cloud.PS.hlsl
 * \brief
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note
 *********************************************************************/
#include "Cloud.hlsli"

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.depth = 1.0f;
    
    float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    
    float4 nearPoint = float4(ndc, 0.0f, 1.0f);
    float4 farPoint = float4(ndc, 1.0f, 1.0f);
    
    float4 nearWorld = mul(nearPoint, gInvViewProjection);
    nearWorld.xyz /= nearWorld.w;
    
    float4 farWorld = mul(farPoint, gInvViewProjection);
    farWorld.xyz /= farWorld.w;
    
    float3 rayOrigin = gCameraPosition;
    float3 rayDir = normalize(farWorld.xyz - nearWorld.xyz);
    
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;
    
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
    
    float3 sunDir = normalize(gSunDirection);
    float3 accumulatedLight = 0.0f;
    float transmittance = 1.0f;
    
    float t = tNear;
    float marchDistance = tFar - tNear;
    int numSteps = min(int(marchDistance / gStepSize), MAX_STEPS);
    float actualStepSize = marchDistance / float(max(numSteps, 1));
    
    int denseSampleCount = 0;
    bool foundFirstHit = false;
    
    for (int i = 0; i < numSteps && transmittance > 0.01f; i++) {
        float3 position = rayOrigin + rayDir * (t + actualStepSize * 0.5f);
        
        float density = SampleCloudDensity(position);
        
        if (density > 0.001f) {
            denseSampleCount++;
            
            if (!foundFirstHit) {
                // ビュープロジェクション行列を逆行列から復元
                float4x4 viewProj = InvertMatrix(gInvViewProjection);
                
                // ワールド座標をクリップ空間に変換
                float4 clipPos = mul(float4(position, 1.0f), viewProj);
                
                // NDC深度を計算（パースペクティブディバイド後）
                output.depth = clipPos.z / clipPos.w;
                
                foundFirstHit = true;
            }
            
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
    
    // デバッグモード
    if (gDebugFlag > 0.5f) {
        if (denseSampleCount == 0) {
            output.color = float4(0.0f, 0.0f, 1.0f, 0.5f);
        } else if (alpha < 0.01f) {
            output.color = float4(1.0f, 1.0f, 0.0f, 0.5f);
        } else {
            output.color = float4(accumulatedLight + float3(0.2f, 0.5f, 0.2f), alpha);
        }
        return output;
    }
    
    output.color = float4(accumulatedLight, alpha);
    return output;
}