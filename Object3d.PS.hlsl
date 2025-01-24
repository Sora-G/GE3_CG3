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
    output.color = gMaterial.color;

    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = gMaterial.color * textureColor;
    
    
    if (gMaterial.enableLighting != 0)//Lightingする場合
    {
        
        float cos = saturate(dot(normalize(input.normal)), normalize(-gDirecitonalLight.direction));
        output.color = gMaterial.color * textureColor * gDirecitonalLight.color * cos * gDirecitonalLight.intensity;
        float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    
        float32_t3 reflectLight = reflect(normalize(gDirecitonalLight.direction), normalize(input.normal));
    
        float RdotE = dot(reflectLight, toEye);
        float specularPow = pow(saturate(RdotE), gMaterial.shininess); //反射鏡面
    
    //拡散反射
        float32_t3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirecitonalLight.color.rgb * cos * gDirecitonalLight.intensity;
    //鏡面反射
        float32_t3 specular = gDirecitonalLight.color.rgb * gDirecitonalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
    //拡散反射+鏡面反射
        output.color.rgb = diffuse + specular;
    //アルファは今まで通り
        output.color.a = gMaterial.color.a * textureColor.a;
       
    }
    else//Lightingしない場合
    {
        output.color = gMaterial.color * textureColor;
    }
	return output;
}