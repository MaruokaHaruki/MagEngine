#include "Skybox.hlsli"

///=============================================================================
///						コンスタントバッファ
// 変換行列
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

///=============================================================================
///						VertexShader
VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    // 位置座標をビュープロジェクション行列で変換
    output.position = mul(input.position, gTransformationMatrix.WVP);
    // zをwと同じ値にして最遠方に配置
    output.position.z = output.position.w;
    // 頂点位置をそのままテクスチャ座標として使用（キューブマップ用）
    output.texcoord = input.position.xyz;
    return output;
}