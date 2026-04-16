#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float radius;
    float decay;
};

struct Camera
{
    float32_t3 worldPosition;
};


ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord,0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    float32_t3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
   
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float32_t3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
    float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
   
    float32_t3 reflectPointLight = reflect(gPointLight.position, normalize(input.normal));
    float32_t3 halfVectorPoint = normalize(pointLightDirection + toEye);
    
    float NdotH = dot(normalize(input.normal), halfVector);
    float specularPow = pow(saturate(NdotH), gMaterial.shininess);
    
    if(textureColor.a <= 0.5)
    {
        discard;
    }
    
    if(gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float NdotLP = dot(normalize(input.normal), -pointLightDirection);
        float cosP = pow(NdotLP * 0.5f + 0.5f, 2.0f);
        float32_t distance = length(gPointLight.position - input.worldPosition);
        float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);
        
        float32_t3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        float32_t3 specularDirectionalLight = gMaterial.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
        float32_t3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosP * gPointLight.intensity * factor;
        float32_t3 specularPointLight = gMaterial.color.rgb * gPointLight.intensity * specularPow * (gPointLight.color.rgb * gPointLight.intensity) * factor;
        
        
        output.color.rgb = diffuseDirectionalLight + specularDirectionalLight + diffusePointLight + specularPointLight;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
        
    return output;
}