#include "Cloud.hlsli"

///=============================================================================
///						VertexShader
VertexShaderOutput main(VertexShaderInput input){
    VertexShaderOutput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}