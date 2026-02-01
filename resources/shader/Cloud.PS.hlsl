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
    float3 sunDir = normalize(gSunDirection);  // 太陽光の方向
    float3 accumulatedLight = 0.0f;            // 累積光量
    float transmittance = 1.0f;                // 透過率（初期値は完全透過）
    
    //========================================
    // レイマーチングのステップ計算
    float t = tNear;                                    // 現在のレイパラメータ
    float marchDistance = tFar - tNear;                 // マーチング総距離
    int numSteps = min(int(marchDistance / gStepSize), MAX_STEPS);  // ステップ数を計算
    float actualStepSize = marchDistance / float(max(numSteps, 1)); // 実際のステップサイズ
    
    //========================================
    // デバッグ用変数
    int denseSampleCount = 0;       // 密度が検出された回数
    float firstHitDepth = tFar;     // 最初に雲に衝突した位置
    bool hasHit = false;            // 雲に衝突したかどうか
    
    //========================================
    // レイマーチングループ
    for (int i = 0; i < numSteps && transmittance > 0.01f; i++) {
        float3 position = rayOrigin + rayDir * (t + actualStepSize * 0.5f);
        
        float density = SampleCloudDensity(position);
        
        // 影響ポイント効果を適用：密度を減少させる
        if (gImpactPointCount > 0) {
            float impactEffect = CalculateImpactEffect(position);
            density *= (1.0f - impactEffect * gImpactInfluence);
        }
        
        // 適応的ステップサイズ: 密度が低い場合は大きくスキップ
        float adaptiveStep = (density < 0.001f) ? actualStepSize * 2.0f : actualStepSize;
        
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