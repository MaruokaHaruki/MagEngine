/*********************************************************************
 * \file   CloudHoleSdf.hlsli
 * \brief  Cloud穴形状用2D SDF
 *
 * \note   Cloud.hlsliへ肥大化したSDF群を置かないため、穴形状だけを分離する。
 *********************************************************************/
#pragma once

static const uint kCloudHoleShapeCircle = 0;
static const uint kCloudHoleShapeRoundedBox = 1;
static const uint kCloudHoleShapeChamferBox = 2;
static const uint kCloudHoleShapeBox = 3;
static const uint kCloudHoleShapeOrientedBox = 4;
static const uint kCloudHoleShapeSegment = 5;
static const uint kCloudHoleShapeRhombus = 6;
static const uint kCloudHoleShapeTrapezoid = 7;
static const uint kCloudHoleShapeParallelogram = 8;
static const uint kCloudHoleShapeEquilateralTriangle = 9;
static const uint kCloudHoleShapeIsoscelesTriangle = 10;
static const uint kCloudHoleShapeTriangle = 11;
static const uint kCloudHoleShapeUnevenCapsule = 12;
static const uint kCloudHoleShapePentagon = 13;
static const uint kCloudHoleShapeHexagon = 14;
static const uint kCloudHoleShapeOctagon = 15;
static const uint kCloudHoleShapeHexagram = 16;
static const uint kCloudHoleShapePentagram = 17;
static const uint kCloudHoleShapeRegularStar = 18;
static const uint kCloudHoleShapePie = 19;
static const uint kCloudHoleShapeCutDisk = 20;
static const uint kCloudHoleShapeArc = 21;
static const uint kCloudHoleShapeRing = 22;
static const uint kCloudHoleShapeHorseshoe = 23;
static const uint kCloudHoleShapeVesica = 24;
static const uint kCloudHoleShapeOrientedVesica = 25;
static const uint kCloudHoleShapeMoon = 26;
static const uint kCloudHoleShapeRoundedCross = 27;
static const uint kCloudHoleShapeEgg = 28;
static const uint kCloudHoleShapeHeart = 29;
static const uint kCloudHoleShapeCross = 30;
static const uint kCloudHoleShapeRoundedX = 31;
static const uint kCloudHoleShapePolygon = 32;
static const uint kCloudHoleShapeEllipse = 33;
static const uint kCloudHoleShapeParabola = 34;
static const uint kCloudHoleShapeParabolaSegment = 35;
static const uint kCloudHoleShapeQuadraticBezier = 36;
static const uint kCloudHoleShapeBlobbyCross = 37;
static const uint kCloudHoleShapeTunnel = 38;
static const uint kCloudHoleShapeStairs = 39;
static const uint kCloudHoleShapeQuadraticCircle = 40;
static const uint kCloudHoleShapeHyperbola = 41;
static const uint kCloudHoleShapeCoolS = 42;
static const uint kCloudHoleShapeCircleWave = 43;
static const uint kCloudHoleFlagRounded = 1u;
static const uint kCloudHoleFlagOnion = 2u;
static const uint kMaxCloudHolePolygonPoints = 8;

float Dot2(float2 v) { return dot(v, v); }
float2 Rotate2D(float2 p, float angle) { float s = sin(angle); float c = cos(angle); return float2(c * p.x - s * p.y, s * p.x + c * p.y); }
float SafeAspectRatio(float aspectRatio) { return max(abs(aspectRatio), 0.05f); }
float SafeRadius(float radius) { return max(radius, 0.0001f); }
float2 SafeNormalize2(float2 v, float2 fallback) { float l = length(v); return l > 0.0001f ? v / l : fallback; }
float3 SafeNormalize3(float3 v, float3 fallback) { float l = length(v); return l > 0.0001f ? v / l : fallback; }

float2 TransformHoleLocalPosition(float3 worldPosition, BulletHoleGPU hole, float radius)
{
    float3 axis = SafeNormalize3(hole.direction, float3(0.0f, 1.0f, 0.0f));
    float3 up = abs(axis.y) < 0.95f ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 right = SafeNormalize3(cross(up, axis), float3(1.0f, 0.0f, 0.0f));
    float3 bitangent = SafeNormalize3(cross(axis, right), float3(0.0f, 0.0f, 1.0f));
    float3 local = worldPosition - hole.origin;
    float2 p = float2(dot(local, right), dot(local, bitangent)) / SafeRadius(radius);
    p = Rotate2D(p, -hole.rotation);
    p.x /= SafeAspectRatio(hole.aspectRatio);
    return p;
}

