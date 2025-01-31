#include "Object3d.hlsli"

struct Material
{
	float32_t4 color;
    int32_t enableLighting;
    float32_t shininess;
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

struct Camera
{
    float32_t3 worldPosition;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirecitonalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);

Texture2D<float32_t4>gTexture : register(t0);
SamplerState gSampler : register(t0);
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), normalize(-gDirecitonalLight.direction));
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);

        float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        float3 reflectLight = reflect(normalize(gDirecitonalLight.direction), normalize(input.normal));

        //float RdotE = dot(reflectLight, toEye);
        //float specularPow = pow(saturate(RdotE), 70.0);

        float3 halfVector = normalize(-gDirecitonalLight.direction + toEye);
        float NDotH = dot(normalize(input.normal), halfVector);
        float specularPow = pow(saturate(NDotH), gMaterial.shininess);
        
        // 拡散反射
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirecitonalLight.color.rgb * cos * gDirecitonalLight.intensity;
        // 鏡面反射
        float3 specular = gDirecitonalLight.color.rgb * gDirecitonalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
        // 拡散反射+鏡面反射
        output.color.rgb = diffuse + specular;
        // アルファは今まで通り
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;

    }

    return output;
}