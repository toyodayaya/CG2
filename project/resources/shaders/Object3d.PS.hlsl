#include "Object3d.hlsli"

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t shininess;
    float32_t environmentCoefficient;
    int32_t useEnvironment;
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

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    float32_t cosFalloffStart;
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
ConstantBuffer<SpotLight> gSpotLight : register(b4);
TextureCube<float32_t4> gEnvironmentTexture : register(t1);

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
    float32_t3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);
   
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float32_t3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
    float32_t3 halfVector = normalize(-gDirectionalLight.direction + toEye);
   
    float32_t3 reflectPointLight = reflect(gPointLight.position, normalize(input.normal));
    float32_t3 halfVectorPoint = normalize(pointLightDirection + toEye);
    
    float32_t3 reflectSpotLight = reflect(gSpotLight.position, normalize(input.normal));
    float32_t3 halfVectorSpot = normalize(spotLightDirectionOnSurface + toEye);
    
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
        float NdotLS = dot(normalize(input.normal), -spotLightDirectionOnSurface);
        float cosS = pow(NdotLS * 0.5f + 0.5f, 2.0f);
        
        float32_t distance = length(gPointLight.position - input.worldPosition);
        float32_t factor = pow(saturate(-distance / gPointLight.radius + 1.0), gPointLight.decay);
        
        float32_t attenuation = length(gSpotLight.position - input.worldPosition);
        float32_t attenuationFactor = pow(saturate(-attenuation / gSpotLight.distance + 1.0), gSpotLight.decay);
        float32_t cosAngle = dot(spotLightDirectionOnSurface, gSpotLight.direction);
        float32_t falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
        
        float32_t3 diffuseDirectionalLight = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        float32_t3 specularDirectionalLight = gMaterial.color.rgb * gDirectionalLight.intensity * specularPow * float32_t3(1.0f, 1.0f, 1.0f);
        float32_t3 diffusePointLight = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosP * gPointLight.intensity * factor;
        float32_t3 specularPointLight = gMaterial.color.rgb * gPointLight.intensity * specularPow * (gPointLight.color.rgb * gPointLight.intensity) * factor;
        float32_t3 diffuseSpotLight = gMaterial.color.rgb * textureColor.rgb * gSpotLight.color.rgb * cosS * gSpotLight.intensity * attenuationFactor * falloffFactor;
        float32_t3 specularSpotLight = gMaterial.color.rgb * gSpotLight.intensity * specularPow * (gSpotLight.color.rgb * gSpotLight.intensity) * attenuationFactor * falloffFactor;
        
        output.color.rgb = diffuseDirectionalLight + specularDirectionalLight + diffusePointLight + specularPointLight + diffuseSpotLight + specularSpotLight;
        output.color.a = gMaterial.color.a * textureColor.a;
        
        if(gMaterial.useEnvironment != 0)
        {
            float32_t3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
            float32_t3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
            float32_t4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        
            output.color.rgb += (environmentColor.rgb * gMaterial.environmentCoefficient);
        }

    }
    else
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
        
    return output;
}