float SdCircle(float2 p, float r) { return length(p) - r; }
float SdBox(float2 p, float2 b) { float2 d = abs(p) - b; return length(max(d, 0.0f)) + min(max(d.x, d.y), 0.0f); }
float SdRoundedBox(float2 p, float2 b, float r) { return SdBox(p, max(b - r, 0.001f)) - r; }
float SdChamferBox(float2 p, float2 b, float c) { float2 q = abs(p) - b; return max(max(q.x, q.y), (q.x + q.y + c) * 0.70710678f); }
float SdSegment(float2 p, float2 a, float2 b) { float2 pa = p - a; float2 ba = b - a; float h = saturate(dot(pa, ba) / max(dot(ba, ba), 0.0001f)); return length(pa - ba * h) - 0.08f; }
float SdRhombus(float2 p, float2 b) { p = abs(p); float h = clamp((-2.0f * dot(p, b) + dot(b, b)) / max(dot(b, b), 0.0001f), -1.0f, 1.0f); float d = length(p - 0.5f * b * float2(1.0f - h, 1.0f + h)); return d * sign(p.x * b.y + p.y * b.x - b.x * b.y); }
float SdTrapezoid(float2 p, float r1, float r2, float he) { float2 k1 = float2(r2, he); float2 k2 = float2(r2 - r1, 2.0f * he); p.x = abs(p.x); float2 ca = float2(p.x - min(p.x, (p.y < 0.0f) ? r1 : r2), abs(p.y) - he); float2 cb = p - k1 + k2 * clamp(dot(k1 - p, k2) / max(dot(k2, k2), 0.0001f), 0.0f, 1.0f); float s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f; return s * sqrt(min(dot(ca, ca), dot(cb, cb))); }
float SdParallelogram(float2 p, float wi, float he, float sk) { float2 e = float2(sk, he); p = (p.y < 0.0f) ? -p : p; float2 w = p - e; w.x -= clamp(w.x, -wi, wi); float d = dot(w, w); float s = p.x * e.y - p.y * e.x > 0.0f ? 1.0f : -1.0f; return sqrt(d) * s; }
float SdEquilateralTriangle(float2 p) { const float k = 1.7320508f; p.x = abs(p.x) - 1.0f; p.y = p.y + 0.57735027f; if(p.x + k * p.y > 0.0f) p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0f; p.x -= clamp(p.x, -2.0f, 0.0f); return -length(p) * sign(p.y); }
float SdIsoscelesTriangle(float2 p, float2 q) { p.x = abs(p.x); float2 a = p - q * clamp(dot(p, q) / max(dot(q, q), 0.0001f), 0.0f, 1.0f); float2 b = p - q * float2(clamp(p.x / max(q.x, 0.0001f), 0.0f, 1.0f), 1.0f); float s = -sign(q.y); float2 d = min(float2(dot(a, a), s * (p.x * q.y - p.y * q.x)), float2(dot(b, b), s * (p.y - q.y))); return -sqrt(d.x) * sign(d.y); }
float SdTriangle(float2 p) { float d = max(max(dot(p, SafeNormalize2(float2(0.8f, 0.35f), float2(1, 0))) - 0.55f, dot(p, SafeNormalize2(float2(-0.75f, 0.45f), float2(-1, 0))) - 0.55f), -p.y - 0.55f); return d; }
float SdUnevenCapsule(float2 p, float r1, float r2, float h) { p.x = abs(p.x); float b = (r1 - r2) / max(h, 0.0001f); float a = sqrt(max(1.0f - b * b, 0.0f)); float k = dot(p, float2(-b, a)); if(k < 0.0f) return length(p) - r1; if(k > a * h) return length(p - float2(0.0f, h)) - r2; return dot(p, float2(a, b)) - r1; }
float SdRegularPolygon(float2 p, int n) { float an = 6.2831853f / max(n, 3); float a = atan2(p.y, p.x) + an * 0.5f; float sector = floor(a / an); float2 q = Rotate2D(p, -sector * an); return q.x * cos(an * 0.5f) + abs(q.y) * sin(an * 0.5f) - 0.82f; }
float SdStar(float2 p, int n, float innerRatio) { float an = 3.14159265f / max(n, 2); float a = atan2(p.y, p.x); float r = length(p); float k = frac(a / an * 0.5f + 0.5f); float target = lerp(1.0f, clamp(innerRatio, 0.1f, 0.95f), abs(k * 2.0f - 1.0f)); return r - target; }
float SdPie(float2 p, float c) { float2 q = float2(abs(p.x), p.y); float2 dir = SafeNormalize2(float2(sin(c), cos(c)), float2(0.5f, 0.86f)); return max(length(q) - 1.0f, dot(q, dir) < 0.0f ? length(q) * sign(-q.y) : dot(q, float2(dir.y, -dir.x))); }
float SdCutDisk(float2 p, float h) { float w = sqrt(max(1.0f - h * h, 0.0f)); p.x = abs(p.x); float s = max((h - 1.0f) * p.x * p.x + w * w * (h + 1.0f - 2.0f * p.y), h * p.x - w * p.y); return (s < 0.0f) ? length(p) - 1.0f : (p.x < w ? h - p.y : length(p - float2(w, h))); }
float SdArc(float2 p, float thickness, float angle) { p.x = abs(p.x); float2 sc = float2(sin(angle), cos(angle)); float k = (sc.y * p.x > sc.x * p.y) ? dot(p, sc) : length(p); return sqrt(dot(p, p) + 1.0f - 2.0f * k) - thickness; }
float SdRing(float2 p, float thickness) { return abs(length(p) - 0.72f) - thickness; }
float SdHorseshoe(float2 p, float thickness, float angle) { return max(SdArc(p, thickness, angle), -p.y - 0.25f); }
float SdVesica(float2 p, float offset) { float d = max(length(p - float2(offset, 0.0f)) - 0.8f, length(p + float2(offset, 0.0f)) - 0.8f); return d; }
float SdMoon(float2 p, float offset) { return max(length(p) - 1.0f, -(length(p - float2(offset, 0.0f)) - 0.9f)); }
float SdCross(float2 p, float arm) { float d1 = SdBox(p, float2(arm, 0.85f)); float d2 = SdBox(p, float2(0.85f, arm)); return min(d1, d2); }
float SdRoundedX(float2 p, float arm) { float2 q = Rotate2D(p, 0.78539816f); return min(SdRoundedBox(q, float2(arm, 0.9f), 0.12f), SdRoundedBox(q, float2(0.9f, arm), 0.12f)); }
float SdEgg(float2 p) { p.y += 0.12f; float k = p.y > 0.0f ? 0.85f : 1.2f; return length(float2(p.x, p.y * k)) - 0.75f; }
float SdHeart(float2 p) { p.x = abs(p.x); p.y += 0.25f; if(p.y + p.x > 1.0f) return sqrt(dot(p - float2(0.25f, 0.75f), p - float2(0.25f, 0.75f))) - 0.35f; return sqrt(min(dot(p - float2(0.0f, 0.55f), p - float2(0.0f, 0.55f)), dot(p - 0.5f * max(p.x + p.y, 0.0f), p - 0.5f * max(p.x + p.y, 0.0f)))) * sign(p.x - p.y); }
float SdEllipse(float2 p, float aspect) { float2 ab = float2(SafeAspectRatio(aspect), 1.0f); return (length(p / ab) - 1.0f) * min(ab.x, ab.y); }
float SdParabola(float2 p, float k) { float x = clamp(p.x, -1.0f, 1.0f); return length(p - float2(x, k * x * x - 0.5f)) - 0.08f; }
float SdParabolaSegment(float2 p, float k, float width) { float x = clamp(p.x, -width, width); return length(p - float2(x, k * x * x - 0.5f)) - 0.08f; }
float SdQuadraticBezier(float2 p, float2 a, float2 b, float2 c) { float d = 10000.0f; float2 prev = a; [unroll] for(int i = 1; i <= 8; ++i) { float t = float(i) / 8.0f; float u = 1.0f - t; float2 samplePoint = u * u * a + 2.0f * u * t * b + t * t * c; d = min(d, SdSegment(p, prev, samplePoint)); prev = samplePoint; } return d; }
float SdBlobbyCross(float2 p) { return min(SdRoundedBox(p, float2(0.18f, 0.85f), 0.18f), SdRoundedBox(p, float2(0.85f, 0.18f), 0.18f)); }
float SdTunnel(float2 p) { return max(SdBox(p - float2(0.0f, -0.25f), float2(0.65f, 0.55f)), -(length(p - float2(0.0f, 0.25f)) - 0.55f)); }
float SdStairs(float2 p, float steps) { float n = clamp(round(steps), 2.0f, 8.0f); float2 q = p + 0.5f; q = floor(q * n) / n - q; return max(abs(q.x), abs(q.y)) - 0.04f; }
float SdQuadraticCircle(float2 p) { return abs(length(p) - 0.72f) - 0.08f + 0.08f * sin(atan2(p.y, p.x) * 4.0f); }
float SdHyperbola(float2 p) { return abs(p.x * p.y) - 0.18f; }
float SdCoolS(float2 p) { return min(min(SdSegment(p, float2(-0.45f, 0.55f), float2(0.45f, 0.55f)), SdSegment(p, float2(-0.45f, -0.55f), float2(0.45f, -0.55f))), min(SdSegment(p, float2(-0.45f, 0.55f), float2(0.45f, -0.55f)), SdSegment(p, float2(0.45f, 0.55f), float2(-0.45f, -0.55f)))); }
float SdCircleWave(float2 p, float amount, float frequency) { float a = atan2(p.y, p.x); return length(p) - (0.72f + amount * 0.12f * sin(a * max(frequency, 1.0f))); }

