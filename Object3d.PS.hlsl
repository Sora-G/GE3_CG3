#include "Object3d.hlsli"

struct Material
{
	float32_t4 color;
    int32_t enableLighting;
};

struct DirectionalLight
{
    float32_t4 color;//ライトの色
    float32_t3 direction;//ライトの向き
    float intensity;//輝度
};

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirecitonalLight : register(b1);

Texture2D<float32_t4>gTexture : register(t0);
SamplerState gSampler : register(t0);
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gMaterial.color;

    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = gMaterial.color * textureColor;

    if (gMaterial.enableLighting != 0)//Lightingする場合
    {
        float cos = saturate(dot(normalize(input.normal)), -gDirecitonalLight.direction);
        output.color = gMaterial.color * textureColor * gDirecitonalLight.color * cos * gDirecitonalLight.intensity;
    }
    else//Lightingしない場合
    {
        output.color = gMaterial.color * textureColor;
    }
	return output;
}