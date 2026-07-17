/*********************************************************************
 * \file   CloudBulletHole.hlsli
 * \brief  Cloud弾痕マスク合成
 *
 * \note   SDF形状本体はCloudHoleSdf.hlsliへ分離し、ここでは既存密度マスクへの合成だけを扱う。
 *********************************************************************/
#pragma once

#include "CloudHoleSdf.hlsli"

float CalculateBulletHoleMask(float3 position)
{
    float mask = 1.0f;
    if(gBulletHoleCount <= 0) {
        return mask;
    }

    int activeBullets = min(gBulletHoleCount, 8);
    for(int i = 0; i < activeBullets; ++i) {
        if(mask < 0.01f) {
            break;
        }

        BulletHoleGPU hole = gBulletHoles[i];
        if(hole.lifeTime < 0.01f) {
            continue;
        }

        float3 direction = SafeNormalize3(hole.direction, float3(0.0f, 1.0f, 0.0f));
        float3 offset = position - hole.origin;
        float axialDist = dot(offset, direction);
        if(axialDist < 0.0f || axialDist > hole.coneLength) {
            continue;
        }

        float t = axialDist / SafeRadius(hole.coneLength);
        float currentRadius = lerp(hole.startRadius, hole.endRadius, saturate(t));
        float roughRadius = max(hole.startRadius, hole.endRadius) * max(SafeAspectRatio(hole.aspectRatio), 1.0f) + gBulletHoleFadeEnd;
        float3 perpendicular = offset - axialDist * direction;
        if(length(perpendicular) > roughRadius * 1.75f) {
            continue;
        }

        // NOTE: Shape IDの分岐先だけを評価し、全Shapeのmin合成は行わない。
        float2 localPosition = TransformHoleLocalPosition(position, hole, currentRadius);
        float sdfDist = EvaluateCloudHoleSdf(localPosition, hole.shape, hole.shapeParams0, hole.shapeParams1, hole.flags, hole.polygonPointCount) * currentRadius;

        float shapeMask = smoothstep(gBulletHoleFadeStart, gBulletHoleFadeEnd, sdfDist);
        float entryFade = smoothstep(0.0f, hole.coneLength * 0.2f, axialDist);
        float exitFade = smoothstep(hole.coneLength, hole.coneLength * 0.8f, axialDist);
        float axialMask = entryFade * exitFade;
        float holeMask = max(shapeMask, 1.0f - axialMask);
        holeMask = smoothstep(0.0f, 1.0f, holeMask);
        holeMask = smoothstep(0.0f, 1.0f, holeMask);
        holeMask = lerp(1.0f, holeMask, hole.lifeTime);
        mask *= holeMask;
    }

    return mask;
}
