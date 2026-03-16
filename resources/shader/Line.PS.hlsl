#include "Line.hlsli"

struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float thickness : THICKNESS;
};

float4 main(PixelInput input) : SV_TARGET
{
    // 太さ情報を使用してアルファを調整することもできます
    // 例：太さが小さければアルファを落とすなど
    return input.color;
}
