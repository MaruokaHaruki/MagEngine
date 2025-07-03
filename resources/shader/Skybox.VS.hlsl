#include "Object3d.hlsli"

///=============================================================================
///						コンスタントバッファ

// 変換行列
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

///=============================================================================
///						VertexShader
VertexShaderOutput main(VertexShaderInput input){
    VertexShaderOutput output;
    output.texcoord = input.texcoord;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    //NOTE:法線の変換には拡縮回転情報のみが必要なため取り出す処理を行っている
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.WorldInverseTranspose));
    output.worldPosition = (float3) mul(input.position, gTransformationMatrix.World).xyz; // 修正
    
    return output;
}