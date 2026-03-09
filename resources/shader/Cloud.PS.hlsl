/*********************************************************************
 * \file   Cloud.PS.hlsl
 * \brief  雲のレンダリング用ピクセルシェーダー
 *
 * \author Harukichimaru
 * \date   March 2025
 * \note   レイマーチングによるボリュメトリック雲の描画
 *********************************************************************/
#include "Cloud.hlsli"

///=============================================================================
///						PixelShader出力構造体
struct PixelOutput {
    float4 color : SV_TARGET0;  // 最終的な色
    float depth : SV_Depth;     // 深度値（Object3Dとの前後関係を正しく表現するため）
};

///=============================================================================
///						メインエントリーポイント
PixelOutput main(VertexShaderOutput input) {
    PixelOutput output;
    // デフォルトの深度値を最遠に設定（雲が存在しない場合は背景として扱う）
    output.depth = 1.0f;
    
    //========================================
    // スクリーン座標からNDC座標への変換
    // UV座標(0~1)をNDC座標(-1~1)に変換
    // Y軸は反転させる（DirectXのUV座標系とNDC座標系の違いを吸収）
    float2 ndc = float2(input.uv.x * 2.0f - 1.0f, 1.0f - input.uv.y * 2.0f);
    
    //========================================
    // レイの方向を計算するための2点を定義
    // 近平面(z=0)と遠平面(z=1)の2点をNDC空間で定義
    float4 nearPoint = float4(ndc, 0.0f, 1.0f);  // 近平面上の点
    float4 farPoint = float4(ndc, 1.0f, 1.0f);   // 遠平面上の点
    
    //========================================
    // NDC空間からワールド空間への変換
    // 逆ビュープロジェクション行列を使用してワールド座標を復元
    float4 nearWorld = mul(nearPoint, gInvViewProjection);
    nearWorld.xyz /= nearWorld.w;  // 同次座標系から3D座標系への変換
    
    float4 farWorld = mul(farPoint, gInvViewProjection);
    farWorld.xyz /= farWorld.w;    // 同次座標系から3D座標系への変換
    
    //========================================
    // レイの原点と方向ベクトルを計算
    float3 rayOrigin = gCameraPosition;                     // レイの始点はカメラ位置
    float3 rayDir = normalize(farWorld.xyz - nearWorld.xyz); // 2点間の方向ベクトルを正規化
    
    //========================================
    // 雲のバウンディングボックス（AABB）を定義
    // 雲の存在範囲を限定することで、計算効率を向上
    float3 boxMin = gCloudCenter - gCloudSize * 0.5f;  // AABBの最小座標
    float3 boxMax = gCloudCenter + gCloudSize * 0.5f;  // AABBの最大座標
    
    //========================================
    // レイとAABBの交差判定
    float tNear, tFar;  // レイのパラメータ（交差区間）
    if (!IntersectAABB(rayOrigin, rayDir, boxMin, boxMax, tNear, tFar)) {
        // AABBと交差しない場合は透明を返す
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }
    
    //========================================
    // レイマーチングの範囲を調整
    tNear = max(tNear, 0.0f);           // カメラより後ろは無視
    tFar = min(tFar, gMaxDistance);     // 最大描画距離でクリップ
    
    // 有効な範囲がない場合は透明を返す
    if (tFar <= tNear) {
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }
    
    //========================================
    // レイマーチングの初期化
    float3 accumulatedLight = 0.0f;            // 累積光量
    float transmittance = 1.0f;                // 透過率（初期値は完全透過）
    
    //========================================
    // レイマーチングのステップ計算
    float t = tNear;
    float marchDistance = tFar - tNear;
    int numSteps = min(int(marchDistance / gStepSize), MAX_STEPS);
    float actualStepSize = marchDistance / float(max(numSteps, 1));
    
    //NOTE : レイマーチループの事前計算をループ外で実施（GPU分岐最適化）
    float3 sunDir = normalize(gDirectionalLight.direction);
    float cosThetaSun = dot(rayDir, sunDir);
    float phaseDual = PhaseDualLobe(cosThetaSun, 0.8f, -0.3f, 0.7f);
    
    //========================================
    // デバッグ用変数
    int denseSampleCount = 0;       // 密度が検出された回数
    float firstHitDepth = tFar;     // 最初に雲に衝突した位置
    bool hasHit = false;            // 雲に衝突したかどうか
    
    //========================================
    // レイマーチングループ
    //NOTE : 透過率閾値を0.01→0.02に緩和（視覚的にほぼ同じで早期終了頻度UP）
    for (int i = 0; i < numSteps && transmittance > 0.02f; i++) {
        float3 position = rayOrigin + rayDir * (t + actualStepSize * 0.5f);
        
        float density = SampleCloudDensity(position);
        
        //NOTE : 空き空間スキップ倍率を2.0→3.0に強化（空の空間を素早く通過）
        float adaptiveStep = (density < 0.001f) ? actualStepSize * 3.0f : actualStepSize;
        
        if (density > 0.001f) {
            if (!hasHit) {
                firstHitDepth = t;
                hasHit = true;
            }
            
            denseSampleCount++;
            
            float3 lighting = float3(0.0f, 0.0f, 0.0f);
            
            //========================================
            // ディレクショナルライト
            float lightEnergy = LightMarch(position);
            //NOTE : PhaseDualLobeをループ外で事前計算済み（レイ方向は不変なため）
            lighting += gDirectionalLight.color.rgb * lightEnergy * phaseDual * gDirectionalLight.intensity * 2.0f;
            
            //========================================
            // 高さベースのアンビエントライト（品質重視）
            // 雲の上部は明るく、下部は暗い（実際の雲の光の散乱を再現）
            float3 uvw = (position - gCloudCenter) / gCloudSize;
            float heightFraction = saturate(uvw.y + 0.5f);
            // 空の色によるアンビエント（上部は暖色、下部は寒色）
            float3 ambientTop = float3(0.35f, 0.35f, 0.4f);    // 上方からの空の散乱光
            float3 ambientBottom = float3(0.12f, 0.12f, 0.16f); // 地面からの反射光
            float3 ambientLight = lerp(ambientBottom, ambientTop, heightFraction) * gAmbient;
            lighting += ambientLight;
            
            //========================================
            //NOTE : ポイントライトは強度>0の場合のみ計算（分岐最適化）
            if (gPointLight.intensity > 0.0f) {
                float3 posToLight = gPointLight.position - position;
                float distToLightSq = dot(posToLight, posToLight); //NOTE : length⇒dotでsqrt回避
                float radiusSq = gPointLight.radius * gPointLight.radius;
                if (distToLightSq < radiusSq) {
                    float distToLight = sqrt(distToLightSq);
                    float3 lightDir2 = posToLight / distToLight; //NOTE : normalizeを手動化（sqrt再利用）
                    float attenuation = 1.0f / (1.0f + gPointLight.decay * distToLightSq);
                    float phasePoint = PhaseDualLobe(dot(rayDir, lightDir2), 0.6f, -0.2f, 0.65f);
                    lighting += gPointLight.color.rgb * phasePoint * gPointLight.intensity * attenuation * 1.2f;
                }
            }
            
            //========================================
            //NOTE : スポットライトは強度>0の場合のみ計算（分岐最適化）
            if (gSpotLight.intensity > 0.0f) {
                float3 posToSpot = gSpotLight.position - position;
                float distToSpotSq = dot(posToSpot, posToSpot); //NOTE : sqrt回避
                float distanceSq = gSpotLight.distance * gSpotLight.distance;
                if (distToSpotSq < distanceSq) {
                    float distToSpot = sqrt(distToSpotSq);
                    float3 spotDir = posToSpot / distToSpot;
                    float angleCos = dot(spotDir, normalize(gSpotLight.direction));
                    
                    float falloff = smoothstep(gSpotLight.cosFalloffEnd, gSpotLight.cosFalloffStart, angleCos);
                    falloff = falloff * falloff;
                    
                    if (falloff > 0.0f) {
                        float attenuation = 1.0f / (1.0f + gSpotLight.decay * distToSpotSq); //NOTE : distToSpotSq再利用
                        float phaseSpot = PhaseDualLobe(dot(rayDir, spotDir), 0.6f, -0.2f, 0.65f);
                        lighting += gSpotLight.color.rgb * phaseSpot * gSpotLight.intensity * attenuation * falloff * 1.3f;
                    }
                }
            }
            
            float scatterAmount = density * actualStepSize;
            accumulatedLight += lighting * scatterAmount * transmittance;
            
            transmittance *= exp(-density * actualStepSize);
        }
        
        t += adaptiveStep;  // 適応的ステップで進む
    }
    
    //========================================
    // 最終的なアルファ値を計算
    // 透過率が低いほど不透明
    float alpha = 1.0f - transmittance;
    
    //========================================
    // 深度値の計算
    // 最初に雲にヒットした位置の深度を書き込む
    // これによりObject3Dとの前後関係が正しく描画される
    if (hasHit) {
        float3 hitPos = rayOrigin + rayDir * firstHitDepth;  // ワールド空間でのヒット位置
        float4 clipPos = mul(float4(hitPos, 1.0f), gViewProjection);  // クリップ空間へ変換
        output.depth = clipPos.z / clipPos.w;  // 深度値を計算
    }
    
    //========================================
    // デバッグモードの処理
    if (gDebugFlag > 0.5f) {
        if (denseSampleCount == 0) {
            // 密度サンプルが0の場合は青色で表示
            output.color = float4(0.0f, 0.0f, 1.0f, 0.5f);
        } else if (alpha < 0.01f) {
            // アルファが低い場合は黄色で表示
            output.color = float4(1.0f, 1.0f, 0.0f, 0.5f);
        } else {
            // 正常な雲は緑がかった色で表示
            output.color = float4(accumulatedLight + float3(0.2f, 0.5f, 0.2f), alpha);
        }
        return output;
    }
    
    //========================================
    // 最終的な色を出力
    output.color = float4(accumulatedLight, alpha);
    return output;
}