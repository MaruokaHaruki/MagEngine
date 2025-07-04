#include "Skybox.hlsli"

///=============================================================================
///						コンスタントバッファ
// ViewProjection行列
ConstantBuffer<SkyboxViewProjection> gViewProjection : register(b0);

///=============================================================================
///						VertexShader
SkyboxVertexOutput main(SkyboxVertexInput input) {
    SkyboxVertexOutput output;
    
    // 位置座標をビュープロジェクション行列で変換
    output.position = mul(input.position, gViewProjection.viewProjection);
    
    // スカイボックスは常に最遠に描画されるよう、Z値をW値に設定
    output.position.z = output.position.w;
    
    // 頂点位置をそのままテクスチャ座標として使用（キューブマップ用）
    output.texcoord = input.position.xyz;
    
    return output;
}