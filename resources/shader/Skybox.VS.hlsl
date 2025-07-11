#include "Skybox.hlsli"

///=============================================================================
///						コンスタントバッファ
// 変換行列
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

///=============================================================================
///						VertexShader
SkyboxVertexOutput main(VertexShaderInput input) {
    SkyboxVertexOutput output;
    // 位置座標をビュープロジェクション行列で変換
    output.position = mul(input.position, gTransformationMatrix.WVP).zyww; // zとwを保持し、xは無視する（スカイボックス用）
    // 頂点位置をそのままテクスチャ座標として使用（キューブマップ用）
    output.texcoord = input.position.xyz;
    return output;
}