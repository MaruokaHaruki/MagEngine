#include "Cloud.hlsli"

///=============================================================================
///						VertexShader
VertexShaderOutput main(VertexShaderInput input){
    VertexShaderOutput output;
    // NDC座標をそのまま使用（フルスクリーンクアッド）
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    
    // 深度値を最大に設定（遠平面）して、ピクセルシェーダーで正しい深度を計算
    output.position.z = 1.0f;
    
    return output;
}