float ApplyCloudHoleModifiers(float sdf, float4 shapeParams0, float4 shapeParams1, uint flags)
{
    if((flags & kCloudHoleFlagRounded) != 0u) {
        sdf -= clamp(abs(shapeParams1.z), 0.02f, 0.25f);
    }
    if((flags & kCloudHoleFlagOnion) != 0u) {
        sdf = abs(sdf) - clamp(abs(shapeParams1.w), 0.02f, 0.25f);
    }
    return sdf;
}

float EvaluateCloudHoleSdf(float2 p, uint shapeId, float4 p0, float4 p1, uint flags, uint polygonPointCount)
{
    float d = 10000.0f;
    if(shapeId == kCloudHoleShapeCircle) d = SdCircle(p, 1.0f);
    else if(shapeId == kCloudHoleShapeRoundedBox) d = SdRoundedBox(p, float2(0.75f, 0.75f), clamp(p0.x, 0.0f, 0.45f));
    else if(shapeId == kCloudHoleShapeChamferBox) d = SdChamferBox(p, float2(0.75f, 0.75f), clamp(p0.x, 0.0f, 0.5f));
    else if(shapeId == kCloudHoleShapeBox) d = SdBox(p, float2(0.8f, 0.8f));
    else if(shapeId == kCloudHoleShapeOrientedBox) d = SdBox(Rotate2D(p, 0.55f), float2(0.8f, 0.45f));
    else if(shapeId == kCloudHoleShapeSegment) d = SdSegment(p, float2(-0.85f, 0.0f), float2(0.85f, 0.0f));
    else if(shapeId == kCloudHoleShapeRhombus) d = SdRhombus(p, float2(0.85f, 0.65f));
    else if(shapeId == kCloudHoleShapeTrapezoid) d = SdTrapezoid(p, clamp(p0.x, 0.2f, 1.0f), 0.85f, 0.75f);
    else if(shapeId == kCloudHoleShapeParallelogram) d = SdParallelogram(p, 0.7f, 0.65f, clamp(p0.x, -0.8f, 0.8f));
    else if(shapeId == kCloudHoleShapeEquilateralTriangle) d = SdEquilateralTriangle(p);
    else if(shapeId == kCloudHoleShapeIsoscelesTriangle) d = SdIsoscelesTriangle(p, float2(0.8f, 0.9f));
    else if(shapeId == kCloudHoleShapeTriangle) d = SdTriangle(p);
    else if(shapeId == kCloudHoleShapeUnevenCapsule) d = SdUnevenCapsule(p, clamp(p0.x, 0.1f, 0.8f), clamp(p0.y, 0.1f, 0.8f), 1.0f);
    else if(shapeId == kCloudHoleShapePentagon) d = SdRegularPolygon(p, 5);
    else if(shapeId == kCloudHoleShapeHexagon) d = SdRegularPolygon(p, 6);
    else if(shapeId == kCloudHoleShapeOctagon) d = SdRegularPolygon(p, 8);
    else if(shapeId == kCloudHoleShapeHexagram) d = SdStar(p, 6, 0.45f);
    else if(shapeId == kCloudHoleShapePentagram) d = SdStar(p, 5, 0.42f);
    else if(shapeId == kCloudHoleShapeRegularStar) d = SdStar(p, (int)clamp(round(p0.x), 3.0f, 8.0f), clamp(p0.y, 0.15f, 0.9f));
    else if(shapeId == kCloudHoleShapePie) d = SdPie(p, clamp(p0.x, 0.15f, 1.3f));
    else if(shapeId == kCloudHoleShapeCutDisk) d = SdCutDisk(p, clamp(p0.x, -0.8f, 0.8f));
    else if(shapeId == kCloudHoleShapeArc) d = SdArc(p, clamp(p0.x, 0.03f, 0.5f), clamp(p0.y, 0.1f, 1.4f));
    else if(shapeId == kCloudHoleShapeRing) d = SdRing(p, clamp(p0.x, 0.03f, 0.45f));
    else if(shapeId == kCloudHoleShapeHorseshoe) d = SdHorseshoe(p, clamp(p0.x, 0.03f, 0.45f), clamp(p0.y, 0.1f, 1.4f));
    else if(shapeId == kCloudHoleShapeVesica) d = SdVesica(p, 0.35f);
    else if(shapeId == kCloudHoleShapeOrientedVesica) d = SdVesica(Rotate2D(p, 0.55f), 0.35f);
    else if(shapeId == kCloudHoleShapeMoon) d = SdMoon(p, clamp(p0.x, 0.1f, 0.8f));
    else if(shapeId == kCloudHoleShapeRoundedCross) {
        d = min(SdRoundedBox(p, float2(0.2f, 0.85f), 0.18f), SdRoundedBox(p, float2(0.85f, 0.2f), 0.18f));
    }
    else if(shapeId == kCloudHoleShapeEgg) d = SdEgg(p);
    else if(shapeId == kCloudHoleShapeHeart) d = SdHeart(p);
    else if(shapeId == kCloudHoleShapeCross) d = SdCross(p, clamp(p0.x, 0.08f, 0.5f));
    else if(shapeId == kCloudHoleShapeRoundedX) d = SdRoundedX(p, clamp(p0.x, 0.08f, 0.5f));
    else if(shapeId == kCloudHoleShapePolygon) d = SdRegularPolygon(p, (int)clamp(polygonPointCount, 3u, kMaxCloudHolePolygonPoints));
    else if(shapeId == kCloudHoleShapeEllipse) d = SdEllipse(p, 1.5f);
    else if(shapeId == kCloudHoleShapeParabola) d = SdParabola(p, max(abs(p0.x), 0.1f));
    else if(shapeId == kCloudHoleShapeParabolaSegment) d = SdParabolaSegment(p, max(abs(p0.x), 0.1f), clamp(p0.y, 0.2f, 1.2f));
    else if(shapeId == kCloudHoleShapeQuadraticBezier) d = SdQuadraticBezier(p, p0.xy, p0.zw, p1.xy);
    else if(shapeId == kCloudHoleShapeBlobbyCross) d = SdBlobbyCross(p);
    else if(shapeId == kCloudHoleShapeTunnel) d = SdTunnel(p);
    else if(shapeId == kCloudHoleShapeStairs) d = SdStairs(p, p0.x);
    else if(shapeId == kCloudHoleShapeQuadraticCircle) d = SdQuadraticCircle(p);
    else if(shapeId == kCloudHoleShapeHyperbola) d = SdHyperbola(p);
    else if(shapeId == kCloudHoleShapeCoolS) d = SdCoolS(p);
    else if(shapeId == kCloudHoleShapeCircleWave) d = SdCircleWave(p, p0.x, p0.y);
    else d = SdCircle(p, 1.0f);

    return ApplyCloudHoleModifiers(d, p0, p1, flags);
}
