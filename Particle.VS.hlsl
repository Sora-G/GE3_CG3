#include "Particle.hlsli"

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(b0);

struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(VertexShaderInput input,uint32_t instanced : SV_InstancedID)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrices[instanced].WVP);
    output.texcoord = input.texcoord;
    return output;
}