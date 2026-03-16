#include "Skybox.hlsli"

///=============================================================================
///						リソース
// キューブマップテクスチャ
TextureCube<float4> gTexture : register(t0);

///=============================================================================
///						PixelShader
PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    
    //========================================
    // ライティング計算（各ライト型で異なる処理）
    float3 viewDir = normalize(input.texcoord);  // スカイボックステクスチャ座標を視線方向として使用
    float3 lighting = float3(0.0f, 0.0f, 0.0f);
    
    //========================================
    // 1. DirectionalLight：グローバルな方向光，全体的な照明
    // intensity が 0 なら完全に黒、1 なら元の色
    if (gDirectionalLight.intensity > 0.0f) {
        float3 dirLightDir = normalize(-gDirectionalLight.direction);
        float dirAlignment = saturate(dot(viewDir, dirLightDir));
        lighting += gDirectionalLight.color.rgb * gDirectionalLight.intensity * (0.5f + 0.5f * dirAlignment);
    }
    
    //========================================
    // 2. PointLight：点光源，距離減衰で効果が変わる
    // カメラがスカイボックス中心にあるため、PointLight.positionの方向から光が来ていると考える
    if (gPointLight.intensity > 0.0f) {
        float3 pointLightDir = normalize(gPointLight.position);
        
        // 視線方向とポイントライト方向の角度を計算
        float alignment = dot(viewDir, pointLightDir);
        
        // 背後からの光は減弱（フェード開始）
        float angularFalloff = saturate(alignment * 0.5f + 0.5f);  // -1.0～1.0 を 0.0～1.0 に変換
        
        // 距離による減衰（ポイントライトが近いと強い効果）
        float distance = length(gPointLight.position);
        float distanceAttenuation = 1.0f / (1.0f + gPointLight.decay * distance * distance);
        
        // ポイントライトの色を加算
        float3 pointLight = gPointLight.color.rgb * gPointLight.intensity * angularFalloff * distanceAttenuation;
        lighting += pointLight;
    }
    
    //========================================
    // 3. SpotLight：スポット光源，角度フォールオフで効果が変わる
    // スポット方向とビュー方向の角度を計算し、コーン状のライト効果を表現
    if (gSpotLight.intensity > 0.0f) {
        float3 spotDir = normalize(gSpotLight.direction);
        
        // ビュー方向とスポット方向のコサイン値を計算
        float cosTheta = dot(viewDir, spotDir);
        
        // フォールオフを計算（cosFalloffEnd から cosFalloffStart へ smoothstep）
        // cosFalloffStart: 光が明るい領域の内側
        // cosFalloffEnd: フェードアウト開始の外側
        float falloff = smoothstep(gSpotLight.cosFalloffEnd, gSpotLight.cosFalloffStart, cosTheta);
        
        // クローズアップ感を出すため、フォールオフをより強調
        falloff = falloff * falloff;  // 二乗で強調
        
        // スポットライトの色を加算
        float3 spotLight = gSpotLight.color.rgb * gSpotLight.intensity * falloff;
        lighting += spotLight;
    }
    
    // 最終的な色 = テクスチャ色 * ライト
    output.color = textureColor * float4(lighting, 1.0f);
    
    return output;
}